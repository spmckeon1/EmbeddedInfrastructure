#pragma once

//
//  ei_types.h
//
//
//  Created by Stephen McKeon on 7/19/26.
//

#include <Arduino.h>
#include <stdint.h>

static constexpr int STARTUP_TEMP = INT_MIN;

//======================================================
// Common Type Aliases
//======================================================

using DeviceId = uint16_t;

//======================================================
// Common Enumerations
//======================================================

enum class Source {
  NOT_YET_SET = -1,
  WEB,
  NODE_RED,
  APP_STARTUP
};



//======================================================
// Common Structures
//======================================================
/*
struct AppConsts {
    String pgTitle;
    String accessPtName;
    String pageHeader;
    String uploadPg;
    String appName;
    String appSourceId;
};

extern AppConsts appConsts;
*/
