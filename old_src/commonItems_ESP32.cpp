/*
 * Apr  06,2025 - Moved Update.onProgress(printProgress) from the apps .ino file to commonItems_ESP32.cpp.  This makes one less item
 *                  that has to be in every apps .ino file
 * Mar  19,2025 - changed sendMQTT_aliveMsgs to sendNrAliveMsgs
 * Jan  01,2023 - Modified setUpEZTime() to allow five attempts at setting the time and if they all fail to then reboot.
 *                 Disabled EzTime events() as it seems to be crashing the program
 *
 *                suli = subtract unsigned long int
 *
 * Mar 31, 2025 - commonItemsFileSysStartup() needs to be added to new apps right after the call to start the file system
                    has been loaded and before other application start fioe system actions occur, eg, startSD(),
                    LittleFS.begin.
 * jun 10, 2024 - moved generateAppData() from the app.ino file to commonItems_ESP32.cpp to enable a more common use of it 
                    across apps.
 *                In the app.h file the definition of:
 *                  #define COMPILE_DATE __DATE__ " " __TIME__
 *                    needs to be changed to:
 *                #define COMPILE_DATE String(__DATE__ " " __TIME__)
 *              - Removed 'doTempStartUp()' to allow the app to choose to use DHT or DS18B20 temperature sensors.
 *                doTempStartUp() will now have to be called from the application startup routine.
 *
 * jul  6, 1024-  Added code to allow the ESP device to be an access point for a pre-determined time (10 minutes at this time)
 *                  and then it will initiate a reboot.  The intent is that if it connect connect to the desired network it
 *                  will try again every 10 minutes.
 * sep  04, 2024 - Added IP address to then String generateAppData() sends out.
 * Jan  30, 2025 - replaced splitString() with strToArray() (moved to myText.cpp) to eliminate a memory leak.
 *                 NOTE: mqttSetup() has been moved to commonItems_ESP32.cpp in commonItemsStartup().  This used to in the 
 *                       apps main.ino file.  Please remove it.
 * Apr  01, 2025 - decremented the below procedures.  These were implemented when the libraries were first created and
 *                  were believed to be useful but in truth have never been used.  Please delete them from anything that
 *                  uses these libraries.
 *                      bool addNewClientToList(AsyncWebSocketClient * client, String type, String CUID);
 *                      String printListOfConnectedClients();
 *                      int getUnusedClientIndex();
 *                      String findClientsByType(String type);
 *                      int findClientIndex(int id, bool logErr);
 *                      findClientByCUIDint findClientByCUID(String CUID, bool adviseOnResults);
 * Apr  10, 2025 -  Added a int parameter to doTempStartUp().  This will set the DS20B18's temp resolution.  This must be a
 *                    number between between 9 and 12 inclusively.
 * May  19, 2025 -  Moved 'tempSenActNeeded()' from commonItemsEvents() to ds18b20Events() in the myTemperature.cpp file to 
 *                    enable app not using DS18B20 temp sensors to compile without errors.
 * jun  10, 2025 -  Moved mqttSetup() from commonItems_ESP32 to myMQTT.cpp.
 * Nov  14, 2025 -  Added:
 *                    createDirIfNotExist(FS_IN_USE, HTMLFsDir)
 *                    to commonItemsFileSysStartup() to ensure the /html directory is created.  This was previously
 *                    taken care of in the application main file but makes more sense in the CommonItems_ESP32 file.
 *                    This can now be removed from the app main file but will do no damge of lert.
*/

#include <ArduinoTrace.h>
#include <AsyncWebSocket.h>

#include <myStuff.h>
#include <myMQTT.h> 
#include <myOTA_update.h>
#include <myText.h>
#include <commonItems_ESP32.h>

String startUpBootLogStr = "";                                                  // string to hold initial log info until the file system is mounted

short CK_HEAP_INTERVAL = 60;                                                    // once an hour
long lastFreeHeap = 0;                                                          // what was the free heap when last checked
unsigned long lastHeapRun = 0;                                                  // last millisecond count the heap was checked
const String dirLists = "/dirLists";                                            // directory on disk for files with folder contents in them

bool  timeActive = false;                                                       // the ESP has a good clock, set to true when connected to a time server

const String compile_date = __DATE__ " " __TIME__;                              // date and time the compiler ran

String tmpBootSyslog = "/tBSyslog.txt";                                         // temporary boot syslog file
String connectedSSID = "";                                                      // what ssid is the ESP connected to
String srvIPAddr = "";                                                          // holds the ip address of the server
String accessPtIP = "";                                                         // holds the server IP Address when it is acting as an access point
String downloadLoc = "";                                                        // contains the path to add to file downloads from clients
String serverUptime = UPTIME;                                                   // how long since the server came on lime
const String appDir = "/appData";                                               // name of the app directory
const String HTMLFsDir = "/html";                                               // file directory for HTML page files
const String rebootReason = appDir + "/rebootreason.txt";                        // stores the reason when the app initiates a reboot
const String lastBootTimeFname = "/bootTime";                                   // UNIX time the server last booted

unsigned long rebootAskedforAt = 0;                                             // milliseconds reboot was asked for at
unsigned long lastRebootUpdate = 0;                                             // keep track of the milliseconds when a reboot update is done
unsigned long timeTimeSyncLost = 0;                                             // milliseconds since boot the the EZTime time sync was lost
int whereToSendLogEntryTo = BOOT_LOG_STR;                                       // where should the log entries get posted...always to BOOT_LOG_STR on boot
int MQTT_testmsgFreq = 60000;                                                   // MQTT test msg frequency time

bool rebooting = false;                                                         // are we counting down for a reboot
bool downloadingFile = false;                                                   // javascript 'fetch()' is downloading a file...need not to run any 'DallasTemperature' routines while downloading
bool doTx_debug = true;                                                         // turn on EasyTz debugging
bool bootChg = false;                                                           // have the boot wifi credentials been changed
bool tzChange = false;                                                          // has the desired timezone changed
bool startEzTime = false;                                                       // gets set to true when set up ezTime needs to be run
bool startMQTT = false;                                                         // gets set to true when the MQTT server needs to be connected to
bool mqttRunning = false;                                                       // mqtt is up and running
bool logToMQTTServer = false;                                                   // move temp logs to the MQTT server and start doing all logs to the MQTT server
bool mqttDownLogF = false;                                                      // when mqtt goes down this signals action to syslog writer
bool heapDebugging = false;                                                     // is the app in a heap debugging more
bool needBootTime = true;                                                       // need to get the get the servers boot time

size_t content_len = 0;                                                         // used in OTA updates

const String allDirectoriesToFile = dirLists + "/allDirs1Deep.txt";             // file to contains all the directories needed for update.html to work

AsyncWebServer server(80);                                                      // server port 80
Timezone myTZ;                                                                  // declare the ezTime TimeZone object;
AsyncWebSocket ws("/ws");                                                       // declare the asyncWebSocket object
timeStatus_t ezT_status = timeStatus();                                         // declare ezTime status and put the current status in it

int clientIdsInUse [MAX_NUM_CONNECTED_CLIENTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};// connected clients that are in use will be changed to a 1
int numOfConnectedClients = 0;                                                  // number of active connected clients
ConnectedClients connectedClients[MAX_NUM_CONNECTED_CLIENTS];                   // an array, MAX_NUM_CONNECTED_CLIENTS long, that hold connected client info
/*---------------    EZTIME - TIMESTATUS WORDS   ---------------*/

String getTimeStatusWords(int tStatus) {
  switch(tStatus) {
    case 0 : return F("timeNotSet");
      break;
    case 1 : return F("timeNeedsSync");
      break;
    case 2 : return F("timeSet");
      break;
    default : return("An unknown time status has been received, '" + String(tStatus) + "'.  Please advise the developer of this server.\n");
  }
}

/*---------------    IS TIME GOOD   ---------------*/

bool isTimeGood() {
  static unsigned long t = millis();                                            // init 't' to the current clock milliseconds
  if(millis() - t > 2000) {                                                     // if it has been more than 2 seconds since this last ran
    t = millis();                                                               // rest 't' tp the current milliseconds
    if(connectedSSID == "accessPt") return true;                                // if the server is providing an access point then just return true
    bool result = true;                                                         // create result and init it to true
    return result;
    t = millis();
  }
  return true;
}

/*---------------   GET WHERE SYSLOG IS BEING WRITTEN TO   ---------------*/

String syslogLocName(int x) {
  switch(x) {
    case BOOT_LOG_STR:
      return BOOT_LOG_FILE_NAME;
      break;
    case BOOT_LOG_FILE:
      return BOOT_LOG_FILE_NAME;
      break;
    case MQTT_SERVER:
      return MQTT_SERVER_NAME;
      break;
    case OFFLINE_DISK_FALLBACK:
      return OFFLINE_DISK_FALLBACK_NAME;
      break;
    default:
      mySP("The syslog is going to an unknown location, please case this to be fixed.\n", FN, LN);
      return String(x);
  }
}

/*---------------    COMMON FILE SYSTEM STARTUP ACTIONS   ---------------*/

