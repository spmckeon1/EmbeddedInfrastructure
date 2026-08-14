//
//  ei_ds18b20.cpp
//
//
//  Created by Stephen McKeon on 8/11/26.
//

#include <Arduino.h>
#include <ArduinoTrace.h>

#include "ei_ds18b20.h"

EiDs18b20 ds18b20;

/*----  SETUP THE DS18B20 LIBRARY  ----*/

bool EiDs18b20::setup() {
  _oneWire = new OneWire(_config.oneWirePin);
  if (_oneWire == nullptr) return false;
  _sensors = new DallasTemperature(_oneWire);
  if (_sensors == nullptr) {
    delete _oneWire;
    _oneWire = nullptr;
    return false;
  }
  _sensorsInUse = new Sensor[_config.expectedSensorCount];
  Sensor& sensor = _sensorsInUse[0];

  Serial.println("New Sensor slot:");
  Serial.print("  active: ");
  Serial.println(sensor.active);

  Serial.print("  valid: ");
  Serial.println(sensor.valid);

  Serial.print("  index: ");
  Serial.println(sensor.index);

  Serial.print("  rawTempC: ");
  Serial.println(sensor.rawTempC);

  Serial.print("  lastTempC: ");
  Serial.println(sensor.lastTempC);
  if (_sensorsInUse == nullptr) {
    delete _sensors;
    _sensors = nullptr;
    delete _oneWire;
    _oneWire = nullptr;
    return false;
  }
  _readSensor = {IntervalType::IT_SECOND, _config.readInterval, -1};  // init the eventloop read sensore timer
  return true;
}

/*----  START UP THE DS18B20 LIBRARY  ----*/

bool EiDs18b20::startup()
{
  if (_sensors == nullptr)
        return false;

    _sensors->begin();

    if (!discoverSensors())
        return false;

    _state.initialized = true;
  
    return true;
}

/*----  THE DS18B20 EVENT LOOP  ----*/

void EiDs18b20::evtLoop()
{
  if (!_state.initialized) {
    return;
  }
  if(scheduler.isTimeToRun(_readSensor)) {
    readSensors();
  }
}

/*----  GATHER THE DS18B20 CONFGURATION DATA  ----*/

void EiDs18b20::sendStartupData(uint8_t oneWirePin, uint8_t expectedSensorCount) {
  _config.oneWirePin = oneWirePin;
  _config.expectedSensorCount = expectedSensorCount;
}

/*----  EXPLORATION TO DISCOER HOW MANY SENSORS THERE ARE  ----*/

bool EiDs18b20::discoverSensors()
{
    _state.sensorCount = _sensors->getDeviceCount();

    if (_state.sensorCount == 0) {
        logError(LS, ET::SENSOR, "No OneWire sensors were found.");
        return false;
    }


    if (_state.sensorCount != _config.expectedSensorCount) {
        logError(
            LS,
            ET::SENSOR,
            "Discovered sensor count (" + String(_state.sensorCount) +
            ") does not match expected count (" +
            String(_config.expectedSensorCount) + ")."
        );
    }

    // Start with every configured sensor inactive.
    for (uint8_t i = 0; i < _sensorCount; i++) {
        _sensorsInUse[i].active = false;
        _sensorsInUse[i].valid = false;
    }
  bool sensorFound = false;
    for (uint8_t i = 0; i < _state.sensorCount; i++) {
        DeviceAddress address;

        if (!_sensors->getAddress(address, i)) {
            logError(LS, ET::SENSOR,
                     "Unable to get OneWire sensor address.");
            continue;
        }

//        _sensors->setResolution(
//            address,
//            _config.resolution,
//            true
//        );

      Sensor* sensor = findSensor(address);

      if (sensor == nullptr) {
          logError(
              LS,
              ET::SENSOR,
              "Found unconfigured OneWire sensor: " +
              addrToStr(address)
          );
          continue;
      }

      _sensors->setResolution(
          address,
          sensor->resolution,
          true
      );

      sensor->index = i;
      sensor->active = true;
      sensorFound = true;
      
        logInfo(
            LS,
            ET::SENSOR,
            "Sensor '" + sensor->name +
            "' discovered at index " + String(sensor->index) +
            ", address = " + addrToStr(address) +
            ", resolution = " +
            String(_sensors->getResolution(address))
        );
    }

    return sensorFound;
}

