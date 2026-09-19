
#pragma once

#include <Arduino.h>
#include <DHT.h>

#include <ei_scheduler.h>
#include <ei_types.h>


struct DhtConfig {
  uint8_t pin;
  uint8_t type;
  uint32_t readInterval;
  float hysteresis = 0.3;
};

struct DhtState {
  bool initialized = false;
  bool valid = false;
  float temperatureC = NAN;
  float humidity = NAN;
  float rptHysTempC = STARTUP_TEMP;
  int   rptHysTempF = STARTUP_TEMP;
  uint32_t lastReadingTime = 0;
};

struct DhtStats {
    uint32_t readCount = 0;
    uint32_t failedReads = 0;
};

class EiDht {
public:
  EiDht();

  bool setup();
  bool startup();
  bool evtLoop();

  void setConfig(uint8_t pin, uint8_t type, uint32_t readInterval);
  void setReadInterval(uint32_t interval);
  float getHysteresisTemp();
  float getHysteresisTempF();
  
  float getTemp();
  float getTempF();
  float getHumidity();


private:
  DhtConfig _config;
  DhtState _state;
  DhtStats _stats;

  RunTime _readSensor;

  DHT* _sensor = nullptr;

    bool readSensor();
};

extern EiDht dht;
