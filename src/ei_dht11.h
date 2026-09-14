#pragma once

#include <Arduino.h>
#include <DHT.h>

#include <ei_scheduler.h>


struct Dht11Config{
  
};

struct Dht11State {
  
};

struct Dht11Stats {
  
};

class EiDht11 {
public:
    EiDht11();

    bool setup();
    bool startup();
    void evtLoop();

    float getTemperatureC();
    float getTemperatureF();
    float getRawTemp();
    float getRawTempF();
    float getHumidity();

private:
    Dht11Config _config;
    Dht11State _state;
    Dht11Stats _stats;

    RunTime _readSensor;

    DHT* _sensor = nullptr;

    bool readSensor();
};

extern EiDht11 dht11;
