# EmbeddedInfrastructure Component Registry

This document defines every architectural component within EmbeddedInfrastructure.

For each component, it describes:
- Purpose
- Responsibilities
- Not Responsible For
- Primary Interfaces
- Dependencies
- Future Considerations

---

# Logging

## Purpose

Provides a centralized logging service for all framework and application components.

## Responsible For

- Log generation
- Log formatting
- Log transport
- Log buffering
- Log destinations
- Log filtering
- Log levels

## Not Responsible For

- Dashboard display
- Event analysis
- Searching
- Alerting
- Long-term log storage policy

## Public Interfaces

```cpp
log.info(...)
log.warn(...)
log.error(...)
```

## Dependencies

## Future Considerations


None

---

# MQTT

## Purpose

Provides reliable MQTT communication for applications and framework components.

## Responsible For

- Connection management
- Reconnection
- Publish
- Subscribe
- Topic management
- Last Will
- Message routing

## Not Responsible For

- Deciding what messages to publish
- Application data
- Device state

## Primary Interfaces

```cpp
mqtt.publish(...)
mqtt.subscribe(...)
```

## Dependencies

- Network

## Future Considerations


# Time

## Purpose

Provides all time-related services.

## Responsible For

- NTP synchronization
- UTC time
- Local time conversion
- Time formatting
- Uptime

## Not Responsible For

- Scheduling
- Timers
- Application events

---

# Storage

...

---

# Network

...

---

# OTA

...

---

# GPS

...

---

# Temperature

...

---

# Platform

...

---