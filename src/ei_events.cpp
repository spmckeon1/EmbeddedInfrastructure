//
//  network.cpp
//
//
//  Created by Stephen McKeon on 7/19/26.
//

#include <Arduino.h>
#include <ei_events.h>
#include <ei_logging.h>

EiEvents eiEvents;

bool EiEvents::startup() {

  for (uint16_t i = 0;
       i < static_cast<uint16_t>(EiEvent::Count);
       i++) {

    for (uint8_t j = 0;
         j < MAX_HANDLERS_PER_EVENT;
         j++) {

      _handlers[i][j] = nullptr;
    }
  }

  return true;
}

bool EiEvents::on(EiEvent event, EiEventHandler handler) {

  uint16_t index = static_cast<uint16_t>(event);

  if (event == EiEvent::None ||
      index >= static_cast<uint16_t>(EiEvent::Count) ||
      handler == nullptr) {

    logError(
      LS,
      ET::SYSTEM,
      "Invalid event subscription request."
    );

    return false;
  }

  for (uint8_t i = 0; i < MAX_HANDLERS_PER_EVENT; i++) {

    if (_handlers[index][i] == handler) {

      logInfo(
        LS,
        ET::SYSTEM,
        "Event subscription already exists."
      );

      return true;
    }

    if (_handlers[index][i] == nullptr) {

      _handlers[index][i] = handler;

      logInfo(
        LS,
        ET::SYSTEM,
        "Event subscription added: " +
        String(eventToString(event))
      );
      return true;
    }
  }

  logError(
    LS,
    ET::SYSTEM,
    "Event subscription limit exceeded."
  );

  return false;
}

bool EiEvents::off(EiEvent event, EiEventHandler handler) {

  uint16_t index = static_cast<uint16_t>(event);

  if (event == EiEvent::None ||
      index >= static_cast<uint16_t>(EiEvent::Count) ||
      handler == nullptr) {

    return false;
  }

  for (uint8_t i = 0; i < MAX_HANDLERS_PER_EVENT; i++) {

    if (_handlers[index][i] == handler) {

      _handlers[index][i] = nullptr;

      logInfo(
        LS,
        ET::SYSTEM,
        "Event subscription removed."
      );

      return true;
    }
  }

  return false;
}

void EiEvents::notify(EiEvent event) {

  uint16_t index = static_cast<uint16_t>(event);

  if (event == EiEvent::None ||
      index >= static_cast<uint16_t>(EiEvent::Count)) {

    return;
  }

  for (uint8_t i = 0; i < MAX_HANDLERS_PER_EVENT; i++) {

    if (_handlers[index][i] != nullptr) {
      _handlers[index][i]();
    }
  }
}

const char* EiEvents::eventToString(EiEvent event) {

  switch (event) {

    case EiEvent::SystemReady:
      return "SystemReady";

    case EiEvent::TimePosixUpdated:
      return "TimePosixUpdated";

    case EiEvent::WifiConnected:
      return "WifiConnected";

    case EiEvent::WifiDisconnected:
      return "WifiDisconnected";

    case EiEvent::MqttConnected:
      return "MqttConnected";

    case EiEvent::MqttDisconnected:
      return "MqttDisconnected";

    case EiEvent::None:
      return "None";

    default:
      return "Unknown";
  }
}
