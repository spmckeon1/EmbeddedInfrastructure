#pragma once

//
//  network.cpp
//
//
//  Created by Stephen McKeon on 7/19/26.
//

#include "ei_types.h"

enum class EventType : uint16_t {
    None = 0,
    TimePosixUpdated,                   // Time

    // Future...
};

struct Event {
    EventType type = EventType::None;
    uint32_t eventTime = 0;
};

class EiEvents {
  bool begin();
  bool post(EventType eventType);
  bool get(Event &event);
  bool available() const;
  void clear();
private:
  static constexpr uint8_t MAX_EVENTS = 16;
  Event _events[MAX_EVENTS];
  uint8_t _head = 0;
  uint8_t _tail = 0;
  uint8_t _count = 0;
};

extern EiEvents eiEvents;