void commonItemsFileSysStartup() {
  
  myFileSys.createDirIfNotExist(appDir);                                       // ensure each of the directories needed exist n the SD
  myFileSys.createDirIfNotExist(dirLists);                                     // make sure the dir list directory exists
  myFileSys.createDirIfNotExist(HTMLFsDir);                                    // make sure the html directory exists
  myFileSys.ensureFileExists(allDirectoriesToFile.c_str(), DIRS_IN_USE, LN);         // used by the update page to know what directories to advertise
  myFileSys.ensureFileExists(lastBootTimeFname.c_str(), "", LN);                     // if the boot file time file does not exist then create it as an empty file
  myFileSys.ensureFileExists(UPD_HTML_FILE.c_str(), "", LN);                         // if the firmware update file does not exist then log the action needed
  if(myFileSys.ensureFileExistsLazy(netInfoFname, []() {
    String out; LogInDataToStr(out, logInDataPtr, true);
    return out;}, LN) == FILE_ALREADY_EXISTS) {
      readNetInfoFromDisk(netInfoFname);
  }
}

/*---------------    DO ANY MYSTUFF2 SETUP TASKS   ---------------*/

void commonItemsStartup()  {
  TRACE();
  bool WiFiSuccess = false;
  #ifdef ESP32                                                                  // if this is running in an ESP32 set printProgress() up as the update progress routine
    Update.onProgress(printProgress);
  #endif
  mqttSetup(mqttHostPtr);
  wifi_setup();                                                                 // setup Wi-Fi
  WiFiSuccess = connToWiFi(logInD1.ssid, logInD1.ssidPwd, LN);                  // connect to wifi
  if(myFileSys.getFS().exists(rebootReason)) {                                  // if the file exists
    mySP("REASON FOR THE LAST REBOOT WAS: " + myFileSys.readFile(               // read it and write its contents to the log
         rebootReason.c_str(), false) + "\n", FN, LN);
    myFileSys.deleteFile(rebootReason.c_str(), LN);                             // and then delete the file
  } else mySP("ERROR: NO BOOT REASON FILE EXISTS.\n", FN, LN);
}

/*---------------    GET SERVER UP TIME   ---------------*/

String getServerUptime() {
  if(needBootTime) return msToDHMS(millis());                                   // if the server does not have the time yet just us milliseconds
  String s = myFileSys.readFile(lastBootTimeFname.c_str(),false);              // read the start time from disk
  time_t t = s.toInt();
  t = suli(myTZ.now(), t, LN, true);                                                      // get the elapsed seconds between boot and now
  return msToCommaSepDHMS(t * 1000, ',', true);                                 // convert the UNIX time in ms to DHMS
}

/*---------------    GET BOOT TIME SECONDS   ---------------*/

time_t getBootSeconds() {
  return myFileSys.readFile(lastBootTimeFname.c_str(),false).toInt();          // read and return the start time from disk
}

/*---------------    GET BOOT TIME   ---------------*/

void saveBootTime() {
  needBootTime = false;

  time_t bootT = myTZ.now() - (millis() / 1000);                                // get the actual boot time
  int success = myFileSys.writeFile(lastBootTimeFname.c_str(), String(bootT).c_str(), LN);
  
  mySP("Wrote the boot time of " + String(bootT) + " to the file " +
       lastBootTimeFname + ".\n", FN, LN);
}

/*---------------    DO COMMON ITEMS LOOP TASKS   ---------------*/

void commonItemsEvents() {
  static unsigned long t = millis();                                            // declare a static time var
  static unsigned long t2 = 0;                                                  // declare a static time var - #2]
  static runTime hbTimer = {30, 0, CT_SECOND};                                  // Static variable retains its state across loop iterations
  static uint32_t lastMqttConnectAttempt = LAST_MQTT_CONNECT_ATTEMPT;           // set to a -10 seconds to allow first pass to happen immediately on boot

  static runTime upTime = {1, -1, CT_MINUTE};                                   // update the run time every minute
  t = millis();                                                                 // put the current milliseconds in it every time through the loop
  
  if(needBootTime &&
       !booting &&                                    // Wait until MQTT connection steps settle completely
       myTZ.getPosix() != "UTC" &&
       myTZ.getPosix().length() > 0 &&
       timeStatus() == EZT_TIME_SET) {
      
      needBootTime = false;                           // Clear the flag IMMEDIATELY before starting the 136ms flash write
      mySP("SETTING BOOT EPOCH TIME.\n", FN, LN);
      saveBootTime();                                 // Safe to block for 136ms now!
    }

  if(WiFi.status() != WL_CONNECTED  && !AP_createdTime) {                       // if the wifi is disconnected try to reconnect it and the server is not in an Access Point mode
    mySP("WiFi.status() returned WL NOT CONNECTED\n", FN, LN);                  // log the issue
    connectToWiFi(LN);
  }
  
  if(isTimeToRun(&upTime)) {
    String str = msToCommaSepDHMS(millis(), ':', false);;
    sendWS_msg(FN, "uptime:" + str.substring(0, str.lastIndexOf(":")), NULL);   // send the current uptime to all the web clients
    mqttPublishMsg(serverUptime, 2, false,                                      // send the uptime to node red
                   str.substring(0, str.lastIndexOf(":")), LN);
    return;
  }
  
  if(AP_createdTime && suli(t, AP_createdTime, LN, true) > MAX_TIME_AS_ACCESS_PT) {       // if the ESP is acting like an access point and it has been doing this for longer than desired
    requestReboot(MAX_AP_TIME_REACHED, true);                                             // request the server reboot
    return;
  }
  
  if(!connectedToMQTT) {
    if (millis() - lastMqttConnectAttempt > MQTT_RETRY_DELAY_LEN) {
      lastMqttConnectAttempt = millis();
      mySP("MQTT Trigger Gate Active. Forwarding execution to connectToMqtt().\n", FN, LN);
      connectToMqtt();
      return; // Always drop out cleanly to prevent background code race conditions!
    }
  }

  // --- FIXED TIMING STATE MACHINE ---
  if(startEzTime) {
    setUpEZTime(logInD1.olsenTzName);
    
    // Clear the flag ONLY once setUpEZTime successfully binds our clock system.
    // This allows subsequent tasks down below to continue executing while network sync is pending!
    if (timeActive) {
      startEzTime = false;
    }
    return;
  }
  
  if(logToMQTTServer) {
    moveBootLogsToSyslog();
    mySP("********** The " + constData.appName + " is now fully up and running. **********\n", FN, LN);
    return;
  }
  
  if(tzChange) {                                                              // if a Tz change is needed
    static unsigned long lastTzAttempt = 0;
    if (suli(millis(), lastTzAttempt, LN, true) > 10000) {                    // if 10 seconds since the last time
      lastTzAttempt = millis();                                               // remember when
      
      mySP("tzChange is TRUE. Executing timezone update attempt.\n", FN, LN); // log the attempt
      setUpEZTime(logInD1.olsenTzName);                                       // set the time
      return;                                                                 // Drops out cleanly, but ONLY once every 10 seconds!
    }                                                                         // bad timimg and going on to other things
  }
  if (timeStatus() != EZT_TIME_SET && connectedSSID != "accessPt") {            // if the EZTime admits the time is not currently set
    mySP("EZTime status has been found to be '" + String(timeStatus())          // log the issue
         + "' instead of 'timeSet'. Initiating new EZTime setup.\n", FN, LN);
    setUpEZTime(logInD1.olsenTzName);                                           // and attempt to reset it
    return;                                                                     // only do one thing so the watchdog timer does not get tripped
  }
  
  if (rebooting) {                                                              // if the ESP is getting ready to reboot
    if (t - lastRebootUpdate > 1000) {                                          // if it has been 1 second since last time
      serverRebootInProgress();                                                 // call serverRebootInProgress to do any needed updates
      return;                                                                   // only do one thing so the watchdog timer does not get tripped
    }
  }
  if(sendNrAliveMsgs && suli(millis(),lastNrAliveMsgSent, LN, true) > MQTT_testmsgFreq) { // if MQTT alive test msg are being sent and it is time to do one then
    sendMqttAliveMsg();                                                         // send an alive msg
    lastNrAliveMsgSent = millis();                                              // remember the last time this ran
    return;                                                                     // only do one thing so the watchdog timer does not get tripped
  }
  
  if(sendNrAliveMsgs && suli(millis(),lastMqttAliveMsgRcv, LN, true) > aliveMsgWaitTimeout) {      // if it has been too long since the last alive msg from mode red
    hdlOverdueNR_aliveMsg();                                                    // handle the issue
    return;                                                                     // only do one thing so the watchdog timer does not get tripped
  }
  if(connectedToMQTT && mqttHbActive && isTimeToRun(&hbTimer)) {
    runMqttHeartbeat();
  }
    
  events();                                                                     // call the EzTime background polling loop
}

/*---------------    PRINT DASHED SEPARATOR LINE   ID 103---------------*/

String pdl(String callingFunct, int origLineNo)  {
  return callingFunct +":" + "LINE:" + String(origLineNo) + " ----------------------------------------------\n";
}

