/*
 2024-09-04 - Moved areOneWireTempsNeeded() from the app code to tempSensors.cpp to further simplify the writing of new apps.
                The app must still declare:
                     All sensors must be declared usng:
                       TempSensor PCBT = {"PCB", false, false, 0, 5, 0, false, 300000, 30000, 0, "", 0.0, 0.0, 0.0, {0x28,0x52,0xB9,0x0C,0x00,0x00,0x00,0xF0}, NULL, "PCB|", "", "ROController/tempsGauge/PCB", "temperatures/ROcontroller/chart/PCB", MQTT_IN_PCB_TEMP_LOG};
                    tempSensorPtr OneWireSensorsInUse[] = {}; // all temp sensors need to be listed here
                    TempSensors and OneWireSensorsInUse must be declared as an extern

 * Apr  10, 2025 - Added a int parameter to doTempStartUp().  This will set the DS20B18's temp resolution.  This must be a number between
                    inclusively between 9 and 12.
 * May  19, 2025 - Moved 'tempSenActNeeded()' from commonItemsEvents() to ds18b20Events() in the myTemperature.cpp file to enable app not
                     using DS18B20 temp sensors to compile without errors.
 
 * Jun 06, 2025 - Added areOneWireTempsNeeded() to ds18b20Events().  areOneWireTempsNeeded() should now be no longer needed in the
                    startup and main of the apps.
                    in the startup section please call:
                      doTempStartUp(9);   // use a number between 9 and 12 inclusive to set the resolution you want/need
                      ds18b20Events();    // runs the required events to get the temps reported
 * Jul 20, 2025 - Rewrote must of the tempSensors.cpp library.
                    The following TempSensor struct items are no longer in use:
                       int index;                                // NIU // index of sender used by the OneWire sensor library
                       bool needSenTemp;                         // NIU // is the sender temp needed
                       bool needToLogTemp;                       // NIU // time to log the sensor temperature
                       unsigned long tLastCked;                  // NIU // time the sensor temp was last checked
                       int ckMinInt;                             // NIU // minute interval to write temp log file (MUST NOT BE ZERO)
                       int tLastLogged;                          // NIU // the minute the last time this sensor value was logged
                       bool checked;                             // NIU // has the sender temperature been checked since needToLogTemp was set to true
                       unsigned long ckIntv;                     // NIU // millisecond interval to check this sensor value
                       unsigned long pausedAt;                   // NIU // milliseconds the sender paused at waiting for a download to complete
                       String msg;                               // NIU // holds the data being sent to the log file
                    and need to be removed from the apps that use this library.  I believe this will consist of mostly
    `                 the initialization of the structs and any node-red components (changing configuration data) that uses it.
                    There are now new vars that keep track of the time intervals and the need to do actions.  They are:
                      senLastChked    milliseconds the sensors we last read
                      senLastLogged   the minute the logs were last written
                      senChkIntv      the interval to wait between reading the sensors    - application need to set this
                      senLogIntv      the logging interval                                - application need to set this
                      needSenTemp     is sensor reading needed
                      needToLogTemp   is sensor logging needed
                    Individual sensors no longer get read.  The DallasTemperature library's call to read sensors reads all sensors at
                      once so it is a waste of resources to do multiple reads to get all the sensors.  For this reason there is only
                      one check interval for the temps and one for the logs instead of one each for each sensor.  If different times
                      are desired this will need to be worked in the applications routines for writing the temps and logs out with the
                      shortest time desired set to the senChkIntv and senLogIntv.
                    Fixed bug that would continually execute readDS18B20Temperature() every loop rather than waiting the specified time.
                 
 ADDED senChkIntv and senLogIntv to replaced the ones removed from the  TempSensor STRUCT.  The thought is that all sensors will use the same values

*/

#include "myConfig.h"

#ifdef HARDWARE_USES_DS1820

