#ifndef _MYSD_H_
#define _MYSD_H_

#if __has_include("myConfig.h")
  #include "myConfig.h"
#else
  // 🎯 CRITICAL DEFENSIVE SANITY TRAP
  // If the project folder is missing myConfig.h, stop compilation immediately and tell the user why!
  #error "CRITICAL CONFIG ERROR: 'myConfig.h' is missing from your project folder. Please copy a template copy into your local sketch directory."
#endif


// ============================================================================
// CONFIGURATION PROFILES (Keeps background code completely unbloated)
// ============================================================================
#ifdef SYSTEM_USES_LITTLEFS
  #warning "Including LittleFS into mySD framework"
  #include <LittleFS.h>
  #include <FS.h>
  [[deprecated("LEGACY NOTICE: FS_IN_USE is deprecated. Migrate this file to myFileSys class calls.")]] inline fs::FS &FS_IN_USE = LittleFS;
  inline String fsInUse = "LittleFS";
  #endif

#ifdef SYSTEM_USES_SD_CARD
  #warning "Including SD Card into mySD framework"
  #include <SD.h>
  #include <FS.h>
[[deprecated("LEGACY NOTICE: FS_IN_USE is deprecated. Migrate this file to myFileSys class calls.")]] inline fs::FS &FS_IN_USE = LittleFS;
  inline String fsInUse = "SD";
#endif

#include <ArduinoTrace.h>
#include <commonItems_ESP32.h>

#define FILE_ERROR          -1
#define FILE_ALREADY_EXISTS  0
#define FILE_WAS_CREATED     1

// ============================================================================
// THE FILE SYSTEM MANAGER CLASS CAPSULE
// ============================================================================
class MySDClass {
  private:
    fs::FS* _fs; // The hidden master internal pointer

  public:
    // Lifecycle plumbing
    MySDClass();
    void begin(fs::FS &fsInstance);

    /* LEGACY TRAP BLOCK FOR DEPRECATED fileExists() */
    [[deprecated("WARNING: fileExists() has been renamed to ensureFileExists(). Please update your code call.")]]
    inline bool fileExists(String fname, String data, int from) {
      return ensureFileExists(fname, data, from);
    }

    // Universal file actions (Notice the 'fs::FS &fs' parameters are GONE!)
    void dirToDiskFile(String dirname, String filename, int depth);
    void listDirToFile(listDirStruct_ptr dl);
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
    void printDL_struct(listDirStruct_ptr dl, int from);
    fs::FS& getFS();
};

// ============================================================================
// GLOBAL CONVENIENCE INSTANCE DEFINITION
// ============================================================================
extern MySDClass myFileSys;
extern String fsInUse;
extern float freeKB;            // MUST BE DECLARED IN THE .INO FILE - how much availabe disk space is there at boot

// non class functions
extern bool startSD(int cspin); // Hardware initialization function keeps sitting outside the class capsule
extern bool startLittleFS();

#endif /* _MYSD_H_ */