/*---------------    CUSTOMIZED INFO DISSEMINATION SEND A MESSAGE TO THE WEB PAGES CONSOLE  ---------------*
   Requires a ESPAsyncWebServer AsyncWebSocket 'ws' variable exist
   if it is desired to just have the incoming 's' sent to the serial port, 'who' should be -1 and line number may be anything.
   if it is desired to be sent to the serial port and all the connected web pages then who should be defined in the 'getSendToConsoleWho()'
   function and 'lineNo should be the line number of the callomh code line number.  This is usually 'LN'.
   if not who = -1 and 'NOTIME' does not exist in the incoming 's', then 's' is written to the serial port and all connected web pages with
   out any current time information.

*/

void sendWS_msg(String callingF_name, String s, AsyncWebSocketClient *client) {
  if (client != NULL) {                                                 // if the client is not null then
    client->text(s);                                                    // send it to the client
  } else {
    ws.textAll(s);                                                      // else send it to all the clients
  }
}

/*---------------  DUMP OFFLINE RAM LOGS TO MQTT HOST  ---------------*/

void dumpRamLogsToMqtt() {
  if (!connectedToMQTT) return; // Guard clause: abort if network isn't ready
  
  mySP("MQTT Link recovered! Extracting offline RAM cache memory to server...\n", FN, LN);

  // 1. If the wheel has wrapped, read from the current index pointer to the end first (oldest data)
  if (ramLogWrapped) {
    for (int i = ramLogIndex; i < MAX_RAM_LINES; i++) {
      if (ramLog[i][0] != '\0') {
        mqttPublishMsg(MQTT_IN_SYSLOG, 0, false, ramLog[i], LN);        delay(5); // Tiny 5ms delay to prevent flooding your network TX buffer arrays
      }
    }
  }

  // 2. Then read from slot 0 up to the current index pointer (newest data)
  for (int i = 0; i < ramLogIndex; i++) {
    if (ramLog[i][0] != '\0') {
      mqttPublishMsg(MQTT_IN_SYSLOG, 0, false, ramLog[i], LN);      delay(5);
    }
  }

  // 3. Clear the slate completely so we don't send duplicate reports later
  ramLogIndex = 0;
  ramLogWrapped = false;
  memset(ramLog, 0, MAX_RAM_LINES * MAX_LOG_LINE_LEN);      // Fast zero out memory dump
  mySP("RAM cache successfully cleared. System synchronized.\n", FN, LN);
}

/*---------------    CUSTOMIZED SERIAL.PRINT   ID 102---------------*/

void recordToRamLog(String s) {
  // Bounded safe copy: extracts string data directly into our static row array slot
  snprintf(ramLog[ramLogIndex], MAX_LOG_LINE_LEN, "%s", s.c_str());
  
  ramLogIndex++; // Move the pointer to the next slot row
  
  // If the hand hits the top of the clock, rotate back to the zero slot
  if (ramLogIndex >= MAX_RAM_LINES) {
    ramLogIndex = 0;
    ramLogWrapped = true; // The wheel has full history now
  }
}

/*---------------    GET LOG TIME STAMP  ---------------*/

String getLogTimeStamp() {
  if (!timeActive)  {                             // if not 'timeActive' then
    return String(millis()) + "ms";               // put the milliseconds into 't'
  } else return MY_TIME_F;                        // else put the current time into 't'
 
}
   
/*---------------    FORMAT LOG ENTRY FOR THE SERIAL MONITOR  ---------------*/
   
String doSerialMonLogEntry(String event, String functionName, int lineNo) {
  return getLogTimeStamp()+                     //return the log entry
         ", Line#:" + String(lineNo)
         + ", FROM: " + functionName +
         ": " + event;
}

/*---------------    FORMAT LOG ENTRY AS JSON STRING  ---------------*/

String doJsonStrLogEntry(String event, String functionName, int lineNo) {
  static uint32_t devSeq = 1;
  
  JsonDocument doc;
  
  doc["deviceSequence"] = devSeq++;                    // load in the sequence number
  doc["eventTime"]      = getLogTimeStamp();           // load the now time stamp
  doc["component"]      = constData.appShortName;      // load the app short name
  doc["function"]       = functionName;                // load the function name
  doc["line"]           = lineNo;                      // load the code line number
  doc["message"]        = event;                       // load the event

  String json;
  serializeJson(doc, json);
  return json + "\n";
}

/*---------------    CUSTOMIZED SERIAL.PRINT   ID 102---------------*/

void mySP(String event, String functionName, int lineNo) {
  Serial.print(doSerialMonLogEntry(event, functionName, lineNo));
  sendToSyslog(doJsonStrLogEntry(event, functionName, lineNo));
}

/*---------------    CUSTOMIZED SERIAL.PRINT   ID 102---------------*

void mySP(String s, String who, int lineNo) {
  if(who == "-1" && lineNo == -1) {
    Serial.print(s);                                                  // send 's' to the serial monitor
    sendToSyslog(s);
    return;
  }
  if (who == "-1") {                                                  // if just the incoming string is wanted to be sent to the serial port
    Serial.print(s);                                                  // send 's' to the serial monitor
    return;
  }
  String t = "";                                                      // declare a string to hold the time
  if (!timeActive)  {                                                 // if not 'timeActive' then
    t = String(millis()) + "ms";                                      // put the milliseconds into 't'
  } else t = MY_TIME_F;                                               // else put the current time into 't'
  s = t + ", Line#:" + String(lineNo) + ", FROM: " + who + ": " + s;  // build the string to send
  Serial.print(s);                                                    // send it to the Serial monitor
  sendToSyslog(s);                                                    // send it to the syslog
}

/*---------------    WRITE TO SYSLOG   ID 141---------------*/

void sendToSyslog(String s) {
  switch(whereToSendLogEntryTo) {
    case BOOT_LOG_STR:
      startUpBootLogStr += s;
      break;
      
    case BOOT_LOG_FILE:
      myFileSys.appendFile(tmpBootSyslog.c_str(), s.c_str());
      break;
      
    case MQTT_SERVER:
      if (connectedToMQTT) {
        mqttPublishMsg(MQTT_IN_SYSLOG, 0, false, s, LN);
      } else {
        // 🛡️ NETWORK DROP FALLBACK: If MQTT drops, roll it into RAM instead of flash disk!
        recordToRamLog(s);
      }
      break;
    case OFFLINE_DISK_FALLBACK: // Your name-corrected network outage fallback slot
      // 🛡️ NO-DISK SHIELD: Divert high-traffic offline logs straight to RAM
      recordToRamLog(s);
      break;
      
    default:
      Serial.print("Received an unknown code for where to send the syslog entry. Code: " + String(whereToSendLogEntryTo) + "\n");
      break;
  }
}

/*---------------  BEGIN SERIAL  ID 103---------------*/

void beginSerialConnection()  {
  Serial.begin(115200);                                                         // set the serial port speed
  pdl(FN, LN);                                                                  // print a separator line
  Serial.println("");                                                           // move to a new line in the Serial monitor window
  mySP("Serial started at 115200\n", FN, LN);                                   // announce the serial process has been started
  pdl(FN, LN);                                                                  // print a separator line
}

/*---------------  MOVE THE LOGS THAT HAVE BEEN COLLECTED SINCE BOOT TO THE SYSLOG  ---------------*/

void moveBootLogsToSyslog() {
  if (!myFileSys.getFS().exists(tmpBootSyslog)) {
    mySP("ERROR:There was no '" + tmpBootSyslog +
              "' file to move to today's " +                                                  // log there was no temp boot file to work with
              "syslog file. This was n=unexpected.\n", FN, LN);
    return;
  }
  File f = myFileSys.getFS().open(tmpBootSyslog, "r");
                                                                                // if the tmpBootSyslog.txt' does exists
  if (!f) {
    mySP("Failed to open '" + tmpBootSyslog + "'.\n", FN, LN);
    logToMQTTServer = false;
    return;
  }
  int size = f.size();
  while(f.available()) {
    String line = f.readStringUntil('\n');
    if (line.length() == 0) continue;
    Serial.println(line);
    mqttPublishMsg(MQTT_IN_SYSLOG, 1, false, line, LN);
    delay(5);
  }
  f.close();
  whereToSendLogEntryTo = MQTT_SERVER;                                                          // the boot log file has been written to the MQTT server so start sending all logs there
  mySP("Sending logs to: " + syslogLocName(whereToSendLogEntryTo) + "\n", FN, LN);              // log where syslog files are going
  mqttDownLogF = false;                                                                         // mqtt is no longer down so make this false
  mySP("Sent "+String(size)+" bytes from "+tmpBootSyslog+" To the MQTT server log\n",FN,LN);    // log the action
  myFileSys.deleteFile(tmpBootSyslog.c_str(), LN);                                              // delete the temp boot file
  logToMQTTServer = false;
}

/*
 if (myFileSys.getFS().exists(tmpBootSyslog)) {                                                            // if the tmpBootSyslog.txt' does exists
   String s = myFileSys.readFile(tmpBootSyslog.c_str(), false);                                   // pull the data out of the temp boot file
   if (s != "")  {                                                                                 // if 's' is not an empty string
     int size = myFileSys.getFileSize(tmpBootSyslog.c_str());                                       // get the file size of tmpBootSyslog
     mqttPublishMsg(MQTT_IN_SYSLOG, 1, false, s, LN);                                              // send the file contents to the MQTT server
     whereToSendLogEntryTo = MQTT_SERVER;                                                          // the boot log file has been written to the MQTT server so start sending all logs there
     mySP("Sending logs to: " + syslogLocName(whereToSendLogEntryTo) + "\n", FN, LN);              // log where syslog files are going
     mqttDownLogF = false;                                                                         // mqtt is no longer down so make this false
     mySP("Sent "+String(size)+" bytes from "+tmpBootSyslog+" To the MQTT server log\n",FN,LN);      // log the action
     myFileSys.deleteFile(tmpBootSyslog.c_str(), LN);                                             // delete the temp boot file
   } else mySP("Failed to move the boot log into the syslog file.\n", FN, LN);                     // else the process failed
 } else mySP("There was no '" + tmpBootSyslog + "' file to move to today's " +                     // log there was no temp boot file to work with
             "syslog file.\n", FN, LN);
 logToMQTTServer = false;

 */