#include <ArduinoTrace.h>
#include <commonItems_ESP32.h>
#include <mySD.h>
#include <myMQTT.h>
#include <myText.h>
#include <tempSensors.h>

int8_t cntOf1WireSens = 0;                                                      // the number of one wire sensors on this server
bool DnLoadMsgSent = false;
time_t senLastChked = 0;
int senLastLogged = 0;
time_t senChkIntv = 2000;
int senLogIntv = 5;
bool needSenTemp = false;
bool needToLogTemp = false;

/*--------------- DO TEMPERATURE STARTUP ACTIONS ---------------*/
/* TEMPERATURE SENSOR RESOLUTION MUST BE 9, 10, 11, OR 12 ONLY */

void doTempStartUp(int resolution) {
  DUMP("STARTING TEMPERATURE SENSORS.");
  myFileSys.ensureFileExists(oneWireData, tempCfgToStr(), LN);
  readTempCfgFromDisk(oneWireData);
  if(resolution < 9 || resolution > 12) {                                       // for each sensor set its resolution
    mySP("DS18B20 resolution of " + String(resolution) +                        // log this temps resolution
         " does not exist.  Changed to 9.\n", FN, LN);
    resolution = 9;                                                             // set it
  }
  sensors.begin();                                                              // Start the Dallas Temperature sensors
  getMountedOneWireAddresses(resolution);                                       // get and print the one wire address
}

/*--------------- TEMPERATURE EVENTS ---------------*/

void ds18b20Events() {                                                          // OneWire temp sensor event loop - should be placed in the apps main event loop
  if(cntOf1WireSens == 0) return;                                               // no sensors have been detected so just exit.
  areOneWireTempsNeeded();                                                      // handle any needed temperature sender updates
  tempSenActNeeded();                                                           // if an read temp action is needed then do it
}

/*---------------  GET DS18B20 TEMPS  ---------------*/
/* SET THE NEED TEMP OR LOG TO TRUE IS NEEDED */

void areOneWireTempsNeeded() {
  int minutes = minuteToInt();                                                // convert the current minute it to an integer
  if(suli(millis(), senLastChked, LN, true) > senChkIntv) {                             // is it time to read the sensor (now - last checked > interval to check)
    needSenTemp = true;                                                       // then tag it as such
  }
  if (minutes % senLogIntv == 0 && senLastLogged != minutes) {                   // if it is time to write a temperature to the  log file
    needToLogTemp = true;                                                     // tag it as such
  }
}

/*--------------- AVERAGE DS18B20 TEMP ---------------*/

float avgDS18B20Temps(tempSensorPtr sen, float curT) {
  static int count = 0;                                                         // where in the cycle
  float total = 0.0;                                                            // store the total of all the temps
  float avg = 0.0;                                                              // array average
  String s = "";
  sen->avgTemp[count++] = curT;                                                 // store the curret temp in the array
  for(int i = 0; i < AVG_TEMP_ARRAY_SIZE; i++) {                                // for all the elements in the array
    total += sen->avgTemp[i];                                                   // add the element to the total
    s += String(sen->avgTemp[i], 3) + ", ";
  }
  Serial.println(s);
  avg = total / AVG_TEMP_ARRAY_SIZE;                                            // get the average temp
  if(count >= AVG_TEMP_ARRAY_SIZE) count = 0;                                   // if the count goes ober the desired number then roll it over
  if(fabs(curT - avg) > 2.0) {                                                  // if there is a difference of greater than 2 with the average
    mySP("ERROR: curT = "+String(curT)+", avgT = "+String(avg)+"\n",FN,LN);     // loog the error
    avg = curT;                                                                 // then just return the current temp reading
  }
  return avg;                                                                   // return the average temperature
}

/*---------------    CHECK FOR TEMP SENSOR ACTIONS NEEDED   ---------------*/