/*----  FIND A SENSOR BY ITS ADDRESS  ----*/

Sensor* EiDs18b20::findSensor(const DeviceAddress& address) {
  for (uint8_t i = 0; i < _sensorCount; i++) {
    if (matchDeviceAddr(address, _sensorsInUse[i].address))
      return &_sensorsInUse[i];
  }
  return nullptr;
}
/*----  MATCH A SENSOR TO ITS ADDESS  ----*/

bool EiDs18b20::matchDeviceAddr(const DeviceAddress& address0, const DeviceAddress& address1) const {
  for (uint8_t i = 0; i < 8; i++) {
    if (address0[i] != address1[i])
      return false;
  }
  return true;
}

/*----  PUBLIC: LET THE APP ADD A SENSOR  ----*/

bool EiDs18b20::addSensor(EiDs18b20Sensor& sensor)
{
    logInfo(
        LS,
        ET::SENSOR,
        "Adding sensor '" + sensor.name +
        "', address = " + addrToStr(sensor.address)
    );

    if (_sensorCount >= _config.expectedSensorCount) {
        sensor.result = SensorAddResult::SensorLimitReached;
        return false;
    }

    Sensor& eiSensor = _sensorsInUse[_sensorCount];

    eiSensor.appSensor = &sensor;
    eiSensor.name = sensor.name;
    eiSensor.appId = sensor.appId;
    eiSensor.temperatureUnit = sensor.temperatureUnit;

    for (uint8_t i = 0; i < 8; i++)
        eiSensor.address[i] = sensor.address[i];

    if (sensor.temperatureUnit == TemperatureUnit::Fahrenheit)
        eiSensor.hysteresis = sensor.hysteresis / 1.8f;
    else
        eiSensor.hysteresis = sensor.hysteresis;

    eiSensor.resolution = sensor.resolution;

    sensor.ds18b20Index = _sensorCount;
    sensor.result = SensorAddResult::Success;

    _sensorCount++;

    return true;
}

/*----  CONVERT A SENSOR ADDRESS TO A STRING  ----*/

String EiDs18b20::addrToStr(const DeviceAddress& address) const {
char buff[42];
  snprintf(buff, sizeof(buff),
           "0x%02X,0x%02X,0x%02X,0x%02X,"
           "0x%02X,0x%02X,0x%02X,0x%02X",
           address[0], address[1], address[2], address[3],
           address[4], address[5], address[6], address[7]);
  return String(buff);
}

/*----  READ A DS18B20 SENSOR  ----*/

