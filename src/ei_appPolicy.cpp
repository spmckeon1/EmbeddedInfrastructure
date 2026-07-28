
#include <Arduino.h>
#include <ei_appPolicy.h>

AppDirPolicy appDirs;

AppFnamePolicy appFnames {appDirs.dataDir + "/bootTime.json"};


AppIDs appIDs;

MqttLwtPolicy appMqttLwtPolicy;
MqttHeartbeatPolicy mqttHbPolicy;

