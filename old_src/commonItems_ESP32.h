
#ifndef _COMMON_ITEMS_ESP32_H_
#define _COMMON_ITEMS_ESP32_H_

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ezTime.h>
#include <Ticker.h>
#include <Update.h>

#include <myStuff.h>
#include <myMQTT.h>
#include <mySd.h>
#include <myWiFi.h>

#define FN __func__                                                             // compiler get function name
#define LN __LINE__                                                             // what line was the compiler on
#define MY_TIME_F myTZ.dateTime("Y-m-d~ H:i:s.v-T")                             // time format for all the time calls the app makes
#define LOG_F_NAME_FORMAT myTZ.dateTime("ymd")                                  // date format for the log files - 6 chars, DDMMYY
#define ISO8601Time myTZ.dateTime(ISO8601)                                      // get the ISO 8601 date/time
#define WHAT_TZ_IS_SHOWING myTZ.dateTime("O").toInt()                           // what time zone does eztime have us on
#define TIME_TO_WAIT_FOR_TIME_SYNC 120000                                       // how long will the server wait when tring to get a new timeSync before initiating a reboot
#define TIME_ZONE_ABBRIV myTZ.dateTime("T")                                     // what is the abbreviation for the time zone 'CST'
#define JUST_THE_DAY myTZ.dateTime("d")                                         // just put the day number into this
#define YEAR myTZ.dateTime("Y")                                                 // get the current time's year
#define MONTH myTZ.dateTime("m")                                                // get the current time's month
#define DAY myTZ.dateTime("d")                                                  // get the current time's day
#define HOUR myTZ.dateTime("H")                                                 // get the current time's hour
#define MINUTE myTZ.dateTime("i")                                               // get the current time's minutes
#define SEC myTZ.dateTime("s")                                                  // get the current times seconds
#define MILLIS myTZ.dateTime("v")                                               // get the current times milliseconds
#define L_OFF myTZ.dateTime("m/d/y g:i:s a")
#define T_CHG myTZ.dateTime("g:i a D M j, y ")                                  // time changed - used to indicate the time, less seconds, a change occured
#define SEND_T myTZ.dateTime("G:i:sA")                                          // send temperature time
#define DEVICE_IS_RUNNING 2                                                     // ESP8266 on-board LED used to show the server is up and functioning
#define DLY_FOR_W_BRT 2000                                                      // the delay for writing a changed brightness to disk
#define INTVL_TO_LOG_TEMPS 5                                                    // the minute interval to log temperatures
#define MINUTE_TO_CHECK_NODERED_CONNECTION 5
#define WAIT_FOR_EZT_SYNC 100                                                   // seconds to wait for EzTime to succfully set the time on this server
#define EZT_TIME_NOT_SET 0
#define EZT_TIME_NEEDS_SYNC 1
#define EZT_TIME_SET 2
#define MAX_NUM_CONNECTED_CLIENTS 10                                            // size of the array connected clients are kept in
#define RETAIN true
#define DO_NOT_RETAIN false
#define MAX_SPLIT_ELEMENTS 25                                                   // maximum number of elements the splt string library is set to allow
#define LAST_MQTT_CONNECT_ATTEMPT -10000                                        // start at -10 seconds so the first pass through will be allowed on boot
#define MQTT_RETRY_DELAY_LEN 10000                                               // how long to delay between attemst to restart mqtt
#define DIRS_IN_USE "DIR : /,DIR : /var,DIR : /var/log,DIR : /html,DIR : /temp,DIR : /appData,DIR : /dirLists" // dirs to be advertized by update.html

// WHERE TO SEND LOG ENTRIES TO
#define BOOT_LOG_STR 0
#define BOOT_LOG_STR_NAME "Boot log string"
#define BOOT_LOG_FILE 1
#define BOOT_LOG_FILE_NAME "Boot log file"
#define MQTT_SERVER 2
#define MQTT_SERVER_NAME "MQTT Server"
#define OFFLINE_DISK_FALLBACK 3
#define OFFLINE_DISK_FALLBACK_NAME "Offline disk fallback"


#define MAX_TIME_AS_ACCESS_PT 600000                                            // allow the access point to run for only 10 minutes

#define UPTIME "server/uptime"                                                  // how long since the server came on line

// ERRORS
#define MAX_AP_TIME_REACHED "Max time as access point has been reached. Requesting a reboot"
#define ERROR_NO_OLSEN_NAME "The Olsen name is empty, inserting 'Denver'.  Please config the Olsen name using Node-Red.\n"

