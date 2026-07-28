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

...

## Required Startup Sequence

...

## Optional Features

...