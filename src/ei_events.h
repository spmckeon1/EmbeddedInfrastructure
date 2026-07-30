#pragma once

//
//  network.cpp
//
//
//  Created by Stephen McKeon on 7/19/26.
//

#include "ei_types.h"

enum class EiEvent : uint16_t {
  None = 0,

  // System
  SystemReady,

  // Time
  TimePosixUpdated,

  // WiFi
  WifiConnected,
  WifiDisconnected,

  // MQTT
  MqttConnected,
  MqttDisconnected,

  Count
};

using EiEventHandler = void (*)();

class EiEvents {
public:
    bool startup();

    bool on(EiEvent event, EiEventHandler handler);
    bool off(EiEvent event, EiEventHandler handler);

    void notify(EiEvent event);

private:
    EiEventHandler _handlers[static_cast<uint16_t>(EiEvent::Count)] = {};
};

extern EiEvents eiEvents;