#define CLIENT_TIME_OUT 120000                                                  // how long to continue looking for the client after a network test

#define U_PART 100                                                              // U_FS defined in Updater.h (ESP8266) and set equal to 100

#ifndef CI_DO_DEBUG                                                             // if Common items debug has not been defined then define it as false
  #define CI_DO_DEBUG false;
#endif
/*
enum rebootBy = {COMMON_ITEMS,
                 MY_DHT,
                 MY_MQTT,
                 MY_OTA,
                 MY_SD,
                 MY_WIFI,
                 TEMP_SENSORS,
                 MY_GPS,
                 THE_APP
};
*/
/*-------------------------------------*/

/*---------------  SHARED ROLLING LOG DEFINITIONS  ---------------*/
extern const int MAX_RAM_LINES;
extern const int MAX_LOG_LINE_LEN;
extern char ramLog[][256]; // Strict C++ rule: Outer array bounds can be blank, but inner type must be declared!
extern int ramLogIndex;
extern bool ramLogWrapped;


extern const String lastBootTimeFname;                                          // UNIX time the server last booted
extern short CK_HEAP_INTERVAL;                                                  // how often to check the heap.  this ccan be changed in the app
extern String serverUptime;                                                     // how long since the server came on lime
extern const String appDir;                                                     // name of the app directory
extern const String HTMLFsDir;                                                  // file directory for HTML page files
extern const String rebootReason;                                               // stores the reason when the app initiates a reboot
extern AsyncWebServer server;                                                   // server port 80
extern bool mqttDownLogF;                                                       // when MQTT goes down this signals action to syslog writer
extern String startUpBootLogStr;                                                // string to hold initial loog info until the fie system is mounted
extern const String dirLists;                                                   // directory on disk for files with folder contents in them
extern const String allDirectoriesToFile;                                       // file to contains all the directories needed for update.html to work
extern long lastFreeHeap;                                                       // what was the free heap when last checked

extern bool  timeActive;                                                        // the ESP has a good clock, set to true when connected to a time server

extern const String compile_date;                                               // date and time the compiler ran

extern String tmpBootSyslog;                                                    // temporary boot syslog file
extern String connectedSSID;                                                    // what ssid is the ESP connected to
extern String srvIPAddr;                                                        // holds the ip address of the server
extern String accessPtIP;                                                       // holds the server IP Address when it is acting as an access point

extern unsigned long rebootAskedforAt;                                          // milliseconds reboot was asked for at
extern unsigned long lastRebootUpdate;                                          // keep track of the milliseconds when a reboot update is done
extern unsigned long timeTimeSyncLost;                                          // milliseconds since boot the the EZTime time sync was lost
extern int whereToSendLogEntryTo;                                               // where should the log entries get posted...always to BOOT_LOG_STR on boot

extern bool rebooting;                                                          // are we counting down for a reboot
extern bool downloadingFile;                                                    // javascript 'fetch()' is dwnloading a file...need not to run any 'DallasTemperature' routines while downloading
extern bool doTx_debug;                                                         // turn on EasyTz debugging
extern bool sysLogRotationComplete;                                             // have the syslogs been rotated
extern bool tempLogRotationComplete;                                            // have the templogs been rotated
extern bool tzChange;                                                           // has the desired timezone changed

extern size_t content_len;                                                      // used in OTA updates

extern AsyncWebServer server;                                                   // server port 80
extern  Timezone myTZ;                                                          // declare the ezTime TimeZone object;
extern AsyncWebSocket ws;                                                       // declare the asyncWebSocket object
extern timeStatus_t ezT_status;                                                 // declare ezTime status and put the current status in it

extern bool startEzTime;                                                        // gets set to true when set up ezTime needs to be run
extern bool startMQTT;                                                          // gets set to true when the MQTT server needs to be connected to
extern bool mqttRunning;
extern bool startMQTT_subscriptions;                                            // gets set to true when the MQTT suubscripts need to be activated
extern bool logToMQTTServer;                                                    // move temp logs to the MQTT server and start doing all logs to the MQTT server
extern void wifi_setup();
extern String downloadLoc;                                                      // contains the path to add to file downloads from clients
extern bool needBootTime;                                                       // need to get the get the servers boot time


/*-------------------------------------*/


