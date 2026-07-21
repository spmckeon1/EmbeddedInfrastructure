#pragma once

// Module:
//     Application Policy
#include <ei_types.h>

struct EiAppPolicy {
    String configDir = "/libCfg";
    String dataDir   = "/libData";
    String logDir    = "/libLog";
};

extern EiAppPolicy eiAppPolicy;
