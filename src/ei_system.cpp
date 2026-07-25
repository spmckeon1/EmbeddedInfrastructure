
#include <Arduino.h>
#include <ei_appPolicy.h>
#include <ei_storage.h>
#include <ei_network.h>
#include <ei_system.h>

EiSystem eiSystem;

bool EiSystem::setup() {
  // Logging - no setup() needed
  // Storage - no setup() needed
  if(!network.setup()) return false;
  // add more as available
  return true;
}

bool EiSystem::startup()
{
  logging.startup();                       // Logging destinations/config
  if(!storage.startup()) return false;      // Filesystem available
  storage.createDirIfNotExist(appDirs.configDir);
  storage.createDirIfNotExist(appDirs.dataDir);
  storage.createDirIfNotExist(appDirs.logDir);
  storage.createDirIfNotExist(appDirs.appData);
  if(!network.startup()) return false;      // Load credentials, initialize WiFi state
  return true;
}

void EiSystem::loop()
{
    // Service EmbeddedInfrastructure subsystems
}


