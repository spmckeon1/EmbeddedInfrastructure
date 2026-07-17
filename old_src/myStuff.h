/*
REMOVED 'unsigned long logIntv' FROM THE TempSensor STRUCT.  IT IS NOT BEIG USED AND GETS CONFUSED EASILY WITH ckIntv
REMOVED senChkIntv and senLogIntv from TempSensor STRUCT and added them as global vars to the Tempsensore.cpp and .h files.  The thought is that all sensors will
 use the same time values.
*/

#ifndef _MY_STUFF_H_
#define _MY_STUFF_H_

#include <DallasTemperature.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define CONST_DATA_ARRAY_SIZE 7                               // how many elements in the constant array

enum source {
  NOT_YET_SET = -1,       // deviceData record has never been updated since boot
  WEB,                    // deviceData record last updated by an WEB_PG ACTION
  SWITCH,                 // deviceData record last updated by an SWITCH ACTION
  NODERED,                // deviceData record last updated by an Node-Red ACTION
  REV_SHADE_DIR,          // deviceData record last updated by an CMD TO REVERSE SHADE DIRECTION ACTION
  _TIMER,                 // deviceData record last updated by a _TIMER ACTION
  SW_AFTER_HOLDDOWN,      // deviceData record last updated by an SW after being held down
  SECOND_PUSH,            // deviceData record last updated by a 2nd push on the switch (shade automation)
  PK_BK_RELEASE,          // brake came on so reversing shade to go up to brake off stop (shade automation)
  REV_FOR_PK_BK_CHG,      // reverse for parking brake change (shade automation)
  SH_TO_FAR_DN_PKBK_REL,  // the device is to far down when the parking brake was released (shade automation)
  APP_STARTUP             // application start up action
};

typedef struct  {                                   // info on connected web pages
  int id = -1;                                      // the web socket id of this page
  String ipAddr = "";                               // web page ip address
  String type = "";                                 // what kind of web page, main, Debug, Update
  String CUID = "";                                 // web page unique id (this is maintained until the client refreshes their page or the server reboots)
  AsyncWebSocketClient *client = NULL;              // web pages AsyncWebSocketClient object
  bool logTail = false;                             // does this page want to tail the syslog
} ConnectedClients, *ConnectedClients_PTR;

typedef struct {
  bool changed;                                     // has the data changed since last checked
  String ssid;                                      // wifi ssid to use
  String ssidPwd;                                   // wif ssid password
  String tzAbbrv;                                   // time xone abbreviation
  String olsenTzName;                               // Olsen name of the time zone the server is in
  String posixTzRule;                               // contains the tz POSIX string
} LogInData, *LogInData_Ptr;

typedef struct {
  String name;                                        // name of logs
  int arrSize;                                        // max elements to handle at once to insure the wait timer does not get tripped
  String type;                                        // what type of files, syslog, temperature...
  String logFnames;                                   // string of log file names
  String path;                                        // path to the disk loction of these files
  String dataFileName;                                // the data file containing the list of files
  time_t cutOffSecs;                                  // UNIX seconds of the oldest file to keep.  Older ones will be deleted
  char delineator;                                    // what is the delinerator used to seperate file names in the logFname string
  bool complete;                                      // have all the log files been processed
  int filesDeleted;                                   // holds the number of files that were deleted the last time rotate logs was run
  int index;                                          // last item done
}fileRotateData, *fileRotateData_ptr;

typedef struct {
  const String pgTitle;
  const String accessPtName;
  const String pageHeader;
  const String uploadPg;
  const String appName;
  const String appShortName;
  const int const_dataArraySize;
} const_data, *const_dataPtr;

enum cycleType {
  CT_DAY,
  CT_HOUR,
  CT_MINUTE,
  CT_SECOND
};

typedef struct {
  int intvToRun;                          // the time interval to run something
  int lastRun;                            // minute in the day this was last run
  cycleType type;                         // cycle type - day, hour, minute, second
} runTime, *runTimePtr;

// END from commonItems_ESP32 -------------------

// from myMQTT.h

typedef struct {
    int portNo;
    IPAddress host;
    String brokerUser;
    String brokerPwd;
} MQTT_host, *MQTT_hostPtr;

// END from myMQTT.h

// from mySD.h

typedef struct {
  String fName;                             // name of file to put the directory contents into
  String dirName;                           // name of the directory to get this list of files from
  String header;                            // the fixed part of the file name (this is always the same for each days file)
  File root;
  int depth;                                // directory depth to look (restricted to zero at this time)
  bool inProcess;                           // is the build actually in process
  bool complete;                            // if the build of the file complete
} listDirStruct, *listDirStruct_ptr;

extern String startUpBootLogStr;            // string to hold initial loog info until the SD fie system is mounted

// END from mySD.h

// from myTempSensors.h

  struct TempSensor {
    String name;                              // name of the sensor
    int index;                                // index of sender used by the OneWire sensor library
    String msg;                               // holds the data being sent to the log file
    float curTempC;                           // sensor temperature at last reading ºC
    float curTempF;                           // sensor temperature at last reading ºF
    float prevTempF;                          // sensor temperature at the last time the temp was sent to clients
    DeviceAddress address;                    // device address
    AsyncWebSocketClient *client;             // client that requested the PCB temperature reading, may be NULL for going to all clients
    String msgHeader;                         // header for the messages going to a web page with the sensor temp in them
    String MQTT_gaugeTopic;                   // topic for sending temp gauge msgs to mqtt
    String MQTT_chartTopic;                   // topic for sending chart temp msgs to mqtt
    String MQTT_logTopic;                     // topic for sending logs to the MQTT server
    String MQTT_unixTlog;                     // topic for sending the UNIX time temperature log entry
    float avgTemp[5];                         // used in getting the average sensor temp
    runTimePtr failedTempRead;                // notification timerfor is sensor doesn't respond to a read request
};
  typedef TempSensor* tempSensorPtr;

// END from myTempSensors.h

typedef struct {
  int count;
  char c;
  int width;
} PCHAR, *PCHAR_PTR;

typedef struct {
  String MQTT_FROM_NODE_RED;
  String SERVER_ONLINE;
  String MQTT_PCB_GAUGE_TOPIC;
  String MQTT_PCB_CHART_TOPIC;
  String MQTT_IN_PCB_TEMP_LOG;
  String MQTT_TOPIC_SEND_CFG_ITEMS;
  String SERVER_UP_TIME;
  String SYSLOG_TOPIC;
  String MQTT_SERVER_GET_INFO_RESPONSE;
  String MQTT_DATA_VAR_REQ_RESP;
  String GET_CFG_ITEMS;
  String INCOMING_CFG_DATA_UPDATE;
  String MQTT_REQ_ABOUT_INFO;
  String MQTT_REBOOT_NOW;
  String MQTT_REQ_DATA_VARS;
} Std_Topics_To_From_NodeRed, *STTFNR_ptr;

#endif /* _MY_STUFF_H_ */

