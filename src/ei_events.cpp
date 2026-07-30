//
//  network.cpp
//
//
//  Created by Stephen McKeon on 7/19/26.
//

#include <Arduino.h>
#include <ei_events.h>

EiEvents eiEvents;

bool EiEvents::startup() {
  for (uint16_t i = 0; i < static_cast<uint16_t>(EiEvent::Count); i++)
    _handlers[i] = nullptr;
  return true;
}

bool EiEvents::on(EiEvent event, EiEventHandler handler) {
  uint16_t index = static_cast<uint16_t>(event);
  if (event == EiEvent::None || index >= static_cast<uint16_t>(EiEvent::Count))
    return false;
  _handlers[index] = handler;
  return true;
}

bool EiEvents::off(EiEvent event, EiEventHandler handler) {
  uint16_t index = static_cast<uint16_t>(event);
  if (event == EiEvent::None ||
    index >= static_cast<uint16_t>(EiEvent::Count))
    return false;
  if (_handlers[index] != handler)
    return false;
  _handlers[index] = nullptr;
  return true;
}

void EiEvents::notify(EiEvent event) {
  uint16_t index = static_cast<uint16_t>(event);
  if (event == EiEvent::None ||
    index >= static_cast<uint16_t>(EiEvent::Count))
    return;
  if (_handlers[index] != nullptr)
    _handlers[index]();
}
