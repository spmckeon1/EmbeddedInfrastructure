#pragma once

//
//  ei_ds18b20.h
//  
//
//  Created by Stephen McKeon on 8/11/26.
//

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <ei_logging.h>
#include <ei_scheduler.h>

static constexpr uint8_t INVALID_APP_ID = 0xFF;
static constexpr uint8_t INVALID_SENSOR_INDEX = 0xFF;
static constexpr int STARTUP_TEMP = INT_MIN;

enum class SensorAddResult : uint8_t {
    NotProcessed,
    Success,
    SensorLimitReached,
    InvalidAppId
};

enum class TemperatureUnit : uint8_t {
    Celsius,
    Fahrenheit
};
                            //  USED BY THE APPS TO DEFINE THEIR SENSOR...ONE PER SENSOR
struct EiDs18b20Sensor {
  String name = "";
  uint8_t appId = INVALID_APP_ID;
  TemperatureUnit temperatureUnit = TemperatureUnit::Celsius;
  DeviceAddress address;
  float hysteresis = 0.3;
  uint8_t resolution = 12;
  uint8_t ds18b20Index = INVALID_SENSOR_INDEX;
  bool rptTempUpdated = false;
  SensorAddResult result = SensorAddResult::NotProcessed;
};

struct Sensor {
  String name;
  DeviceAddress address;
  uint8_t appId = INVALID_APP_ID;
  int index = -1;
  bool active = false;
  bool valid = false;
  uint8_t resolution = 12;
  float hysteresis = 0.3;
  float rawTempC = 0.0;
  float lastTempC = 0.0;
  float rptHysTempC = STARTUP_TEMP;
  int   rptHysTempF = STARTUP_TEMP;
  TemperatureUnit temperatureUnit = TemperatureUnit::Celsius;
  uint32_t lastReadingTime = 0;
  EiDs18b20Sensor* appSensor = nullptr;
};

struct Config {
    uint8_t oneWirePin;
    uint8_t expectedSensorCount = 0;
//    uint8_t resolution = 12;     // MOVED TO struct Sensor 08-13-2026
    uint32_t readInterval = 5;
//    float hysteresis = 0.3;       // MOVED TO struct Sensor 08-13-2026
};

struct State {
    bool initialized = false;
    bool readingEnabled = true;
    uint8_t sensorCount = 0;
};

struct Stats {
  uint32_t readCount = 0;
  uint32_t failedReads = 0;
};

class EiDs18b20 {
public:
  bool setup();
  bool startup();
  void evtLoop();
  void sendStartupData(uint8_t oneWirePin, uint8_t expectedSensorCount);
  bool addSensor(EiDs18b20Sensor& sensor);
  void setReadInterval(uint32_t interval);
  void setHysteresis(EiDs18b20Sensor& sensor);
  float getRawTemp(uint8_t sensorId);
  float getRawTempF(uint8_t sensorId);
  float getHysteresisTempC(uint8_t index);
  float getHysteresisTempF(uint8_t index);

private:

  Config _config;
  State _state;
  Stats _stats;
  RunTime _readSensor;
  static constexpr uint8_t INVALID_SENSOR_ID = 0xFF;

  OneWire* _oneWire = nullptr;
  DallasTemperature* _sensors = nullptr;
  uint8_t _sensorCount = 0;
  Sensor* _sensorsInUse = nullptr;

  bool readSensors();
  bool sensorConfigured(const DeviceAddress& address) const;
  
  bool discoverSensors();
  Sensor* findSensor(const DeviceAddress& address);
  bool matchDeviceAddr(const DeviceAddress& address0, const DeviceAddress& address1) const;
  String addrToStr(const DeviceAddress& address) const;
  float applyHysteresisC(float newTemp,
                                    float currentTemp,
                                    float hysteresis);
  float applyHysteresisF(float newTemp,
                                    float currentTemp,
                         float hysteresis);

};

extern EiDs18b20 ds18b20;