void tempSenActNeeded() {
  bool result = false;
  if(needSenTemp) {                                                             // if it is time to get the sensor readings then
    if(readDS18B20Temperature()) {                                              // read the sensors and if a good reading was received
      needSenTemp = false;                                                      // reading the sensors so set need to read sensors to false
      for(int i = 0; i < countOfTempSensors; i++) {                             // for each sensor
        sendTemp(OneWireSensorsInUse[i], false);                                // let the application send the temp as needed
      }
    } else {                                                                    // else a good reading was not received
      mySP("ERROR - FAILED TO READ ONEWIRE TEMP SENSORS.\n", FN, LN);           // log the error
    }                                                                           // get the sensor temperature
  }
  if(needToLogTemp) {                                                           // if it is time to send temp logs
    needToLogTemp = false;                                                      // logging the sensors so set need to log to false
    senLastLogged = minuteToInt();                                              // convert the current minute it to an integer
    for(int i = 0; i < countOfTempSensors; i++) {                               // for each sensor
      sendLogTemps(OneWireSensorsInUse[i]);                                     // let the application log the last recorded temp as needed
    }
  }
}

/*--------------- READ DS18B20 TEMP ---------------*/

bool readDS18B20Temperature() {
  float degC = 0;
  float degF = 0;
  tempSensorPtr sen = NULL;
  
  if (downloadingFile) {                                                        // if a file is being downloaded we may not make any DallasTemperature library calls or the server will crash
    if(!DnLoadMsgSent) {                                                        // if a web download is in process
      mySP(DOWNLOAD_ERROR_MSG, FN, LN);                                         // log the error
      DnLoadMsgSent = true;                                                     // don't need to run this again until the next download occurs
    }
    return "-200";                                                              // let the calling function know this occurred
  } else {                                                                      // no download is in process so
    sensors.requestTemperatures();                                              // ask for the DS18B20's temp reading
    for(int i = 0; i < countOfTempSensors; i++) {                               // for each sensor
      degC = sensors.getTempC(OneWireSensorsInUse[i]->address);                 // get it'c Cº's
      degF = floor(sensors.getTempF(OneWireSensorsInUse[i]->address));          // get it's Fº's
      if (degC == -196.6) {                                                     // if the system failed to get a temp reading
        mySP("Failed to get temp reading from OneWire temp sensors\n", FN, LN); // log the error
        return false;                                                           // failed sp return false
      }
      sen = OneWireSensorsInUse[i];                                             // get the sensor struct
      sen->curTempC = sensors.getTempC(sen->address);                           // get it's ºC reading
      sen->curTempF = sensors.getTempF(sen->address);                           // get it's ºF reading
      if(sen->curTempC == -127 || sen->curTempC == 85) {
        if(isTimeToRun(sen->failedTempRead) || sen->failedTempRead == 0) {
          if(sen->curTempC == -127) {
            mySP("The temperature sensor named " + sen->name +
                 SenErrorNeg127C, FN, LN);
          } else if(sen->curTempC == 85) {
            mySP("The temperature sensor named " + sen->name +
                 SenErr85C, FN, LN);
          }
        }
      } else sen->failedTempRead = 0;
    }
    senLastChked = millis();
    return true;
  }
}

/*---------------  PRINT TEMP SENSOR STRUCT  ---------------*/