/*---------------  SETUP EZTIME TO GIVE US TIME  ID 107---------------*/

void setUpEZTime(String olsenName) {
  static time_t lastPass = 0;
  // Quickly skip if called too soon (at most once every 5 seconds)
  if(suli(millis(), lastPass, LN, true) < 5000) return;
  lastPass = millis();
  
  if (timeStatus() != timeSet) {                                                // if the timeStatus does not match timeSet
    mySP("Waiting for background NTP sync packet...\n", FN, LN);                // the system is waiting to receive a NTP packet
    updateNTP();                                                                // fires a UDP data packet at the NTP server and returns instantly
    return;                                                                     // and go around again
  }
  
  timeActive = true;                                                            // NTP IS SET so Evaluate the timezone resolution pipeline
  
  // ==========================================================================
  // PIPELINE STATE 1: LOCAL POSIX RULE MATCH
  // ==========================================================================
  if (logInD1.posixTzRule.length() > 0) {                                       // there is a local profile so use it
    mySP("NTP synced. Applying local POSIX rule instantly: " +                  // log the data
         logInD1.posixTzRule + "\n", FN, LN);
    bool tzSet = myTZ.setPosix(logInD1.posixTzRule);                            // set the tz using the POSIX data
    if (tzSet) {                                                                // if the call succeeded
      mySP("Successfully locked local timezone via POSIX rule.\n", FN, LN);     // log this
      
      // --- LANDING ZONE 1: POSIX SUCCESS ---
      startEzTime = false;                                                      // don't need to come back so turn the flag off
      tzChange = false;                                                         // consume user requested timezone change token successfully
      
      doAppTimeIsSetActions();                                                  // if the app has anything it needs to do when the time is set call them
      return;                                                                   // all finished so return
    }
  }
  
  // ==========================================================================
   // PIPELINE STATE 2: OLSON LOOKUP FALLBACK
   // ==========================================================================
   if (olsenName.length() > 0) {
     mySP("POSIX blank. Resolving timezone via Olson Name: " + olsenName + "\n", FN, LN);
     
     if (myTZ.setLocation(olsenName)) {
       String freshPOSIX = myTZ.getPosix(); // Capture what the server sent back
       
       // 🛡️ FLASH PRESERVATION: Only write if the rule is actually different!
       if (logInD1.posixTzRule != freshPOSIX) {
         logInD1.posixTzRule = freshPOSIX;
         writeNetInfoToDisk(netInfoFname);
         mySP("POSIX changed! Committed updated JSON payload directly to /netInfo.txt\n", FN, LN);
       } else {
         mySP("POSIX rule matches current disk file. Skipping redundant flash write.\n", FN, LN);
       }
       
       // --- SUCCESS: Turn the flags off permanently! ---
       startEzTime = false;
       tzChange = false;
       
       doAppTimeIsSetActions();
       return;
     } else {
       mySP("Olson lookup failed. Retrying in 10 seconds via loop timer...\n", FN, LN);
       return;
     }
   }
  // ==========================================================================
  // PIPELINE STATE 3: HARD FAILURE fallback
  // ==========================================================================
  mySP("WARNING: Timezone profile missing or corrupt. Forcing Central Time fallback.\n", FN, LN); // log the issue
  logInD1.posixTzRule = "CST6CDT,M3.2.0,M11.1.0";                               // Both POSIX and Olson are missing or totally corrupt - using default settings
  myTZ.setPosix(logInD1.posixTzRule);                                           // set the time
  
  // --- LANDING ZONE 3: FALLBACK SUCCESS ---
  startEzTime = false;                                                          // don't need to come back so turn the flag off
  tzChange = false;                                                             // consume user requested timezone change token successfully
  
  doAppTimeIsSetActions();                                                      // if the app has anything it needs to do when the time is set call them
}

/*---------------  IS IT TIME TO RUN A PROCESS  AT A DA, HOUR, MINUTE, OR SECOND INTERVAL  ---------------*/

bool isTimeToRun(runTimePtr when) {
  int d = DAY.toInt();
  int h = HOUR.toInt();                                       // get the current hour
  int m = MINUTE.toInt();                                     // get the current minute and convert it to an int
  int s = SEC.toInt();                                        // get the current second
  int t = (h*60) + m;                                         // minutes since midnight
  int sd = (m * 60) + s;                                      // number of seconds elapsed in the hour
  switch(when->type){
    case CT_DAY:
      if(d % when->intvToRun == 0 && d != when->lastRun) {    // if this is the day interval and this day has not been done
        when->lastRun = d;
        return true;
      }
      break;
    case CT_HOUR:
      if(h % when->intvToRun == 0 && h != when->lastRun) {    // if this is the hour interval and this hour has not been done
        when->lastRun = h;
        return true;
      }
      break;
    case CT_MINUTE:
      if(t % when->intvToRun == 0 && t != when->lastRun) {    // if this is the minute interval and this minute has not been done
        when->lastRun = t;
        return true;
      }
      break;
    case CT_SECOND:
      if(sd % when->intvToRun == 0 && sd != when->lastRun) {  // if this is the second interval and this second has not been done
        when->lastRun = sd;
        return true;
      }
      break;
  }
  return false;                                               // if it made it this far then return false
}

/*---------------  MINUTE STRING TO INT  ---------------*/

int minuteToInt() {
  String min =  MINUTE;                                                         // get the clock minute time field
  return min.toInt();                                                           // convert it to an integer and return it
}

/*---------------  SECOND STRING TO INT  ---------------*/

int secondToInt() {
  String sec =  SEC;                                                            // get the clock minute time field
  return sec.toInt();                                                           // convert it to an integer and return it
}

/*---------------    GET LOG TIME   ID 143---------------*/

String getLogTime() {
  if (timeActive)  {                                                            // if time is active
    return MY_TIME_F;                                                           // return the current formatted time
  } else return String(millis());                                               // else return milliseconds since boot

}

/*---------------  CONVERT MILLISECONDS TO COMMA SEPARATED HOURS, MINUTES, AND SECONDS  ---------------*/

String msToCommaSepDHMS(unsigned long ms, char separator, bool pretty) {
  String theDHMS = "";
  String d = String(ms / 86400000);
  String h = pad(String((ms / 3600000) % 24), '0', 2);
  String m = pad(String((ms / 60000) % 60), '0', 2);
  String s = pad(String((ms / 1000) % 60), '0', 2);
  if(pretty) {
    return d + " days, " + h + ":" + m + ":" + s;
  } else return d + separator + h + separator + m + separator + s;
}

/*---------------  CONVERT MILLISECONDS TO HOURS, MINUTES, AND SECONDS  ID 107---------------*/

String msToDHMS(unsigned long ms)  {       // milliseconds to days, hours, minutes, seconds
  String d = String(ms / 86400000 );
  String h = String((ms / 3600000) % 24 );
  String m = String((ms / 60000) % 60);
  String s = String((ms / 1000) % 60);
  String theDHMS = d + " Days " + h + " Hours " + m + " Minutes " + s + " Seconds";
  return theDHMS;
}

/*---------------    INITIALIZE THE WEB SOCKET   ---------------*/

void startWebServer() {
  ws.onEvent(webSocketEvent);                                                   // set the function that will handle the incoming websockewt events
  server.addHandler(&ws);                                                       // register the websocket object on our HTTP web server. As input, this method will receive the address of our AsyncWebSocket object.
  webServerActions();                                                           // add a route to serve the HTML / JavaScript file for each page
  server.begin();                                                               // Start the web server
  mySP("Web server started.\n", FN, LN);
}

 /*---------------  REBOOT ASKED FOR  ---------------*/

void requestReboot(String reason, bool immediately) {
  mySP(reason + "\n", FN, LN);                                                  // log this
  myFileSys.writeFile(rebootReason.c_str(), reason.c_str(), LN);               // write the reboot reason to disk
  if(immediately) {
    mySP("Rebooting...\n", FN, LN);                       // log the server is rebooting now
    Serial.flush();                                       // flush the serial buffer
    ESP.restart();                                        // restart the server
  } else restartESP32();                                                               // reboot the ESP32
}


/*---------------  REBOOTS THE ESP32  ---------------*/

void restartESP32() {
  digitalWrite(DEVICE_IS_RUNNING, HIGH);                  // on-board LEDS are inverted...LOW is on, HIGH is off
  rebootAskedforAt = millis();                            // init 'rebootAskedforAt' to now
  lastRebootUpdate = rebootAskedforAt;                    // init 'lastRebootUpdate' to now
  rebooting = true;                                       // change 'rebooting' to true
  doServerRebootActions();                                // let the server do any desired reboot actions
  mySP("Restarting server 5 seconds\n", FN, LN);          // log a reboot has been started
}

