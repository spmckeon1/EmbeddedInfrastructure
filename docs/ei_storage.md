# EmbeddedInfrastructure Storage (ei_storage)

## Purpose

ei_storage provides a hardware-independent interface to persistent storage for the EmbeddedInfrastructure library. It is responsible for mounting the configured filesystem and providing a consistent API for file and directory operations.

The implementation hides the underlying filesystem (LittleFS, SD, or future filesystems) from application code.

## Design Goals
- Present a simple, object-oriented API.
- Hide filesystem implementation details.
- Minimize application knowledge of storage internals.
- Support both application services and field diagnostics.
- Keep the library portable across supported filesystems.
- Expose operations, not implementation details.

## In Scope

- Mounting the configured filesystem.
- Reading and writing files.
- Creating and deleting files and directories.
- Renaming files and directories.
- Verifying the existence of files.
- Initializing files with default contents when required.
- Enumerating directory contents.
- Providing administrative diagnostic services.

## Out of Scope

- Application configuration management.
- JSON interpretation.
- Business logic.
- User interface presentation.


## Public API

The public interface consists of two categories of services.

### Application Services

These services are intended for normal application use.

Examples include:

- readFile()
- writeFile()
- deleteFile()
- fileExists()
- ensureFileExists()

These services return data or status to the caller.

### Administrative Services

Administrative services are intended for maintenance, diagnostics, and field support.

Examples include:

- Log a directory listing.
- Log filesystem information.
- Verify filesystem integrity.
- Report storage statistics.

Administrative services report their results through the system logging service (mySP). Their intended consumer is the system operator viewing the Event Explorer or system log.

Administrative reports shall be presented as a single, coherent report that can be read top-to-bottom without requiring the administrator to reconstruct the information from multiple log entries.

## Encapsulation

Applications interact only with the public API.

Internal implementation objects (such as listDirStruct) remain private to ei_storage and are never exposed outside the class.

Public interfaces express operations, not implementation details.

## Logging

Reports operational and administrative events through the system logging service (`mySP`).

Administrative services are intended primarily for system operators and field diagnostics.

Caller information may be supplied to assist diagnostics and simplify tracing the origin of storage requests.

## Design Philosophy

The EmbeddedInfrastructure design principle is followed:

Expose operations, not implementation details.

Applications request storage services. Administrative tools request diagnostic services. Internal algorithms and working structures remain private to the implementation.

## Future Expansion

Applications interact only with the ei_storage interface.

Additional filesystem implementations may be supported in the future without requiring changes to application code.

Additional administrative services may be added as operational needs evolve while maintaining encapsulation of implementation details.