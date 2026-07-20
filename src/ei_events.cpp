//
//  network.cpp
//
//
//  Created by Stephen McKeon on 7/19/26.
//

#include <Arduino.h>
#include <ei_events.h>

EiEvents eiEvents;

bool EiEvents::post(EventType eventType) {
    if (_count >= MAX_EVENTS)
        return false;
    _events[_tail].type = eventType;
    _events[_tail].eventTime = millis();
    _tail = (_tail + 1) % MAX_EVENTS;
    _count++;
    return true;
}

bool EiEvents::get(Event &event)
{
    if (_count == 0)
        return false;

    event = _events[_head];

    _head = (_head + 1) % MAX_EVENTS;
    _count--;

    return true;
}

bool EiEvents::available() const
{
    return _count > 0;
}

void EiEvents::clear()
{
    _head = 0;
    _tail = 0;
    _count = 0;
}
