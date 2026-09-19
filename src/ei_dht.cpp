//
//  ei_dht.cpp
//  
//
//  Created by Stephen McKeon on 9/10/26.
//
/*
  USERS OF THIS LIBRARY MUST 
 */

#include <ei_utilities.h>

#include "ei_dht.h"


EiDht dht;

EiDht::EiDht()
{
}

/*--- SETUP THE DHT SYSTEM  ---*/

bool EiDht::setup() {
  _sensor = new DHT(_config.pin, _config.type);
  if (_sensor == nullptr)
      return false;
  _sensor->begin();
  _readSensor = {IntervalType::IT_SECOND, _config.readInterval, -1};
  _state.initialized = true;
  return true;
}

/*--- STARTUP THE DHT SYSTEM  ---*/

bool EiDht::startup() {
//  CURRENTLY NO STARTUP ACTIONS NEEDED
  return true;
}

  
/*--- EXECUTE THE DHT EVET LOOP  ---*/

bool EiDht::evtLoop() {
  if (!_state.initialized) {
    return false;
  }
  if (scheduler.isTimeToRun(_readSensor)) {
    readSensor();
  }
  return true;
}

/*--- SETUP THE DHT CNFIGURATION  ---*/

void EiDht::setConfig(uint8_t pin, uint8_t type, uint32_t readInterval) {
  _config.pin = pin;
  _config.type = type;
  _config.readInterval = readInterval;
  _readSensor.intvToRun = readInterval;
}

/*--- CHANGE THE DHT SENSOR READ INTERVAL  ---*/

void EiDht::setReadInterval(uint32_t interval) {
  _config.readInterval = interval;
  _readSensor.intvToRun = interval;
}

/*--- READ THE DHT SENSOR  ---*/

bool EiDht::readSensor() {
    if (!_state.initialized)
        return false;
    float temperatureC = _sensor->readTemperature();
    float humidity = _sensor->readHumidity();
    if (isnan(temperatureC) || isnan(humidity)) {
        _state.valid = false;
        _stats.failedReads++;
        return false;
    }
    _state.temperatureC = temperatureC;
    _state.humidity = humidity;
    float temperatureF = temperatureC * 1.8f + 32.0f;
    _state.rptHysTempF = TEMP::applyHysteresisF(temperatureF, _state.rptHysTempF, _config.hysteresis);
    _state.valid = true;
    _state.lastReadingTime = millis();
    _stats.readCount++;

    return true;
}

//*--- GET THE DHT SENSOR TEMPERATURE ºC  ---*/

float EiDht::getTemp() {
  return _state.temperatureC;
}

/*--- GET THE DHT SENSOR TEMPERATURE ºF  ---*/

float EiDht::getTempF() {
  return _state.temperatureC * 1.8f + 32.0f;
}

/*--- GET THE DHT SENSOR HUMDTY  ---*/

float EiDht::getHumidity() {
  return _state.humidity;
}

/*--- GET THE DHT SENSOR HYSTERSIS TEMP ºC  ---*/

float EiDht::getHysteresisTemp() {
  return getTemp();
}

/*--- GET THE DHT SENSOR HYSTERSIS TEMP ºF  ---*/

float EiDht::getHysteresisTempF() {
  return getTempF();
}
