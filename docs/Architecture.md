# EmbeddedInfrastructure Architecture

## Purpose

EmbeddedInfrastructure is a reusable framework providing common infrastructure, hardware abstractions, and services for embedded applications. It intentionally excludes application-specific logic.

EmbeddedInfrastructure is a reusable framework providing common infrastructure, hardware abstractions, and services for embedded applications. It intentionally excludes application-specific logic.

Its purpose is to provide a stable, consistent, and well-defined foundation upon which embedded applications can be built.

The framework is designed to encourage correct architectural decisions through clearly defined ownership, encapsulation, and component responsibilities rather than through coding conventions alone.

### Framework Owns Infrastructure

EmbeddedInfrastructure provides reusable infrastructure services that are common across embedded applications.

The framework owns the implementation and lifecycle of those services, including initialization, resource management, persistence, communication, and error handling.

Applications own the behavior that is built upon those services. They define application-specific data, policies, decision making, and business logic.

This separation allows the framework to remain generic while allowing each application to express its own unique behavior.

## Guiding Principles

The following principles define the architectural philosophy of EmbeddedInfrastructure. They are intended to guide the design of every component within the framework. When making architectural decisions, these principles take precedence over implementation convenience.

### Single Authoritative Owner

Every capability within EmbeddedInfrastructure has exactly one authoritative owner.

Each component is responsible for managing its own state, behavior, and public interface. No other component should duplicate, modify, or assume responsibility for those same capabilities.

This principle provides clear ownership, eliminates conflicting implementations, and reduces unintended interactions between components.

Examples:

- Logging owns log creation, formatting, buffering, and transport.
- MQTT owns MQTT connectivity and message transport.
- Time owns system time synchronization and time services.
- Storage owns access to the file system.

A component requiring one of these capabilities should request the service from its owner rather than implementing its own version.

---

### Mechanism vs. Policy

EmbeddedInfrastructure provides mechanisms.

Applications provide policy.

Infrastructure components implement reusable capabilities without making application-specific decisions. They provide the tools necessary to perform work but never decide when, why, or under what conditions those tools should be used.

Applications are responsible for determining operational behavior by applying their own policies using the services provided by the framework.

Examples:

Infrastructure:

```cpp
mqtt.publish(topic, payload);
```

Application:

```cpp
if (freshTankLevel >= 95)
    mqtt.publish(...);
```

The framework provides the ability to publish an MQTT message.

The application decides when publishing is appropriate.

Separating mechanism from policy allows infrastructure components to remain reusable across many different embedded applications.

---

### Component Design

Each component shall have one clearly defined primary responsibility.

As the framework evolves, new functionality should be placed into the component whose responsibility most naturally includes it. If new functionality does not clearly belong to an existing component, a new component should be created rather than expanding an unrelated one.

Large "utility" or "common" modules should be viewed as indicators that architectural refactoring is needed.

Components should remain cohesive, focused, and easy to understand.

Whenever possible, components should expose a small, well-defined public interface while hiding their implementation details.

A component should be understandable by reading only its public interface and its documented responsibilities.

---

### Loose Coupling

Components communicate through published interfaces rather than through implementation details.

Applications shall depend only on EmbeddedInfrastructure and shall not directly depend on third-party libraries.

For example, an application should call

```cpp
mqtt.publish(...);
```

rather than

```cpp
client.publish(...);
```

where `client` is an instance of a third-party MQTT library.

Likewise, applications should interact with infrastructure services rather than directly with Arduino libraries such as WiFi, ArduinoJson, LittleFS, SD, TinyGPS++, or PubSubClient.

This abstraction provides several important benefits:

- Third-party libraries may be replaced without requiring changes to application code.
- Infrastructure components present a consistent interface across all applications.
- Vendor-specific implementation details remain isolated within the framework.
- Applications become easier to maintain, test, and understand.

A component should know only what another component's public interface promises. It should never rely on internal implementation details or private state.

## Architectural Layers

Application
        │
        ▼
EmbeddedInfrastructure
        │
        ├── Logging
        ├── MQTT
        ├── Time
        ├── Storage
        ├── Network
        ├── GPS
        ├── OTA
        ├── Temperature
        ├── WebServer
        ├── Platform
        └── ...
        │
        ▼
Arduino Libraries
        │
        ▼
ESP32 Hardware

## Component Responsibilities

Each component shall document the following:

- Purpose
- Responsible For
- Not Responsible For
- Public Interface (optional)
- Dependencies (optional)
- 
### For example:

#### Logging

- Responsible For
##### Log generation
##### Log formatting
##### Log transport
##### Log buffering

#### Not Responsible For

##### Dashboard display
##### Event Explorer
##### Log analysis

## Future Direction

EmbeddedInfrastructure is intended to evolve through the addition of new services and components while preserving the architectural principles defined in this document.

New functionality should strengthen the framework rather than increase coupling or duplicate responsibilities.

As the framework grows, simplicity, clarity, and well-defined ownership shall take precedence over convenience or expedient implementation.

The goal of EmbeddedInfrastructure is not to become a collection of reusable code, but to become a coherent foundation upon which reliable embedded applications can be built and maintained for years to come.

