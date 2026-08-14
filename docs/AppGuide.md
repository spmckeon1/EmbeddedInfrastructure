# AppGuide

## Terminology

Throughout this document:

- Library refers to the EmbeddedInfrastructure library.
- Application refers to an Arduino application that consumes the Library.

## Purpose

This document defines the contract between the Application and the Library.
Applications using the Library shall satisfy the requirements defined herein.

## Application Requirements

An Application shall:

- Include the required Library header files.
- Define the required global objects.
- Provide Arduino setup() and loop().
- Invoke the required Library initialization routines.
- Implement all application-specific functionality.

Subsequent sections define each requirement in detail.

## Required Global Objects

### Fill out AppIds

The struct:
```c++
struct AppIDs {
  const char* appName;
  const char* sourceId;
  const char* accessPointName;
  const char* pageTitle;
  const char* pageHeader;
  const char* uploadPage;
};
```
Must be populated completly and it must be accomplished befor any calls to start the EI processes.

- appName: should be the name you desire to have teh app clled by.  It may have spaces
- sourceId: shouls be a short name for the app
- accessPointName: whe the app boots and cannot find a defined WiFi access point to join it will create an access point with this name. It needs to folow all the geeral networking rules for naming access points
- pageTitle: used as web page titles
- pageHeader: used as web page header
- uploadPage: the name of the applications firmware upload page

### MqttLwtPolicy
``` c++
struct MqttLwtPolicy {
    bool enabled      = true;
    String topic      = "";
    String onlineMsg  = "Online";
    String offlineMsg = "Offline";
    uint8_t qos       = 1;
    bool retain       = true;
};
```
- MqttLwtPolicy defines the MQTT Last Will and Testament (LWT) policy for the application.
- Members with default values may be left unchanged (recommended) or modified during `setup()` before the EmbeddedInfrastructure libraries are initialized and started.
- `topic` **must** be assigned a unique MQTT topic before MQTT is started.

#### Recommended Topic Naming Convention
```text
appShortName/mqtt/LWT/status
```

### MqttHeartbeatPolicy
```c++
struct MqttHeartbeatPolicy {
  bool enabled 			= true;
  uint32_t interval 	= 60000;
  uint32_t timeout  	= 180000;  // Seconds before considered offline
};
```
```text
mqttHbPolicy
````
- MqttHeartbeatPolicy defies the MQTT heartbeat policy for the application
- Members with default values may be left unchanged (recommended) or modified during setup() before the EmbeddedInfrastructure libraries are initialized and started.
- Defauts send a heartbeat to Node-Red once per minute, set message retention to false, and set enabked to false.
#### Use of this feature requres the mqttHbPolicy variable to have:
- enabled set to true

## Required Application to EmbeddedInfrastructure Actions

### MQTT Subscriptions
- The application is required to own and manage all MQTT topics used

- In the boot process before any of the library functons are called the application MUST declare the total number of subscriptions it will use.
```c++
setMaxSubCnt(uint16_t maxCnt);
```
#### Example
```c++
setMaxSubCnt(3);
```
- The individual subscriptions must be submitted individually using the below function:
```c++
bool addSubscription(const String& name, const String& topic, uint8_t qos);
```
#### Examples:
```c++ 
addSubscription("Node-Red to GPS", "nr/to/gps/#", 2);
addSubscription("App data GPS", "appData/gps/#", 2);
addSubscription("To GPS serger", "to/server/gps/#", 2);

- If more subscritions are submitted than allocated only the allocated number will be subscribled to.  If a greater nunber is submitted than used, memory will be wasted. 

```
## Subscription to startup events
Ther EI libraries provide visibility to the follow startup routimes
- SystemReady: The EI libraries have cmpleted all ther setup and strt rountines and are ready to perform
- WifiConnected: the wif systej has successfully connected
- WifiDisconnected: The wifi ystem has become disconnected
- MqttConnected: The MQTT serer has been succesfully conneted to
- MqttDisconnected: The MQTT server has been dosconnect

Your function can do take any actims yo desire but understand there will be no incoming parameters

### Function format
```cpp
eiEvents.on(EiEvent::(eventName, AppProcessToCall);
```
#### Sample
```cpp
    eiEvents.on(EiEvent::SystemReady, writeBootBanner);
    eiEvents.on(EiEvent::WifiConnected, appWifiConnected);
    eiEvents.on(EiEvent::WifiDisconnected, appWifiDisconnected);
    eiEvents.on(EiEvent::MqttConnected, appMqttConnected);
    eiEvents.on(EiEvent::MqttDisconnected, appMqttDisconnected);
    
    This is a standard standard boot banner for when system ready is excersized:
```c++
  void writeBootBanner() {
    JsonDocument doc;
    AppInfo::getAppInfo(doc, FI, COMPILE_DATE);
    logInfo(LS, GPS, "\n\n" + AppInfo::addRuntimeInfo(AppInfo::formatAppInfo(doc)) + "\n\n");
      
  }


```
...

## Required Startup Sequence
```c++
 - fillAppIDs();
 - setupHeartBeat();
 - ds18b20.sendStartupData(uint8_t oneWirePin, uint8_t expectedSensorCount);
```

### 
...

## Optional Features

...