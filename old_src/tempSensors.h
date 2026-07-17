/*
REMOVED 'unsigned long logIntv' FROM THE TempSensor STRUCT.  IT IS NOT BEIG USED AND GETS CIONFUSED EASILY WITH ckIntv
*/

#if __has_include("myConfig.h")
  #include "myConfig.h"
#else
  // 🎯 CRITICAL DEFENSIVE SANITY TRAP
  // If the project folder is missing myConfig.h, stop compilation immediately and tell the user why!
  #error "CRITICAL CONFIG ERROR: 'myConfig.h' is missing from your project folder. Please copy a template copy into your local sketch directory."
#endif


#ifndef _TEMP_SENSORS_H_                                                        // if TEMP_SENSORS_H_ is not defined then (skips the file if it is defined)
#define _TEMP_SENSORS_H_                                                        // define it

#ifdef HARDWARE_USES_DS1820

#warning "Including tempSensors.h"

#include <DallasTemperature.h>
#include <ESPAsyncWebServer.h>
#include <myStuff.h>

#define TEMP_TIME_F myTZ.dateTime("Y-m-d H:i:s-T")                              // time format for temp log file
#define TEMP_SEN_ELEM_CNT 2                                                     // count of temp sensor items in the file saved to disk
#define SEN_LOG_INTV 5
#define SEN_CHK_INTV 2000
#define SenErrorNeg127C " has either not been found or is not responding.  Check wiring, specifically the data line connection to ground or VCC.**************************\n"
#define SenErr85C " is powered but not properly receiving the CONVERT T command.**************************\n"

extern const String oneWireData;                                                // name of the temperature sensor data file
extern DallasTemperature sensors;                                               // define the DallasTemperature sensor variable
extern bool downloadingFile;                                                    // javascript 'fetch()' is downloading a file...need not to run any 'DallasTemperature' routines while downloading
extern int8_t cntOf1WireSens;                                                   // the number of one wire sensors installed on this server
extern bool DnLoadMsgSent;                                                      // has the file being downloaded message been logged
extern time_t senChkIntv;                                                       // time interval in milliseconds to check sensor temps
extern int senLogIntv;                                                          // time in minutes between logs information sent


//ROUTINES/VARIABLES THAT MUST BE CREATED IN THE MAIN CODE FILE
extern void sendTemp(tempSensorPtr sen, bool force);
extern void sendLogTemps(tempSensorPtr sen);
extern tempSensorPtr OneWireSensorsInUse[];                                     // array of temp sensor struct pointers

#define DOWNLOAD_ERROR_MSG "In readDS18B20Temperature() and waiting for download to complete.\n"
#define FAILED_TO_GET_SEN_STRUCT "ERROR - Failed to find matching sensor struct: "
#define WRITE_CFG_FILE_ERR "UNKNOWN ERROR: Failed to write the file '"
#define INSTALLED "' DS18B20 sensors installed.\n"
#define SEN_ADDR "' temperature sensor address\n"
#define AVG_TEMP_ARRAY_SIZE 5


// FORWARD ROUTINE DECLARATIONS
extern void doTempStartUp(int resolution);
extern void ds18b20Events();
extern void areOneWireTempsNeeded();
extern float avgDS18B20Temps(tempSensorPtr sen, float curT);
extern void tempSenActNeeded();
extern bool readDS18B20Temperature();
extern void printTempSenStruct(tempSensorPtr sen);
extern String DS18B20_sensorAddrToStr(DeviceAddress addr);
extern bool matchDeviceAddr(DeviceAddress address0, DeviceAddress address1);
extern tempSensorPtr getSenStruct(DeviceAddress address);
extern String addrToStr(DeviceAddress A);
extern void getMountedOneWireAddresses(int resolution);
extern String tempCfgToStr();
extern int writeTempCfgToDisk();
extern bool readTempCfgFromDisk(String fn);
float customRoundTemp(float newTemp, float prevTemp);

#endif // HARDWARE_USES_DS1820
#endif /* _TEMP_SENSORS_H_ */