/*---------------  SERVER REBOOT IN PROGRESS  ---------------*/

void serverRebootInProgress() {
  if (millis() - rebootAskedforAt > 5000)  {          // if it has been 5 seconds since the reboot was started
    mySP("Rebooting...\n", FN, LN);                   // log the server is rebooting now
    Serial.flush();                                   // flush the serial buffer
    ESP.restart();                                    // restart the server
  } else {                                            // else, still under 5 seconds
    mySP(".", "-1", LN);                              // send a '.' to the log
    lastRebootUpdate = millis();                      // update the last server reboot update time
  }
}

/*---------------  SERVER PAGE NOT FOUND  ---------------*/

void notFound(AsyncWebServerRequest *request, String appName) {
  IPAddress ip = request->client()->remoteIP();                                                   // capture the requesters ip address
  String requested_file = request->url();                                                         // capture the URL being requested
  if (requested_file.indexOf("/sendfile:")) {                                                     // if the url contains 'sendfile:' then
    requested_file = requested_file.substring(requested_file.indexOf(":") + 1);                   // strip it off
  }
  if(myFileSys.getFS().exists(requested_file)) {
    mySP("received a request for '" + requested_file + "'.\n", FN, LN);                           // log a request for a file has been received
    downloadingFile = true;                                                                       // tell the server that a download is in process
    mySP("sending web page code file '"+requested_file+"' to '"+ip.toString() + "'.\n", FN, LN);  // log what is happening
    File f = myFileSys.getFS().open(requested_file, "r");
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain",                // begin sending the file in a chuncked response mode
    [f](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      //Write up to "maxLen" bytes into "buffer" and return the amount written.
      //index equals the amount of bytes that have been already sent
      //You will be asked for more data until 0 is returned
      //Keep in mind that you can not delay or yield waiting for more data!
      return load_data(f, buffer, maxLen, index);
    });
    response->addHeader("Server", "ESP Async Web Server");                                        // add a header to the response message
    request->send(response);                                                                      // send the response message
  } else {                                                                                        // else this is a page not found so
    request->send(404, "text/plain", "ERROR:404 - " + appName + " says page not found.");         // tell the browser the page was not found
  }
}

/*---------------  REBOOT ESP8266  ---------------*/

void restartESP() {
  mySP("Flushing serial buffers before reboot.\n", FN, LN);
  Serial.flush();                                   // flush the serial buffer
  ESP.restart();                                    // restart the server
}

/*---------------  SUBTRACT UNSIGNED LONG INTS  ---------------*/

unsigned long suli(unsigned long minuend, unsigned long subtrahend, int from, bool logErr) {
  if(subtrahend > minuend)
    if(subtrahend > minuend) {
      if(logErr)
        mySP("From: LINE " + String(from) + " 'suli' subtrahend: " + String(subtrahend) +
             "' is greater than minuend: '" + String(minuend) + "'\n", FN, LN);
      return 0;
    }
  return minuend - subtrahend;
}

/*---------------  SUBTRACT UNSIGNED LONG INTS  ---------------*  REPLACED WITH SULI

unsigned long subUnsignedLongInt(unsigned long minuend, unsigned long subtrahend) {
  if(subtrahend > minuend) return 0;
  return minuend - subtrahend;
}

  /*---------------  WHAT IS THE PINS VALUE, GET TWO OUT OF THREE SAME READINGS  ---------------*/

bool twoOfThree(int pin)  {
  int results1 = 0;
  int results0 = 0;
  if (digitalRead(pin)) {                                       // read the pin and if it is a 1
    results1 += 1;                                              // increment results1 by one
  } else results0 += 1;                                         // else put increment results0
  delay(20);                                                    // wait 20 milliseconds
  if (digitalRead(pin)) {                                       // read the pin again and if it is a 1
    results1 += 1;                                              // increment results1 by one
  } else results0 += 1;                                         // else put increment results0
  delay(20);                                                    // wait 20 milliseconds
  if (digitalRead(pin)) {                                       // read the pin again and if it is a 1
    results1 += 1;                                              // increment results1 by one
  } else results0 += 1;                                         // else put increment results0
  if (false) {                                                   // if debugging
    mySP("Best 2 out of 3: results = 1:  " + String(results1) + // log the results
         ", results = 0: " + String(results0) + "\n", FN, LN);
  }
  if (results1 > results0) {                                    // if results1 is greater than results0 then
    return true;                                                // return true
  } else {
    return false;                                              // else return false
  }
}

/*--------  DUMP TM STRUCT  --------*/

void dumpTM_struct(tmElements_t *tm) {
  Serial.println("DUMPING TM RECORD:");
  Serial.println("    Second: " + String(tm->Second));
  Serial.println("    Minute: " + String(tm->Minute));
  Serial.println("    Hour: " + String(tm->Hour));
  Serial.println("    Wday: " + String(tm->Wday));
  Serial.println("    Day: " + String(tm->Day));
  Serial.println("    Month: " + String(tm->Month));
  Serial.println("    Year: " + String(tm->Year));
}

/*--------  CONVERT TIME ELEMENTS STRUCT TO UNIX TIME SECONDS  --------*/

time_t localTimeToUNIX_secs() {
  tmElements_t tm;                                                              // define the time struct to be used
  tm.Second = SEC.toInt();                                                      // populate Seconds, Minutes, Hours Day,
  tm.Minute = MINUTE.toInt();                                                   // Month, and Year using EzTime functions to get the time
  tm.Hour = HOUR.toInt();
  tm.Day = DAY.toInt();
  tm.Month = MONTH.toInt();
  tm.Year = YEAR.toInt() - 1970;                                                // load the 'tm' struct with the year, month, and day of the file
  return makeTime(tm);                                                          // return thee time in UNIX seconds
}

  /*--------  CONVERT UNIX TIME SECONDS TO HUMAN TIME  --------*/

String unixT_toHumanT(time_t t) {
  tmElements_t tm;                      // define the time struct to be used
  pdl(FN, LN);
  pdl(FN, LN);
  breakTime(t, tm);
  return String(tm.Year + 1970) + "/" +
         String(tm.Month) + "/" +
         String(tm.Day) + " @ " +
         String(tm.Hour) + ":" +
         String(tm.Minute) + ":" +
         String(tm.Second) +
         myTZ.dateTime(t, " A T");
}

/*--------  CHECK THE HEAP  --------*/

void checkTheHeap(bool force) {
  static runTime isTimeInfo = {CK_HEAP_INTERVAL, -1, CT_MINUTE};                          // store when to run
  if(isTimeToRun(&isTimeInfo) || force) {                                                 // find out if it is time to run and if it is then
    long theHeap = ESP.getFreeHeap();                                                     // get the free heap memory
    mySP("[MAIN] Free heap: " + String(theHeap) +  ": bytes, previous free heap was: " +  // send the info to the consoles
         String(lastFreeHeap) + "\n", FN, LN);
    lastFreeHeap = theHeap;                                                               // remember what the free heap size was
  }
}

/*--------------------------------------   HANDLE CLIENT LISTS  --------------------------------------*/

