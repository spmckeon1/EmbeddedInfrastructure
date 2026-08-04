# EmbeddedInfrastructure Coding Standard

Classes shall be organized in this order:

## Class Layout

## For classes with protected members:

class ClassName {
private:
    // Private data members

    // Private helper functions

protected:
    // Protected members (used rarely)

public:
    // Constructors/destructor

    // Public interface
};

### For classes without protected members:

class Logging {
private:
private:
    // Private data members

    // Private helper functions


public:
    // Constructors/destructor

    // Public interface
};

## MQTT Message Contract

### Purpose

The MQTT Message Contract defines the standard message format used by
EmbeddedInfrastructure.

All MQTT messages exchanged with the library shall conform to this
contract. The common message envelope provides a consistent mechanism
for routing messages, identifying ownership, and invoking services
without coupling the transport layer to individual library subsystems.

Applications may define their own services and commands while
continuing to use the common message envelope.

### Standard Message Envelope
{
    "source": "...",
    "scope": "...",
    "service": "...",
    "command": "...",
    "data": {...}
}

### Required Fields

Every message shall contain the following fields:

- scouce
- owner
- service
- command
- data

#### Source

Identifies the logical originator of the request. This field provides operational context for logging, diagnostics, auditing, and application policy. It is not used by EmbeddedInfrastructure for routing decisions.

#### owner

Identifies the owner of the message.

##### Valid Values

- library
- application

#### service

- Identifies the subsystem responsible for processing the message.

##### Library Services

- network
- mqtt
- time
- logging
- ota
- system

Applications may define their own service names.

#### command

- Identifies the operation requested of the service.

- Each service defines the commands it supports.

#### Data

- Contains the data associated with the command.

- The Data format is defined by the owning service.