void printTempSenStruct(tempSensorPtr sen) {
  mySP("Current millisecond count = " + String(millis()) +
       "\nPrinting DS18B20 information struct:"
       "\n    Name: " + sen->name +
       "\n    index: " + String(sen->index) +
       "\n    msg: " + sen->msg +
       "\n    curTempC: " + String(sen->curTempC, 2) +
       "\n    curTempF: " + String(sen->curTempF, 2) +
       "\n    prevTempF: " + String(sen->prevTempF, 2) +
//       "\n    Device address: " + DS18B20_sensorAddrToStr(sen->address) +
//       "\n    Client: " + String(sen->client?"Yes":"NULL") +
       "\n    msgHeader: " + sen->msgHeader +
       "\n    MQTT_gaugeTopic: " + sen->MQTT_gaugeTopic +
       "\n    MQTT_chartTopic: " + sen->MQTT_chartTopic +
       "\n    MQTT_logTopic: " + sen->MQTT_logTopic +
       "\n    MQTT_unixTlog: " + sen->MQTT_unixTlog +
       "\n    avgTemp: " + String(sen->avgTemp[0], 2) +
       "\n    avgTemp: " + String(sen->avgTemp[1], 2) +
       "\n    avgTemp: " + String(sen->avgTemp[2], 2) +
       "\n    avgTemp: " + String(sen->avgTemp[3], 2) +
       "\n    avgTemp: " + String(sen->avgTemp[4], 2) +
       "\n", FN, LN);
}

/*---------------  DS18B20 SENSOR ADDRESS TO STRING ---------------*/

String DS18B20_sensorAddrToStr(DeviceAddress addr) {
  String str = "";
  String s = "";
  for (int i = 0; i < 8; i++) {
    s = String(addr[i], HEX);
    if (s.length() == 1) s = "0" + s;
    s.toUpperCase();
    str += "0x" + s + ", ";
  }
  str = str.substring(0, str.lastIndexOf(","));
  return str;
}

/*---------------  MATCH DEVICE ADDRESS  ---------------*/

bool matchDeviceAddr(DeviceAddress address0, DeviceAddress address1) {
  for(int i = 0; i < 8; i++) {                                                  // for each sensor
    if(address0[i] != address1[i]) return false;                                // if this part of the address does not match return false
  }
  return true;                                                                  // if this is reached then the addresses match so return true
}


/*---------------  GET PROPER SENSOR STRUCT  ---------------*/

tempSensorPtr getSenStruct(DeviceAddress address) {
//  int i = 0;
  for(int i = 0; i < countOfTempSensors; i++) {                                     // for each sensor
    if(matchDeviceAddr(address, OneWireSensorsInUse[i]->address)) {             // if the addresses match then
//      OneWireSensorsInUse[i]->index = index;                                    // fill in the sensors struct index field
//      mySP(OneWireSensorsInUse[i]->name+" assigned index "+index + "\n",FN,LN); // log the index set
      return OneWireSensorsInUse[i];
    }
  }
  mySP(String(FAILED_TO_GET_SEN_STRUCT) + addrToStr(address) + "\n", FN, LN);   // if a match is not found log the error - should never happen
  return NULL;
  }


/*---------------  ONE WIRE ADDRESS TO STRING  ---------------*/

String addrToStr(DeviceAddress A) {
  char buff[42];                                                                // create a buffer to hold the addresses
  sprintf(buff,"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",       // put the address in hex into char buffer
          A[0],A[1],A[2],A[3],A[4],A[5],A[6],A[7]);
  return String(buff);
}


/*---------------  GET ONEWIRE SENSOR ADDRESS'S  ---------------*/

void getMountedOneWireAddresses(int resolution) {
  DeviceAddress A;                                                              // create a DeviceAddress variable
  tempSensorPtr sen = NULL;                                                     // pointer to temp sensor
  cntOf1WireSens = sensors.getDeviceCount();                                    // get the count of sensors on board
  if(cntOf1WireSens == 0) {
    mySP("ERROR: NO ONEWIRE SENSORS WERE FOUND.  NOT TEMPERATURE READING WILL BE ATTEMPTED VIA ONEWIRE.\n", FN, LN);
    return;
  }
  mySP("There are '" + String(cntOf1WireSens) + String(INSTALLED), FN, LN);     // log the device count
  for (int i = 0; i < cntOf1WireSens; i++) {                                    // for each sensor found
    if (sensors.getAddress(A, i)) {                                             // get its address
      sensors.setResolution(A, resolution, true);                               // set the desired temperature sensor resolution resolution
      if((sen = getSenStruct(A)) == NULL) {         // get the sensor struct
        mySP("There is a sensor but it is not one of the expected ones.\n", FN, LN);
             return;
      };
      sen->index = i;                                                           // set the structs index
     mySP(sen->name + " Sensor address: " + addrToStr(A) +                     // log the sensor, resolution, and index
           ", Resolution = " + String(int(sensors.getResolution(A))) +
           ", Index = " + String(sen->index) + ".\n", FN, LN);
    } else {
      mySP("Error fetching the #'" + String(i) + String(SEN_ADDR), FN, LN);     // else if an error occurred log it
    }
  }
}