/*---------------  DECREMENT A CLIENT FROM THE LIST WHEN THEY LEAVE  ID 115---------------*
   When a client leaves, decrement them from the list
*

bool decrementClientFromList(int clientNum) {
  
  int aPos = findClientIndex(clientNum, true);
  if (aPos != -1) {
    mySP("Client " + String(clientNum) + " removed from Client List. " + // log the new web page has disconnected
         ", Type :'" + connectedClients[aPos].type +
         "', IP Address:'" + connectedClients[aPos].ipAddr +
         "', CUID:'" + connectedClients[aPos].CUID +
         "'\n", FN, LN);
    numOfConnectedClients --;                                             // decrement the number of connected clients
    connectedClients[aPos].id = 0;                                        // erase the old clients num
    connectedClients[aPos].ipAddr = "";                                   // and ip address
    connectedClients[aPos].type = "";                                     // and type
    connectedClients[aPos].CUID = "";                                     // and type
    clientIdsInUse[aPos] = 0;                                             // tell clientIdsInUse this ID is no longer in use
    return true;
  } else {                                                                  // else failed to fimd the record
    printListOfConnectedClients();
    mySP("ERROR 102: Failed to delete client id:'" + String(clientNum) +  // log this info
         "'s connectedClients record. Please report this to the developer.\n"
         , FN, LN);
    return false;
  }
}

/*---------------  ADD A NEW CLIENT TO THE LIST  ---------------*

bool addNewClientToList(AsyncWebSocketClient * client, String type, String CUID)  {
  int aPos = getUnusedClientIndex();                                  // get the first unused array position
  if (aPos == -1)  {
    sendWS_msg(FN, "ERROR 101:To many clients are connected.  Please try later.", client);
    return false;
  }
  numOfConnectedClients ++;                                       // increment the number of connected clients
  connectedClients[aPos].id = (int)client->id();                  // remember the websocket assigned client number
  connectedClients[aPos].ipAddr = client->remoteIP().toString();  // remember its ip address.
  connectedClients[aPos].type = type;                             // type of web page, main, wifi
  connectedClients[aPos].CUID = CUID;                             // record the clients unique id
  connectedClients[aPos].logTail = false;                         // record the client is not tailing a file
  connectedClients[aPos].client = client;                         // record the clients 'client' object
  clientIdsInUse[aPos] = 1;                                       // update the array of connected client IDs in use
  return true;
}

/*---------------  PRINT A LIST OF THE CONNECTED CLIENTS  ID 116---------------*
   returns a list of all the connected clients (web pages)
*

String printListOfConnectedClients() {
  if (false) {
    mySP("In printListOfConnectedClients()\n", FN, LN);
  }
  String s = "";
  for (int i = 0; i < MAX_NUM_CONNECTED_CLIENTS; i++)  {        // for each possible connected client
    s = s + String(clientIdsInUse[i]) + ",";                    // assemble an array of the clients in use
  }
  s = s.substring(0, s.lastIndexOf(","));                       // delete the last comma
  s = "ClientIdsInUse: " + s + "\n";                            // put this into 's'
  for (int i = 0; i < MAX_NUM_CONNECTED_CLIENTS; i++)  {        // for each possible client
    if (clientIdsInUse[i]) {                                    // if the array index is in use
      s += "  Client num: " +                                   // add the information on it to 's'
           String(connectedClients[i].id) +
           ", " + connectedClients[i].type +
           ", IP Address: " + connectedClients[i].ipAddr +
           ", CUID: " + connectedClients[i].CUID +
           ", tail log: " + connectedClients[i].logTail +
           ", client->id = " + (connectedClients[i].client ? String(connectedClients[i].client->id()) : "Not in use") + "\n";
    }
  }
  //  mySP(s, FN, LN);                                             // log 's'
  s = s + "\n";
  return s;                                                // return 's' to the calling function
}

/*---------------  GET AN UNUSED POSITION IN clientIdsInUse ARRAY  ID 117---------------*

int getUnusedClientIndex() {
  for (int i = 0; i < MAX_NUM_CONNECTED_CLIENTS; i++) {   // for each avaiable position in the clientIdsInUse
    if (clientIdsInUse[i] == 0) return i;                 // if the position is unused, return 'i'
  }
  return -1;                                              // if an unused one was not foound then MAX_NUM_CONNECTED_CLIENTS are connected
}

/*---------------  FIND CLIENTS BY TYPE ---------------*

String findClientsByType(String type) {
  String s = "";
  for (int i = 0; i < MAX_NUM_CONNECTED_CLIENTS; i++) {   // for each avaiable position in the clientIdsInUse
    if(connectedClients[i].type == type) {
      s += "1";
    } else s += "0";
  }
  return s;
}


/*---------------  FIND A CLIENTS clientIdsInUse ARRAY POSITION  ID 118---------------*

int findClientIndex(int id, bool logErr) {
  for (int i = 0; i < MAX_NUM_CONNECTED_CLIENTS; i++) {                                                           // for each avaiable position in the clientIdsInUse
    if (connectedClients[i].id == id) return i;                                                                   // if this is the right one then return i
  }
  if (logErr) {                                                                                                   // if this error is not expected
    mySP("ERROR 103: Failed to locate the connectedClients record for client id: " + String(id) + "\n", FN, LN);  // log it
    printListOfConnectedClients();                                                                                // print the list of connected clients
  }
  return -1;                                                                                                      // failed to find id, return -1
}

/*---------------  FIND A CLIENTS BY CUID  ID 121---------------*

int findClientByCUID(String CUID, bool adviseOnResults) {
  for (int i = 0; i < MAX_NUM_CONNECTED_CLIENTS; i++) {   // for each avaiable position in the clientIdsInUse
    if (connectedClients[i].CUID == CUID) {
      mySP("Client CUID: " + CUID + " found.  Client id is " + String(connectedClients[i].id) + "\n", FN, LN);
      return i;        // if this is the right one then return i
    }
  }
  if (adviseOnResults) {
    mySP("ERROR 104: Failed to locate by CUID the connectedClients record for client id: " + CUID + "\n", FN, LN);
  }
  return -1;                                            // failed to find id, return -1
}

/*---------------  GATHER WIFI SETUP DATA  ---------------*/

 void gatherWiFiSetupData(String &jsonOutput, MQTT_hostPtr mqttPtr) {
   // Allocate our clean ArduinoJson v7 document
   JsonDocument doc;

   // 1. Pack Wi-Fi Data Keys straight from your live 'logInD1' struct
   doc["ssid"]       = logInD1.ssid;
   doc["pass"]       = logInD1.ssidPwd;
   doc["tz"]         = logInD1.tzAbbrv;
   doc["olson"]      = logInD1.olsenTzName;
   doc["posix"]      = logInD1.posixTzRule;

   // 2. Pack MQTT Data Keys straight from your 'mqttPtr' struct
   doc["mqttServer"] = mqttPtr->host.toString();
   doc["mqttPort"]   = mqttPtr->portNo;
   doc["mqttUser"]   = mqttPtr->brokerUser;
   doc["mqttPass"]   = mqttPtr->brokerPwd;

   // 3. Serialize the object directly into a safe local buffer (No fragmentation)
   char buffer[256];
   serializeJson(doc, buffer, sizeof(buffer));

   // Drop the finished text directly into your passed-in variable
   jsonOutput = buffer;
 }
 
/*---------------  GATHER WIFI SETUP DATA  ---------------*

String gatherWiFiSetupData(MQTT_hostPtr mqttPtr) {
  return logInD1.ssid + "|" +                                                   // assemble and return the data required for
         logInD1.ssidPwd + "|" +                                                // initing the WiFi setup page
         mqttPtr->host.toString() + "|" +
         String(mqttPtr->portNo) + "|" +
         mqttPtr->brokerUser + "|" +
         mqttPtr->brokerPwd;
}

/*---------------    HANDLE A NEW WIFI SETUP WEB PAGE   ---------------*/

void initNewWiFiPg(String s, AsyncWebSocketClient *client, const_dataPtr constDataPtr, MQTT_hostPtr mqttPtr) {
  String outboundData = "";
  String CUID = s.substring(s.lastIndexOf(":") + 1);                                              // split the clients UID out of the string
  String ip = client->remoteIP().toString();                                                      // get its IP address
  mySP("A new WiFi setup web page has joined.  ClientId:'"+String(client->id())+                  // log the new web page that joined
              "', Type :'WiFi setup', IP Address:'"+ip+"', CUID:'"+CUID+"'\n", FN, LN);
  sendWS_msg(FN, "serverip:" + srvIPAddr, client);                                                // send the client the servers ip address
  sendWS_msg(FN, "clientip:" + ip, client);                                                       // send the client its ip address
  sendWS_msg(FN, "pgHeader:" + constDataPtr->pageHeader, client);                                 // send the page header
  sendWS_msg(FN, "listDir:" + myFileSys.readFile(allDirectoriesToFile.c_str(), true), client);
  gatherWiFiSetupData(outboundData, mqttPtr);
  sendWS_msg(FN, "initWebDataJSON:" + outboundData, client);                                      // send the web page the info needed to setup the data fields
 }

/*---------------    HANDLE GETTING THE WIFI SETUP WEB PAGE   ---------------*/

void hdlWiFiSetupEvent(String s, AsyncWebSocketClient *client, MQTT_hostPtr mqttHostPtr, LogInData_Ptr logInDataPtr) {
  if(true) mySP("Incoming string: " + s + "\n", FN, LN);
  
  // 📡 ROUTE 1: Modern JSON Payload Handler
  if(s.indexOf("cfgDataJSON:") != -1) {
    
    // Fix: Cut right after the final colon, isolating pure curly brackets {...}
    int jsonStartPos = s.indexOf("cfgDataJSON:") + 12;
    String jsonPayload = s.substring(jsonStartPos);
    mySP("Isolated JSON Text Block: '" + jsonPayload + "'\n", FN, LN);
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonPayload);
    
    if (error) {
      mySP("ERROR: Web form JSON parsing failed! Reason: " + String(error.c_str()) + "\n", FN, LN);
      return;
    }
    
    // Keys extract straight to memory parameters dynamically
    // ✅ Modernized and routed to the correct pointers!
    if (doc["ssid"].is<String>())       logInDataPtr->ssid       = doc["ssid"].as<String>();
    if (doc["pass"].is<String>())       logInDataPtr->ssidPwd    = doc["pass"].as<String>();
    if (doc["mqttServer"].is<String>()) mqttHostPtr->host.fromString(doc["mqttServer"].as<String>());
    if (doc["mqttPort"].is<int>())      mqttHostPtr->portNo      = doc["mqttPort"].as<int>();
    
    // Fixed: Routed to mqttHostPtr instead of logInDataPtr!
    if (doc["mqttUser"].is<String>())   mqttHostPtr->brokerUser  = doc["mqttUser"].as<String>();
    if (doc["mqttPass"].is<String>())   mqttHostPtr->brokerPwd   = doc["mqttPass"].as<String>();

    // Automatically sanitize strings using .trim() for absolute boot security
    logInDataPtr->ssid.trim();
    logInDataPtr->ssidPwd.trim();
    
    // Commit the fresh settings instantly to their proper dedicated storage files
    mySP("Writing new credentials to storage disks...\n", FN, LN);
    writeNetInfoToDisk(netInfoFname);
    writeMqttDataToDisk(mqttHostPtr, mqttInfoFname, LN);
    
    mySP("SUCCESS: Web configuration successfully saved via unified JSON layout!\n", FN, LN);
    return; // Exits cleanly!
  }
  static String path = "";
  if (s.indexOf("incomingFile") != -1) {
    String ts = s.substring(0, 25);
    if (ts.indexOf("incomingFileComplete") != -1) {
      Serial.println("file download is complete.");
      mySP("Downloaded '" + path + "' - '" + String(myFileSys.getFileSize(path.c_str())) +
           " bytes\n", FN, LN);
      path = "";
    } else {
      path = s.substring(s.indexOf(":") + 1, s.indexOf("|"));
      myFileSys.appendFile(path.c_str(), (s.substring(s.indexOf("|") + 1)).c_str());
      return;
    }
  }
  if (s.indexOf("downloadLocation:") != -1) {
    downloadLoc = s.substring(s.lastIndexOf(":") + 1);
    mySP("Download location  = '" + downloadLoc + "'\n", FN, LN);
    sendWS_msg(FN, "DownLoadLocRec:" + downloadLoc, client);
  }
  if (s.indexOf("fileSizePlease:") != -1) {
    client->text("requestedFileSizeIs:" + myFileSys.getFileSize(path.c_str()));
  }
}

