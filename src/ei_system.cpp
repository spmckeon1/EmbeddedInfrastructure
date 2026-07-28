
#include <Arduino.h>
#include <ei_appPolicy.h>
#include <ei_storage.h>
#include <ei_mqtt.h>
#include <ei_network.h>
#include <ei_time.h>
#include <ei_system.h>

EiSystem eiSystem;

bool EiSystem::bootStrap() {
  logging.startup();                              // Logging destinations/config - MUST BE FIRST TO ALLOW LOGGING TO WORK
  if(!storage.startup()) return false;            // Filesystem available
  storage.createDirIfNotExist(appDirs.libCfgDir);
  storage.createDirIfNotExist(appDirs.dataDir);
  storage.createDirIfNotExist(appDirs.logDir);
  storage.createDirIfNotExist(appDirs.appData);
  return true;
}

bool EiSystem::setup() {
  // Logging - no setup() needed
  // Storage - no setup() needed
  if(!network.setup()) return false;
  if(!eiTime.setup()) return false;
  if(!mqtt.setup()) return false;
  // add more as available
  return true;
}

bool EiSystem::startup()
{
  if(!network.startup()) return false;      // Load credentials, initialize WiFi state
  if(!mqtt.startup()) return false;
  return true;
}

void EiSystem::loop() {
  static constexpr uint8_t NUM_SUBSYSTEMS = 5;
  static uint8_t nextSubsystem = 0;
  
  // Service EmbeddedInfrastructure subsystems
  
  if(nextSubsystem == 0) {
    nextSubsystem = (nextSubsystem + 1) % NUM_SUBSYSTEMS;
    if(logging.evtLoop()) {
      return;
    }
  }
  if(nextSubsystem == 1) {
    nextSubsystem = (nextSubsystem + 1) % NUM_SUBSYSTEMS;
    if(storage.evtLoop()) {
      return;
    }
  }
  if(nextSubsystem == 2) {
    nextSubsystem = (nextSubsystem + 1) % NUM_SUBSYSTEMS;
    if(network.evtLoop()) {
      return;
    }
  }
  if(nextSubsystem == 3) {
    nextSubsystem = (nextSubsystem + 1) % NUM_SUBSYSTEMS;
    if(eiTime.evtLoop()) {
      return;
    }
  }
  if(nextSubsystem == 4) {
    nextSubsystem = (nextSubsystem + 1) % NUM_SUBSYSTEMS;
    if(mqtt.evtLoop()) {
      return;
    }
  }
}

