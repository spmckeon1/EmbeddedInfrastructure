#pragma once

// Module:
//     Application Policy
#include <ei_types.h>

struct AppDirPolicy {
  String configDir = "/libCfg";
  String dataDir   = "/libData";
  String logDir    = "/libLog";
  String appData   = "/appData";
};

extern AppDirPolicy appDirs;
