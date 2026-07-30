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
    bool enabled      = false;
    String topic      = "";
    uint32_t interval = 60000;
    bool retain       = false;
};
```
```text
mqttHbPolicy
````
- MqttHeartbeatPolicy defies the MQTT heartbeat policy for the application
- Members with default values may be left unchanged (recommended) or modified during setup() before the EmbeddedInfrastructure libraries are initialized and started.
- Defauts send a heartbeat to Nioe-Red once per minute, set message retention to false, and set enabked to false.
#### Use of this feature requres the mqttHbPolicy variable to have:

- enabled set to true
- `topic` **must** be assigned a unique MQTT topic before MQTT is started.

##### Recommended Topic Naming Convention
```text
appShortName/to/nr/mqtt/hb
```

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
...

## Required Startup Sequence

...

## Optional Features

...