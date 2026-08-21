
# EmbeddedInfrastructure Architecture

## Purpose

The primary purpose of EmbeddedInfrastructure is to collect the functionality that is common across ESP32 applications into a reusable library. Those capabilities are implemented once, using consistent architectural principles and well-defined component responsibilities, allowing applications to focus on the functionality that makes them unique.

By moving common functionality into a shared library:

- Common capabilities are implemented once rather than repeatedly.
- Applications become smaller, simpler, and easier to understand.
- All applications implement shared services in a consistent manner.
- Improvements and bug fixes benefit every application that uses the library.
- Debugging and maintenance are simplified because common functionality behaves consistently throughout the environment.

EmbeddedInfrastructure is intended to provide the common infrastructure required by many ESP32 applications. It intentionally excludes application-specific behavior, allowing each application to focus on the logic that makes it unique.

The application remains responsible for its own policies, decision making, device-specific behavior, and business logic. EmbeddedInfrastructure provides the common services upon which those applications are built.

### Infrastructure Ownership

EmbeddedInfrastructure provides reusable infrastructure services that are common across embedded applications.

The infrastructure owns the implementation and lifecycle of those services, including initialization, resource management, persistence, communication, and error handling.

Applications own the behavior that is built upon those services. They define application-specific data, policies, decision making, and business logic.

This separation allows the infrastructure to remain generic while allowing each application to express its own unique behavior.

## Guiding Principles

The following principles define the architectural philosophy of EmbeddedInfrastructure. They are intended to guide the design of every component within the infrastructure. When making architectural decisions, these principles take precedence over implementation convenience.

The library contains common infrastructure. Application-specific behavior remains in the application.

Subsystems should perform work in bounded time slices whenever practical. Long-running operations should be decomposed into incremental steps executed over successive calls to loop().

Subsystem state is private by default. Public interfaces are added only when another subsystem has a legitimate need that cannot be satisfied through an existing higher-level operation.

Prefer exposing capabilities over exposing implementation details.



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

#### Web Message Envelope

Web messages entering the system shall use a common JSON message
envelope.

The envelope provides the information required to identify the
message owner, route the message within that owner, identify the
requested operation, and provide command-specific data.

```json
{
  "owner": "mqtt",
  "route": "server/cfg",
  "command": "SET",
  "data": {
    "address": "192.168.1.22"
  }
}

---

### Mechanism vs. Policy

EmbeddedInfrastructure provides mechanisms.

Applications provide policy.

Infrastructure components implement reusable capabilities without making application-specific decisions. They provide the tools necessary to perform work but never decide when, why, or under what conditions those tools should be used.

Applications are responsible for determining operational behavior by applying their own policies using the services provided by the infrastructure.

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

The infrastructure provides the ability to publish an MQTT message.

The application decides when publishing is appropriate.

Separating mechanism from policy allows infrastructure components to remain reusable across many different embedded applications.

---

### Component Design

- Each component shall have one clearly defined primary responsibility.

- As the infrastructure evolves, new functionality should be placed into the component whose responsibility most naturally includes it. If new functionality does not clearly belong to an existing component, a new component should be created rather than expanding an unrelated one.

- Large "utility" or "common" modules should be viewed as indicators that architectural refactoring is needed.

- Components should remain cohesive, focused, and easy to understand.

- Whenever possible, components should expose a small, well-defined public interface while hiding their implementation details.

A component should be understandable by reading only its public interface and its documented responsibilities.

#### The Web subsystem supports a single active file upload at a time.
- File uploads are intended for maintenance and configuration activities, not high-throughput file serving. If an upload is already in progress, additional upload requests are rejected until the current upload completes.

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
- Vendor-specific implementation details remain isolated within the infrastructure.
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

## Communication Architecture

The EmbeddedInfrastructure communication architecture defines how information moves between external interfaces, library subsystems, and the application. Its primary goals are to preserve subsystem ownership, minimize coupling, and provide a consistent communication model regardless of whether a request originates from MQTT, the web interface, or another transport.

### Message Ownership

- Every incoming message has a single authoritative owner. Ownership is determined before the message is interpreted.

- Library messages are owned by EmbeddedInfrastructure and are coordinated by the EiSystem subsystem. Application messages are owned entirely by the application.

- Within the library, each subsystem owns the interpretation of messages addressed to it. No subsystem modifies or interprets another subsystem's configuration or runtime data.

- This ownership model preserves subsystem independence while allowing new services to be added without affecting existing components.

### Message Flow

Communication is performed in layers. Transport-specific subsystems receive incoming messages, perform any protocol-specific processing, and then forward the message to its owner.

#### Library messages
- EiSystem coordinates the processing of the request and forwards it to the subsystem responsible for the requested service.

#### Application messages

- The library forwards the request directly to the application, allowing the application complete control over its own messaging protocol.

The details of individual message formats and protocol requirements are defined separately from this architectural document.

### System Coordination

EiSystem coordinates operations that involve multiple library subsystems.

Its responsibilities include coordinating subsystem startup, routing library-level requests to the appropriate subsystem, and managing operations that span subsystem boundaries.

EiSystem does not implement protocol-specific behavior, own subsystem configuration, or perform subsystem-specific processing. Those responsibilities remain within the subsystem that owns the associated data and behavior.

By separating coordination from implementation, EmbeddedInfrastructure maintains loose coupling while allowing each subsystem to evolve independently.


## Each component shall document the following:

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

New functionality should strengthen the infrastructure rather than increase coupling or duplicate responsibilities.

As the infrastructure grows, simplicity, clarity, and well-defined ownership shall take precedence over convenience or expedient implementation.

The goal of EmbeddedInfrastructure is not simply to collect reusable code, but to provide a coherent, consistent foundation upon which reliable embedded applications can be built and maintained for years to come.