bool EiDs18b20::readSensors()
{
    if (!_state.initialized)
        return false;

    _sensors->requestTemperatures();

    for (uint8_t i = 0; i < _sensorCount; i++) {

        Sensor& sensor = _sensorsInUse[i];

        if (!sensor.active)
            continue;

        float temperature = _sensors->getTempC(sensor.address);

        if (temperature == DEVICE_DISCONNECTED_C) {
            sensor.valid = false;
            _stats.failedReads++;
            continue;
        }

        sensor.lastTempC = sensor.rawTempC;
        sensor.rawTempC = temperature;
        sensor.valid = true;
        sensor.lastReadingTime = millis();

        bool rptTempChanged = false;

        if (sensor.temperatureUnit == TemperatureUnit::Fahrenheit) {

            float temperatureF =
                sensor.rawTempC * 1.8f + 32.0f;

            if (sensor.rptHysTempF == STARTUP_TEMP) {
                sensor.rptHysTempF = roundf(temperatureF);
                rptTempChanged = true;
            }
            else {
                int previous = sensor.rptHysTempF;

                sensor.rptHysTempF = applyHysteresisF(
                    temperatureF,
                    sensor.rptHysTempF,
                    sensor.hysteresis
                );

                rptTempChanged =
                    sensor.rptHysTempF != previous;
            }
        }
        else {

            if (sensor.rptHysTempC == STARTUP_TEMP) {
                sensor.rptHysTempC =
                    roundf(sensor.rawTempC * 10.0f) / 10.0f;

                rptTempChanged = true;
            }
            else {
                float previous = sensor.rptHysTempC;

                sensor.rptHysTempC = applyHysteresisC(
                    sensor.rawTempC,
                    sensor.rptHysTempC,
                    sensor.hysteresis
                );

                rptTempChanged =
                    sensor.rptHysTempC != previous;
            }
        }

        if (rptTempChanged && sensor.appSensor != nullptr)
            sensor.appSensor->rptTempUpdated = true;
    }

    _stats.readCount++;

    return true;
}

/*----  SET THE SENSOR READ INTERVAL  ----*/

void EiDs18b20::setReadInterval(uint32_t interval) {
  _config.readInterval = interval;
  _readSensor.intvToRun = interval;
}

/*----  SET THE HYSTERESIS TEMP SPREAD  ----*/

void EiDs18b20::setHysteresis(EiDs18b20Sensor& sensor) {
  Sensor& eiSensor = _sensorsInUse[sensor.ds18b20Index];
  eiSensor.hysteresis = sensor.hysteresis;
}
/*----  GET A SENSORS RAW ºC TEMPERATURE  ----*/

float EiDs18b20::getRawTemp(uint8_t sensorId)
{
    if (sensorId >= _sensorCount)
        return NAN;

    return _sensorsInUse[sensorId].rawTempC;
}


/*----  GET A SENSOR'S RAW ºF TEMPERATURE  ----*/

float EiDs18b20::getRawTempF(uint8_t sensorId)
{
    if (sensorId >= _sensorCount)
        return NAN;

    return (_sensorsInUse[sensorId].rawTempC * 9.0 / 5.0) + 32.0;
}

/*----  CHECK TO SEE IF THE ºC VALUE HAS CHANGED ENOUGH TO ACTUALLY CHANGE WHAT IS REPORTED  ----*/

float EiDs18b20::applyHysteresisC(float newTemp,
                                  float currentTemp,
                                  float hysteresis)
{
    float nextUp = currentTemp + 0.1f;
    float nextDown = currentTemp - 0.1f;

    if (newTemp >= nextUp + hysteresis)
        return nextUp;

    if (newTemp <= nextDown - hysteresis)
        return nextDown;

    return currentTemp;
}

/*----  CHECK TO SEE IF THE ºF VALUE HAS CHANGED ENOUGH TO ACTUALLY CHANGE WHAT IS REPORTED  ----*/

float EiDs18b20::applyHysteresisF(float newTemp,
                                  float currentTemp,
                                  float hysteresis)
{
    float nextUp = currentTemp + 1.0f;
    float nextDown = currentTemp - 1.0f;

    if (newTemp >= nextUp + hysteresis)
        return nextUp;

    if (newTemp <= nextDown - hysteresis)
        return nextDown;

    return currentTemp;
}

/*----  SEND THE COSUMER THE LATEST HYSTERESIS READING IN ºC  ----*/

float EiDs18b20::getHysteresisTempC(uint8_t index)
{
    return _sensorsInUse[index].rptHysTempC;
}

/*----  SEND THE COSUMER THE LATEST HYSTERESIS READING IN ºF  ----*/

float EiDs18b20::getHysteresisTempF(uint8_t index)
{
    return _sensorsInUse[index].rptHysTempF;
}