/*---------------  TEMP CONFIG DATA TO STRING  ---------------*/

String tempCfgToStr() {
  return String(senChkIntv) + "|" + String(senLogIntv);                         // return the setable temp config data
}


/*---------------  SAVE TEMP CONFIG DATA TO DISK  ---------------*/

int writeTempCfgToDisk() {
  int len = myFileSys.writeFile(oneWireData.c_str(), tempCfgToStr().c_str(), LN);     // attempt to write the data to disk
  if(len == 0) {                                                                // if nothing was written to disk then
    mySP(String(WRITE_CFG_FILE_ERR) + oneWireData + " to disk.\n", FN, LN);     // log the error
  }
  return len;
}

/*---------------  READ TEMP CONFIG DATA FROM DISK  ---------------*/

bool readTempCfgFromDisk(String fn) {
  String s = myFileSys.readFile(fn.c_str(), true);                                            // read the file
  DUMP(s);
  int elements = countCharOccurrences(s, '|') + 1;                                      // calc the number of data elements in the file
  if(elements != TEMP_SEN_ELEM_CNT) {                                                   // if the file has the wrong number of elements in it
    mySP("ERROR: Reading file '" + fn + "' and received the wrong number of elements."+ // log the error
         " Element count is '" + String(elements) + " and should have been " +
         TEMP_SEN_ELEM_CNT + "./n", FN, LN);
    writeTempCfgToDisk();                                                             // attempt to write the existing light data to disk
    return false;                                                                       // and return to the calling routine
  }
  String arr[elements];                                                                 // create an array to hold the data
  int count = strToArray(s, '|', elements, arr);                                        // split the string into an array
  int tstChkIntv = arr[0].toInt();
  int tstLogIntv = arr[1].toInt();
  if(tstChkIntv == 0  || tstLogIntv == 0) {
    if(tstChkIntv == 0) {
      tstChkIntv = SEN_CHK_INTV;
      mySP("ERROR: tstChkIntv was set to 0 which will crash the system.  Changed to "
           + String(SEN_CHK_INTV) +"\n", FN, LN);
    }
    if(tstLogIntv == 0) {
      tstLogIntv = SEN_LOG_INTV;
      mySP("ERROR: senLogIntv was set to 0 which will crash the system.  Changed to "
           + String(SEN_LOG_INTV) +"\n", FN, LN);
   }
    writeTempCfgToDisk();
  }
  senChkIntv = tstChkIntv;
  senLogIntv = tstLogIntv;
  return true;
}

/*---------------  CUSTOM ROUND FLOAT TEMP TO INT TEMP  ---------------*/

