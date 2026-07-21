#pragma once
//
//  ei_storage.h
//  
//
//  Created by Stephen McKeon on 7/17/26.
//

// -----------------------------------------------------------------------------
// EmbeddedInfrastructure
//
// Module:
//     Storage
//
// Owns:
//
//     - Filesystem abstraction
//     - File operations
//     - Directory operations
//     - Filesystem initialization
//
// Provides:
//
//     - Read files
//     - Write files
//     - Append files
//     - Delete files
//     - Rename files
//     - Create directories
//     - Enumerate directories
//     - Query filesystem information
//
// Does Not Own:
//
//     - Startup policy
//     - Configuration data
//     - Which files should exist
//     - Default file contents
//
// Goal:
//
//     Provide a single, consistent interface for persistent storage while
//     hiding the underlying filesystem implementation (LittleFS, SD, or
//     future storage technologies).
//
// -----------------------------------------------------------------------------

#if __has_include("myConfig.h")
  #include "myConfig.h"
#else
  // 🎯 CRITICAL DEFENSIVE SANITY TRAP
  // If the project folder is missing myConfig.h, stop compilation immediately and tell the user why!
  #error "CRITICAL CONFIG ERROR: 'myConfig.h' is missing from your project folder. Please copy a template copy into your local sketch directory."
#endif

#include <ei_types.h>

#include <ei_time.h>
#include <ei_logging.h>

// ============================================================================
// CONFIGURATION PROFILES (Keeps background code completely unbloated)
// ============================================================================
#ifdef SYSTEM_USES_LITTLEFS
  #warning "Including LittleFS into mySD framework"
  #include <LittleFS.h>
  #endif

#ifdef SYSTEM_USES_SD
  #warning "Including SD Card into mySD framework"
  #include <SD.h>
#endif

#include <FS.h>
#include <ArduinoTrace.h>

#define FILE_ERROR          -1
#define FILE_ALREADY_EXISTS  0
#define FILE_WAS_CREATED     1

// -----------------------------------------------------------------------------
// StorageConfig
//
// Configuration options for the Storage component.
//
// The application may modify these values before calling Storage::begin().
// -----------------------------------------------------------------------------

struct StorageConfig {
    uint32_t lowSpaceLimitBytes = 10 * 1024;    // Warn if free storage falls below this many bytes during initialization.
    bool logStatisticsOnBoot = true;            // Log filesystem statistics during initialization.
};

// -----------------------------------------------------------------------------
// StorageStats
//
// Current status of the mounted filesystem.
// -----------------------------------------------------------------------------

struct StorageStats {
    bool mounted = false;
    uint32_t totalBytes = 0;
    uint32_t usedBytes = 0;
    uint32_t freeBytes = 0;
};

struct StorageInfo {
    bool mounted;
    uint32_t totalBytes;
    uint32_t usedBytes;
    uint32_t freeBytes;
};

// -----------------------------------------------------------------------------
// Storage Component
//
// Encapsulates access to the active filesystem and provides the public storage
// interface used by the rest of EmbeddedInfrastructure.
//
// -----------------------------------------------------------------------------

class Storage {
private:
  fs::FS* _fs;
  StorageStats _stats;
#ifdef SYSTEM_USES_LITTLEFS
  bool startLittleFS();
  void refreshLittleFSStats();
#endif
#ifdef SYSTEM_USES_SD
  bool startSD(int cspin);
  void refreshSDStats();
#endif
  void refreshStats();
  void buildDirReport(String &report, const char *dirname, uint8_t levels);

public:
  // Lifecycle plumbing
  Storage();

  /* LEGACY TRAP BLOCK FOR DEPRECATED fileExists() */
  [[deprecated("WARNING: fileExists() has been renamed to ensureFileExists(). Please update your code call.")]]
  inline bool fileExists(String fname, String data, int from) {
    return ensureFileExists(fname, data, from);
  }
  bool startFileSystem();
  void dirReport(const char *dirname, uint8_t levels);
  String listDir(const char * dirname, uint8_t levels, char separator);
  bool createDir(const char * path);
  void removeDir(const char * path);
  String readFile(const char * path, bool logFile);
  size_t load_data(File f, uint8_t *buffer, size_t maxLen, size_t index);
  bool createFile(const char * path, const char * message);
  int writeFile(const char * path, const char * message, int from);
  int appendFile(const char * path, const char * message);
  void renameFile(const char * path1, const char * path2);
  void deleteFile(const char * path, int from);
  void testFileIO(const char * path);
  int getFileSize(const char * path);
  bool createDirIfNotExist(String dirName);
  int ensureFileExistsLazy(String fname, String (*dataGenerator)(), int from);
  bool ensureFileExists(String fname, String data, int from);
  const char* fileSystemName() const;
  fs::FS& getFS();
  void getInfo(StorageInfo& info) const;
  void logInfo(const String& msg);
  void logError(const String& msg);
  int writeJsonFile(const char* path, const JsonDocument& doc, int from);
  bool readJsonFile(const char* path, JsonDocument& doc, int from)
};

// ============================================================================
// GLOBAL CONVENIENCE INSTANCE DEFINITION
// ============================================================================
extern Storage storage;