/*--  DEFINITIONS FROM EXTERNAL SOURCES REQUIRED. --*/
extern const_data constData;                                                    // from the main file = {PG_TITLE, ACCESS_PT_NAME, PAGE_HEADER, UPLOAD_PG, APPNAME}
extern AsyncWebServer server;                                                   // server port 80
extern ConnectedClients connectedClients[];                                     // an array, MAX_NUM_CONNECTED_CLIENTS long, that hold connected client info
extern const_dataPtr constDataPtr;                                              // constants data struct pointer from main
extern void webServerActions();
extern void saveCfgChgs();
extern String MQTT_IN_SYSLOG;                                             // MQTT topic for sending syslogs to the MQTT server (DEFINED IN THE APP FILE)
extern void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
extern void doAppTimeIsSetActions();
extern void doServerRebootActions();
extern MQTT_hostPtr mqttHostPtr;
extern const int countOfTempSensors;                                            // defined in the main app file.h
extern tempSensorPtr OneWireSensorsInUse[];                                     // defined in the main app file.h
extern const String UPD_HTML_FILE;                                              // from .ino file: name of the update page HTML file
extern LogInData logInD1;                                                       // from .ino file: information used for booting
extern LogInData_Ptr logInDataPtr;                                              // from .ino file: pointer to the log in data struct

/*--  FUNCTION DECLARATIONS. --*/
extern String getTimeStatusWords(int status);
extern bool isTimeGood();
extern String syslogLocName(int x);
extern void commonItemsFileSysStartup();
extern void commonItemsStartup();
extern String getServerUptime();
extern time_t getBootSeconds();
extern void saveBootTime();
extern void commonItemsEvents();
extern String pdl(String callingFunct, int origLineNo);
extern void sendWS_msg(String callingF_name, String s, AsyncWebSocketClient *client);
extern void dumpRamLogsToMqtt();
extern void recordToRamLog(String s);
extern String getLogTimeStamp();
extern String doSerialMonLogEntry(String event, String functionName, int lineNo);
extern String doJsonStrLogEntry(String event, String functionName, int lineNo);
extern void mySP(String event, String functionName, int lineNo);
extern void sendToSyslog(String s);
extern void beginSerialConnection();
extern void moveBootLogsToSyslog();
extern void setUpEZTime(String olsenName);
extern bool isTimeToRun(runTimePtr when);
extern int minuteToInt();
extern int secondToInt();
extern String getLogTime();
extern String msToCommaSepDHMS(unsigned long ms, char separator, bool pretty);
extern String msToDHMS(unsigned long ms);                                              // milliseconds to days, hours, minutes, second
extern void startWebServer();
extern void serverRebootInProgress();
extern void requestReboot(String reason, bool immediately);
extern void restartESP32();
extern void notFound(AsyncWebServerRequest *request, String appName);
extern size_t load_data(File f, uint8_t *buffer, size_t maxLen, size_t index);
extern void restartESP();
extern unsigned long suli(unsigned long minuend, unsigned long subtrahend, int from, bool logErr);
extern unsigned long subUnsignedLongInt(unsigned long minuend, unsigned long subtrahend);
extern bool twoOfThree(int pin);
extern void dumpTM_struct(tmElements_t *tm);
extern time_t localTimeToUNIX_secs();
extern String unixT_toHumanT(time_t t);
extern void checkTheHeap(bool force);
extern  void gatherWiFiSetupData(String &jsonOutput, MQTT_hostPtr mqttPtr);
extern void initNewWiFiPg(String s, AsyncWebSocketClient *client, const_dataPtr constDataPtr, MQTT_hostPtr mqttPtr);
extern void hdlWiFiSetupEvent(String s, AsyncWebSocketClient *client, MQTT_hostPtr mqttHostPtr, LogInData_Ptr logInDataPtr);
extern void getBootReasonMessage(char *buffer, int bufferlength);
extern void printCharArrValues(char sourceArr[], int sourceArrLen);
extern void fileNotFound(AsyncWebServerRequest *request, const_dataPtr cdp);
extern void sendFileIfPresent(AsyncWebServerRequest *request, const_dataPtr cdp);
extern void handlePostFile(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
extern void gblCfgChg(String data, MQTT_hostPtr mhp);
extern void gblCfgChgJson(String jsonStr, MQTT_hostPtr mhp);
extern String ults(unsigned long x);                                                   // Unsigned long to String
extern String generateAppData(String appName, String fname, String compileDate);
extern String makeDefaultStr(void (*workerFunction)(String &));

#endif /* _COMMON_ITEMS_ESP32_H_ */
