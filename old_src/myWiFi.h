#ifndef _MY_WIFI_H_
#define _MY_WIFI_H_

//#include <c>
#include <WiFi.h>
#include <myStuff.h>

#define LOGIN_DATA_ARRAY_SIZE 4
#define MAX_RETRYS_ALLOWED 4

enum netInfoElements {CFG_SSID, CFG_SSID_PWD, CFG_TZ_ABBRV, CFG_TZ_OLSEN};

extern const String netInfoFname;                                               // name of the net information file

extern time_t accessPtConnectTime;                                              // time the access point was created
extern time_t AP_createdTime;                                                   // time the access point was created

extern void doAppWiFiConnected();                                               // send to main to do any application specific actions on wifi connect
extern const_data constData;                                                    // contains the access point name to use.  Must be declared in the main file
extern const_dataPtr constDataPtr;                                              // contains the access point name to use.  Must be declared in the main file
extern LogInData logInD1;                                                       // must be declared by the main app
extern LogInData_Ptr logInDataPtr;                                              // pointer to information used for booting

extern void aWiFiEvent(WiFiEvent_t event);
extern void logLoginDataStructValues(LogInData_Ptr data);
extern void LogInDataToStr(String &jsonOutput, LogInData_Ptr data, bool showPass);
extern void readNetInfoFromDisk(String fn);
extern int writeNetInfoToDisk(String fn);
extern void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
extern void wifi_setup();
extern void onWifiConnect(WiFiEvent_t event);
extern void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info);
extern bool manageTstWiFiConnect();
extern void startTstWiFiConnect(String ssid, String pwd);
extern void connectToWiFi(int calledBy);
extern bool connToWiFi(String ssid, String pwd, int from);
extern bool createWiFIAccesPt(int from);
extern String isIpDefined(String ip);

#endif /* _MY_WIFI_H_ */