/*---------------  GET REBOOT REASON  ---------------*/

void getBootReasonMessage(char *buffer, int bufferlength) {
  #if defined(ARDUINO_ARCH_ESP32)
    esp_reset_reason_t reset_reason = esp_reset_reason();
    switch (reset_reason) {
      case ESP_RST_UNKNOWN:
        snprintf(buffer, bufferlength, "Reset reason can not be determined" + '\0');
        break;
      case ESP_RST_POWERON:
        snprintf(buffer, bufferlength, "Reset due to power-on event" + '\0');
        break;
      case ESP_RST_EXT:
        snprintf(buffer, bufferlength, "Reset by external pin (not applicable for ESP32)" + '\0');
        break;
      case ESP_RST_SW:
        snprintf(buffer, bufferlength, "Software reset via esp_restart" + '\0');
        break;
      case ESP_RST_PANIC:
        snprintf(buffer, bufferlength, "Software reset due to exception/panic" + '\0');
        break;
      case ESP_RST_INT_WDT:
        snprintf(buffer, bufferlength, "Reset (software or hardware) due to interrupt watchdog" + '\0');
        break;
      case ESP_RST_TASK_WDT:
        snprintf(buffer, bufferlength, "Reset due to task watchdog" + '\0');
        break;
      case ESP_RST_WDT:
        snprintf(buffer, bufferlength, "Reset due to other watchdogs" + '\0');
        break;
      case ESP_RST_DEEPSLEEP:
        snprintf(buffer, bufferlength, "Reset after exiting deep sleep mode" + '\0');
        break;
      case ESP_RST_BROWNOUT:
        snprintf(buffer, bufferlength, "Brownout reset (software or hardware)" + '\0');
        break;
      case ESP_RST_SDIO:
        snprintf(buffer, bufferlength, "Reset over printListOfConnectedClientsIO" + '\0');
        break;
    }
    if (reset_reason == ESP_RST_DEEPSLEEP) {
      esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
      switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_UNDEFINED:
          snprintf(buffer, bufferlength, "In case of deep sleep: reset was not caused by exit from deep sleep" + '\0');
          break;
        case ESP_SLEEP_WAKEUP_ALL:
          snprintf(buffer, bufferlength, "Not a wakeup cause: used to disable all wakeup sources with esp_sleep_disable_wakeup_source" + '\0');
          break;
        case ESP_SLEEP_WAKEUP_EXT0:
          snprintf(buffer, bufferlength, "Wakeup caused by external signal using RTC_IO" + '\0');
          break;
        case ESP_SLEEP_WAKEUP_EXT1:
          snprintf(buffer, bufferlength, "Wakeup caused by external signal using RTC_CNTL" + '\0');
          break;
        case ESP_SLEEP_WAKEUP_TIMER:
          snprintf(buffer, bufferlength, "Wakeup caused by timer" + '\0');
          break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
          snprintf(buffer, bufferlength, "Wakeup caused by touchpad" + '\0');
          break;
        case ESP_SLEEP_WAKEUP_ULP:
          snprintf(buffer, bufferlength, "Wakeup caused by ULP program" + '\0');
          break;
        case ESP_SLEEP_WAKEUP_GPIO:
          snprintf(buffer, bufferlength, "Wakeup caused by GPIO (light sleep only)" + '\0');
          break;
        case ESP_SLEEP_WAKEUP_UART:
          snprintf(buffer, bufferlength, "Wakeup caused by UART (light sleep only)" + '\0');
          break;
        default :
          snprintf(buffer, bufferlength, "Unknown reset reason %d", reset_reason);
          break;
      }
    }
  #endif
  #if defined(ARDUINO_ARCH_ESP8266)
    rst_info *resetInfo;
    resetInfo = ESP.getResetInfoPtr();
    switch (resetInfo->reason) {
      case REASON_DEFAULT_RST:
        snprintf(buffer, bufferlength, "Normal startup by power on" + '\0');
        break;
      case REASON_WDT_RST:
        snprintf(buffer, bufferlength, "Hardware watch dog reset" + '\0');
        break;
      case REASON_EXCEPTION_RST:
        snprintf(buffer, bufferlength, "Exception reset, GPIO status won't change" + '\0');
        break;
      case REASON_SOFT_WDT_RST:
        snprintf(buffer, bufferlength, "Software watch dog reset, GPIO status won't change" + '\0');
        break;
      case REASON_SOFT_RESTART:
        snprintf(buffer, bufferlength, "Software restart ,system_restart , GPIO status won't change" + '\0');
        break;
      case REASON_DEEP_SLEEP_AWAKE:
        snprintf(buffer, bufferlength, "Wake up from deep-sleep" + '\0');
        break;
      case REASON_EXT_SYS_RST:
        snprintf(buffer, bufferlength, "External system reset" + '\0');
        break;
      default:
        snprintf(buffer, bufferlength, "Unknown reset cause %d", resetInfo->reason);
        break;
  };
  #endif
}

/*---------------  SERVER FILE NOT FOUND  ---------------*/

void fileNotFound(AsyncWebServerRequest *request, const_dataPtr cdp) {
  request->send(404, "text/plain", "ERROR:404 - " + String(cdp->appName) + " says page not found.");      // tell the requesting browser the page was not found
}


/*---------------  SERVER PAGE NOT FOUND AND CHECKING TO SEE IF FILE EXISTS ON SERVER  ---------------*/

void sendFileIfPresent(AsyncWebServerRequest *request, const_dataPtr cdp) {
  IPAddress ip = request->client()->remoteIP();                                                       // capture the requesters ip address
  String requested_file = request->url();                                                             // capture the URL being requested
  if(requested_file.indexOf("/api/2W") == -1) {                                                       // if the request does not contain "/api/2W"
    mySP("received a request for '" + requested_file + "' from '" + ip.toString() + "'.\n", FN, LN);  // log a request for a file has been received
  }
  if (requested_file.indexOf("/sendfile:")) {                                                         // if the url contains 'sendfile:' then
    requested_file = requested_file.substring(requested_file.indexOf(":") + 1);                       // strip it off
  }
  if (myFileSys.getFS().exists(requested_file)) {                                                             // if the url requested is the same as a file on this server's disk
    downloadingFile = true;                                                                           // tell the server that a download is in process
    mySP("sending web page code file '"+requested_file+"' to '"+ip.toString() + "'.\n", FN, LN);      // log what is happening
    File f = myFileSys.getFS().open(requested_file, FILE_READ);                                               // open the file
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain",                    // begin sending the file in a chuncked response mode
                                  [f](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      //Write up to "maxLen" bytes into "buffer" and return the amount written.
      //index equals the amount of bytes that have been already sent
      //You will be asked for more data until 0 is returned
      //Keep in mind that you can not delay or yield waiting for more data!
      return myFileSys.load_data(f, buffer, maxLen, index);                                                       // load the next maxlen btes of the file into the buffer
    });
    response->addHeader("Server", "ESP Async Web Server");                                              // add a header to the response message
    request->send(response);                                                                            // send the response message
  } else {                                                                                              // else this is a page not found so
    fileNotFound(request, cdp);
  }
}

/*--------------- UPLOADS THE FILE (REPLACES IT) VIA A WIFI CONNECTION ---------------*/

