//
//  ei_sensors.cpp
//  
//
//  Created by Stephen McKeon on 9/10/26.
//

#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include <ei_appFramework.h>

#ifdef SENSOR_USES_DS18B20
  #include <ei_ds18b20.h>
#elif defined(SENSOR_USES_DHT)
  #include <ei_dht.h>
#endif
#include <ei_ds18b20.h>

#include <ei_logging.h>
#include <ei_events.h>

#include "ei_sensors.h"

inline constexpr const char SENSORS[] = "SENSORS";

Sensors sensors;

/*-----  GET THE CURENT TEMPERATURE AND RETURN IT IN A JSON STRING  -----*/

String Sensors::getTemp(uint8_t appId, const char* senName, const char* route) {
  int curTemp = ds18b20.getHysteresisTempF(ds18b20.getSensorId(appId));
  JsonDocument data;
  data["schema"] = "pcbTemperature.v1";
  data["name"] = senName;
  data["temperature"] = curTemp;

  return appFramework.buildJsonAppMqttMsg(String(route), "TEMPERATURE", data.as<JsonObjectConst>());
}

