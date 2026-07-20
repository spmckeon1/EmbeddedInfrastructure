//
//  ei_startup.h
//  
//
//  Created by Stephen McKeon on 7/17/26.
//

#pragma once

// -----------------------------------------------------------------------------
// EmbeddedInfrastructure
//
// Common application startup services.
//
// This component provides reusable startup routines shared by EmbeddedInfrastructure
// applications. It coordinates initialization tasks that occur during application
// startup but does not own the underlying infrastructure components.
//
// Responsibilities:
//   - Prepare application storage
//   - Initialize persistent application data
//   - Perform common startup tasks
//   - Coordinate startup initialization
//
// Not Responsible For:
//   - Logging implementation
//   - Storage implementation
//   - Network implementation
//   - Application-specific startup logic
// -----------------------------------------------------------------------------


// FOREWARD DECLARATIONS
extern void startupFileChk();