void handlePostFile(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  static File file;
  if(false) {
    mySP("Received a POST request from: " + request->url(), FN, LN);
    mySP("request->url: " + request->url() + "\n", FN, LN);
    mySP("request->contentLength(): " + String(request->contentLength()) + "\n", FN, LN);
    mySP("filename: " + filename + "\n", FN, LN);
    mySP("index: " + String(index) + "\n", FN, LN);
    mySP("len: " + String(len) + "\n", FN, LN);
    mySP("final: " + String(final?"Yes":"No") + "\n", FN, LN);
  }
  String fname = downloadLoc + "/" + filename;
  if (index == 0) {                                                                     // if this is the first pass through
    downloadingFile = true;                                                             // let all the parts of the system know a download has been started
    myFileSys.deleteFile((fname).c_str(), LN);                                         // delete the existing file
    mySP("Downloading " + filename + " and putting it into " + fname + "\n", FN, LN);   // log the action
    file = myFileSys.getFS().open((fname).c_str(), FILE_WRITE);                                 // open the file
  }
  if (!file) {                                                                  // if it failed to be opened
    mySP(getLogTime() + ",Failed to open file '" + fname                        // log the error
                + "' for appending.\n", FN, LN);
  }
  if (!file.write(data, len)) {                                                 // write the data to the file and if written bytes are zero
    mySP("Append failed to file'" + fname + "'\n", FN, LN);                     // log the data was not successfully written
  }
  if (final) {                                                                  // if this is the last pass through then
    downloadingFile = false;                                                    // let all the parts f the system know the download is complete
    file.close();                                                               //  close the file
    mySP("Download of '" + fname + "' is complete. File size is: "              // log the transfer is complete
         + String(myFileSys.getFileSize(fname.c_str())) + "\n", FN, LN);
    request->send(200);                                                         // send an okay back to the sending web page
  }
}

/*---------------    GLOBAL CONFIGURATION CHANGE   ---------------*/

void gblCfgChg(String data, MQTT_hostPtr mhp) {
  const int EXPECTED_COUNT = 10;
  String oldTzAbbrv = logInD1.tzAbbrv;
  String oldOlsename = logInD1.olsenTzName;                                   // extract the individual fields and save them to the appropiate place
  enum gcc {GCC_SSID,                                       // GCC = global configuration change
            GCC_SSID_PWD, 
            GCC_MQTT_SERVER_NAME,
            GCC_MQTT_SERVER_PORT_NUM,
            GCC_MQTT_BROKER_NAME,
            GCC_MQTT_BROKER_PWD,
            GCC_TZ_ABBREV,
            GCC_TZ_OLSEN_NAME
  };
  int actCnt = numOfCharInStr(data, '|') + 1 ;                                        // actCnt (actual count) the number of fields in the incoming string
  if(actCnt != EXPECTED_COUNT) {                                                      // if the number of items is not the expected count then
    mySP("Received an inbound Global Cfg change MQTT nessage with " + String(actCnt)  // log the issue
       + " items instead of the expected " + String(EXPECTED_COUNT) 
       + " items.  Aborting receive.\n", FN, LN);
    mySP("Received an inbound Global Cfg change MQTT nessage:" + data + "\n", FN, LN);
    return;                                                                           // and return to the calling function (ABORT CFG CHANGE)
  }
  String arr[actCnt];                                                                 // create an array for each of the fields
  int count = strToArray(data, '|', actCnt, arr);                                    // split the string into the array 'arr'
  if(true) {                                                                          // if debugging
    mySP("Planning in receiving " + String(actCnt) + " elements.\n", FN, LN);         // log the planned actCnt on cfg items to be received
    mySP("Incoming cfg data has " + String(count) + " elements.\n", FN, LN);          // log the count of elements strToArray found
  }
  logInD1.ssid = arr[GCC_SSID];
  logInD1.ssidPwd = arr[GCC_SSID_PWD];
  mhp->host.fromString(arr[GCC_MQTT_SERVER_NAME]);
  mhp->portNo  = arr[GCC_MQTT_SERVER_PORT_NUM].toInt();
  mhp->brokerUser = arr[GCC_MQTT_BROKER_NAME];
  mhp->brokerPwd = arr[GCC_MQTT_BROKER_PWD];
  logInD1.tzAbbrv = arr[GCC_TZ_ABBREV];
  logInD1.olsenTzName = arr[GCC_TZ_OLSEN_NAME];
  saveCfgChgs();                                                                      // sane the configuration changes to disk
  if(oldOlsename != logInD1.olsenTzName || oldTzAbbrv != logInD1.tzAbbrv) {           // if the tz has changed then
    tzChange = true;                                                                  // cause the re-sync of EzTime
  }
}

/*---------------    GLOBAL CONFIGURATION CHANGE (JSON VERSION)   ---------------*/

void gblCfgChgJson(String jsonStr, MQTT_hostPtr mhp) {
  // 1. Allocate memory for parsing the incoming JSON payload string
  JsonDocument doc;
  
  // 2. Deserialize the JSON string
  DeserializationError error = deserializeJson(doc, jsonStr);
  
  if (error) {
    mySP("JSON Deserialization failed: " + String(error.c_str()) + "\n", FN, LN);
    mySP("Received invalid JSON payload: " + jsonStr + "\n", FN, LN);
    return; // Abort if string is corrupted or poorly formatted
  }
  
  // 3. Stash your historical settings to check for changes at the end
  String oldTzAbbrv   = logInD1.tzAbbrv;
  String oldOlsename  = logInD1.olsenTzName;
  
  if(true) { // If debugging, print confirmation
    mySP("Successfully received valid JSON configuration payload.\n", FN, LN);
  }
  
  // 4. Extract parameters directly out of the JSON object keys using v7 compliance
  if (doc["ssid"].is<String>())   logInD1.ssid    = doc["ssid"].as<String>();
  if (doc["pass"].is<String>())   logInD1.ssidPwd = doc["pass"].as<String>();
  
  if (!doc["broker"].isNull()) {
    mhp->host.fromString(doc["broker"].as<String>());
  }
  
  if (!doc["port"].isNull()) {
    mhp->portNo = doc["port"].as<int>();
  }
  
  if (doc["user"].is<String>())   mhp->brokerUser = doc["user"].as<String>();
  if (doc["pwd"].is<String>())    mhp->brokerPwd  = doc["pwd"].as<String>();
  if (doc["tz"].is<String>())     logInD1.tzAbbrv = doc["tz"].as<String>();
  
  // Fallback checks to catch both 'olsen' and 'olson' spellings out of Node-RED
  if (!doc["olsen"].isNull()) {
    logInD1.olsenTzName = doc["olsen"].as<String>();
  } else if (!doc["olson"].isNull()) {
    logInD1.olsenTzName = doc["olson"].as<String>();
  }
  
  // ==========================================
  // 5. EXTRACT AND COMMIT THE NEW POSIX RULE
  // ==========================================
  if (!doc["posix"].isNull()) {
    logInD1.posixTzRule = doc["posix"].as<String>();
    
    // Proactively apply this rule to the ESP32 internal environment clock engine right now!
    setenv("TZ", logInD1.posixTzRule.c_str(), 1);
    tzset();
    mySP("Applied new POSIX Timezone Rule string to ESP32 core clock: " + logInD1.posixTzRule + "\n", FN, LN);
  }
  
  // 6. Commit the updated struct to flash/disk memory on the chip
  saveCfgChgs();
  
  // 7. Fire the ezTime re-sync flag if environmental constraints shifted
  if(oldOlsename != logInD1.olsenTzName || oldTzAbbrv != logInD1.tzAbbrv) {
    tzChange = true;
  }
  // ==========================================
  // 💾 NEW CONCLUSION LOGGING BLOCKS
  // ==========================================
  if(true) {
    mySP("\n--- gblCfgChgJson Conclusion Variable Verification ---\n", FN, LN);
    mySP("Stored Wi-Fi SSID  : " + logInD1.ssid + "\n", FN, LN);
    mySP("Stored Wi-Fi Pass  : [Masked]\n", FN, LN);
    mySP("MQTT Host Address  : " + mhp->host.toString() + "\n", FN, LN);
    mySP("MQTT Target Port   : " + String(mhp->portNo) + "\n", FN, LN);
    mySP("MQTT Broker User   : " + mhp->brokerUser + "\n", FN, LN);
    mySP("MQTT Broker Pass   : [Masked]\n", FN, LN);
    mySP("Timezone Abbrev    : " + logInD1.tzAbbrv + "\n", FN, LN);
    mySP("Olsen Timezone Name: " + logInD1.olsenTzName + "\n", FN, LN);
    mySP("POSIX Clock Rule   : " + logInD1.posixTzRule + "\n", FN, LN);
    mySP("------------------------------------------------------\n\n", FN, LN);
  }
}

/*---------------    CONVERT UNSIGNED LONG TO STRING   ---------------*/
  
String ults(unsigned long x) {                                                  // Unsigned long to String
  char str[33];
  sprintf(str, "%lu",  x);
  return(String(str));
}

/*---------------    GENERATE APP DATA INFO FOR WEB PAGES   ---------------*/

String generateAppData(String appName, String fname, String compileDate) {
  String theVer = fname.substring(fname.lastIndexOf("_v") + 2, fname.length() - 4);
  return  "\n  " + appName + " Version: " + theVer + "\n"
          "    by Stephen McKeon\n"
          "    Compiled: " + String(compileDate) + "\n    " + fname + "\n"
          "    Uptime: " + getServerUptime() + "\n" +
          "    IP address: " + WiFi.localIP().toString() + "\n";
}

/*--------------- UNIVERSAL STRING WRAPPER FACTORY ---------------*/

String makeDefaultStr(void (*workerFunction)(String &)) {
  String tempContainer;
  workerFunction(tempContainer); // 1. Jumps directly to the address you passed
  return tempContainer;          // 2. Passes the filled string back
}
