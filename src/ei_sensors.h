#pragma once
//
//  ei_sensors.h
//  
//
//  Created by Stephen McKeon on 9/10/26.
//

#ifdef SENSOR_USES_DS18B20
  #include <ei_ds18b20.h>
#elif defined(SENSOR_USES_DHT11)
  #include <ei_dht11.h>
#endif

#include <ei_scheduler.h>

class Sensors
{
public:

  String getTemp(uint8_t appId, const char* senName, const char* route);
  
private:

  
};

extern Sensors sensors;
