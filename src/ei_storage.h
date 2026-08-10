#pragma once

#if __has_include("config.h")
  #include "config.h"
#else
  // 🎯 CRITICAL DEFENSIVE SANITY TRAP
  // If the project folder is missing config.h, stop compilation immediately and tell the user why!
  #error "CRITICAL CONFIG ERROR: 'config.h' is missing from your project folder. Please copy a template copy into your local sketch directory."
#endif

#include <ArduinoJson.h>
#include <ei_types.h>

#include <ei_logging.h>

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

struct StorageState {
    bool mounted;
};

struct StorageConfig {
    uint32_t lowSpaceLimitBytes = 10 * 1024;    // Warn if free storage falls below this many bytes during initialization.
    bool logStatisticsOnBoot = true;            // Log filesystem statistics during initialization.
};

struct StorageStats {
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

class Storage {
public:
  Storage();      // WHAT DOES THIS DO?
  
  enum class WriteResult {
      Success,
      OpenFailed,
      WriteFailed
  };

  enum class EnsureFileResult {
      Created,
      AlreadyExists,
      Error
  };


  /* LEGACY TRAP BLOCK FOR DEPRECATED fileExists() */
  [[deprecated("WARNING: fileExists() has been renamed to ensureFileExists(). Please update your code call.")]]
  inline bool fileExists(String fname, String data, int from) {
  }
  
  bool evtLoop();
  bool startup();
  void dirReport(const char *dirname, uint8_t levels);
  String listDir(const char * dirname, uint8_t levels, char separator);
  bool createDir(const char * path);
  void removeDir(const char * path);
  String readFile(const char * path, bool logFile);
  size_t load_data(File f, uint8_t *buffer, size_t maxLen, size_t index);
  bool createFile(const char * path, const char * message);
  WriteResult writeFile(const char * path, const char * message, int from);
  int appendFile(const char * path, const char * message);
  void renameFile(const char * path1, const char * path2);
  void deleteFile(const char * path, int from);
  void testFileIO(const char * path);
  int getFileSize(const char * path);
  bool createDirIfNotExist(String dirName);
  EnsureFileResult ensureFileExists(const String& fileName, const JsonDocument& doc, int from);
  bool exists(const char* path) const;
  bool exists(const String& path) const;
  const char* fileSystemName() const;
  fs::FS& getFS();
  void getInfo(StorageInfo& info) const;
  WriteResult writeJsonFile(const char* path, const JsonDocument& doc, int from);
  bool readJsonFile(const char* path, JsonDocument& doc, int from);
  void processMsg(const JsonDocument& doc);

private:
  StorageConfig _config;
  StorageState  _state;
  StorageStats  _stats;
  int  _writeIndex = 0;
  bool _wrapped    = false;

  fs::FS* _fs;                                      // File system
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
};

// ============================================================================
// GLOBAL CONVENIENCE INSTANCE DEFINITION
// ============================================================================
extern Storage storage;
