#pragma once
//
//  ei_sensors.h
//  
//
//  Created by Stephen McKeon on 9/10/26.
//

#ifdef SENSOR_USES_DS18B20
  #include <ei_ds18b20.h>
#elif defined(SENSOR_USES_DHT)
  #include <ei_dht.h>
#endif

#include <ei_scheduler.h>

#pragma once

#ifdef SENSOR_USES_DS18B20
  #include <ei_ds18b20.h>
#elif defined(SENSOR_USES_DHT)
  #include <ei_dht.h>
#endif

#include <ei_scheduler.h>

class Sensors
{
public:
  bool setup();
  bool startup();
  void evtLoop();

//  float getTemp();                 // DHT / single sensor
//  float getTemp(uint8_t sensorId); // DS18B20 / selected sensor

  float getTemp();                  // DHT / single sensor
  float getTempF(uint8_t sensorId); // DS18B20 / selected sensorwhat si teh
  String getTemp(uint8_t appId, const char* senName, const char* route);
  
  float getHysteresisTempC();  // Hysteresis temperature, °C
  float getHysteresisTempF();  // Hysteresis temperature, °F

  void setHysteresis(float hysteresis);

private:
  float _hysteresis = 0.0f;
  float _hysteresisTempC = 0.0f;
  float _hysteresisTempF = 0.0f;

  RunTime _readSensor;
};

extern Sensors sensors;