float customRoundTemp(float newTemp, float prevTemp) {
//  Serial.println("newTemp = " + String(newTemp, 2) + ", prevTemp = " + String(prevTemp, 2));
  float decimal = newTemp - trunc(newTemp);                         // get the decimal part of the temp
//  DUMP(decimal);
  float resTemp = newTemp;                                          // holds the temp to be returned
  bool goingUp = newTemp > prevTemp;                                // is the new value on the rise
  bool goingDown = newTemp < prevTemp;                              // is the new value decreasing
//  DUMP(goingUp);
//  DUMP(goingDown);
  if(trunc(newTemp) == trunc(prevTemp)) {                           // if the int value of the two is the same then
    resTemp = newTemp;                                              // use the current temp
  }
  if(goingUp) {                                                     // if int value of new temp is greater than int value of prev temp
    if(decimal > 0.2) {                                             // if new temp decimal value great 0.2
      resTemp = newTemp;                                            // use the current temp
    } else return prevTemp;                                         // else use the previous temp
  }
  if(goingDown) {                                                   // if int value of new temp is less than int value of prev temp
    if(decimal > 0.8) {                                             // if new temp decimal value great 0.8
      resTemp = prevTemp;                                           // use the prev temp
    } else return newTemp;                                          // else use the current temp
  }
 // DUMP(resTemp);
  return resTemp;
}

/*---------------  CUSTOM ROUND FLOAT TEMP TO INT TEMP  ---------------*

float customRoundTemp(float newTemp, float prevTemp) {
  Serial.println("newTemp = " + String(newTemp, 2) + ", prevTemp = " + String(prevTemp, 2));
  float decimal = newTemp - trunc(newTemp);                         // get the decimal part of the temp
  DUMP(decimal);
  float resTemp = newTemp;                                          // holds the temp to be returned
  bool goingUp = trunc(newTemp) > trunc(prevTemp);                  // is the new value on the rise
  bool goingDown = trunc(newTemp) < trunc(prevTemp);                // is the new value decreasing
  DUMP(goingUp);
  DUMP(goingDown);
  if(trunc(newTemp) == trunc(prevTemp)) {                           // if the int value of the two is the same then
    resTemp = newTemp;                                              // use the current temp
  }
  if(goingUp) {                                                     // if int value of new temp is greater than int value of prev temp
    if(decimal > 0.2) {                                             // if new temp decimal value great 0.2
      resTemp = newTemp;                                            // use the current temp
    } else return prevTemp;                                         // else use the previous temp
  }
  if(goingDown) {                                                   // if int value of new temp is less than int value of prev temp
    if(decimal > 0.8) {                                             // if new temp decimal value great 0.8
      resTemp = prevTemp;                                           // use the prev temp
    } else return newTemp;                                          // else use the current temp
  }
  DUMP(resTemp);
  return resTemp;
}
/*---------------  CUSTOM ROUND FLOAT TEMP TO INT TEMP  ---------------*

float customRoundTemp(tempSensorPtr sen) {
  float decimal = sen->curTempF - trunc(sen->curTempF);                         // get the decimal part of the temp
  float resTemp = sen->curTempF;                                                // holds the temp tp be returned
  bool goingUp = trunc(sen->curTempF) < trunc(sen->prevTempF);                  // is the new value on the rise
  bool goingDown = trunc(sen->curTempF) < trunc(sen->prevTempF);                // is the new value decreasing

  //
  if(trunc(sen->curTempF) == trunc(sen->prevTempF)) {                           // if the int value of the two is the same then
    resTemp = sen->curTempF;                                                       // use the current temp
  }
  if(goingUp) {                                                                 // if int value of new temp is greater than int value of prev temp
    if(decimal > 0.2) {                                                         // if new temp decimal value great 0.2
      DUMP("decimal > 0.2");
      resTemp = sen->curTempF;                                                     // use the current temp
    } else return sen->prevTempF;                                               // else use the previous temp
  }
  if(goingDown) {                                                               // if int value of new temp is less than int value of prev temp
    if(decimal > 0.8) {                                                         // if new temp decimal value great 0.8
      DUMP("decimal > 0.8");
      resTemp = sen->prevTempF;                                                    // use the prev temp
    } else return sen->curTempF;                                                // else use the current temp
  }
  DUMP(sen->curTempF);
  DUMP(sen->prevTempF);
  DUMP(decimal);
  DUMP(resTemp);
  Serial.println("");
  return resTemp;
}
*/

#endif // HARDWARE_USES_DS1820
