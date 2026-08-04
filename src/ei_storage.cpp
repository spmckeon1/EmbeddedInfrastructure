//
//  ei_storage.cpp
//  
//
//  Created by Stephen McKeon on 7/17/26.
//


/*
 01-26-2015 - Changed readFile() to have a boolean parameter for logging file contents.
 
 */
#if __has_include("myConfig.h")
 #include "myConfig.h"
#else
 // 🎯 CRITICAL DEFENSIVE SANITY TRAP
 // If the project folder is missing myConfig.h, stop compilation immediately and tell the user why!
 #error "CRITICAL CONFIG ERROR: 'myConfig.h' is missing from your project folder. Please copy a template copy into your local sketch directory."
#endif

#include <Arduino.h>
#include <ArduinoTrace.h>
#include <ei_conversion.h>
#include <ei_web.h>
#include <ei_storage.h>

Storage storage;        // This brings the object to life in memory!

/*---------------    AUTIMATICALLY CLEARS THE INTERNAL POINTER AT BOOT   ---------------*/

Storage::Storage()
    : _fs(nullptr)
{
}

/*---------------    STRAGE EVENT LOOP   ---------------*/

bool Storage::evtLoop() {
  return false;
}

/*---------------    PUBLIC: START THE CHOOSEN FILE SYSTEM   ---------------*/

bool Storage::startup()
{
#ifdef SYSTEM_USES_LITTLEFS
  bool ok = startLittleFS();
#elif defined(SYSTEM_USES_SD)
  bool ok = startSD(SD_CS_PIN);
#else
  #error "No storage medium selected in myConfig.h"
#endif
  if (!ok) return false;
  if (_fs == nullptr) {
    logError(LS, ET::STORAGE, "Storage startup succeeded but _fs was not initialized.");
    return false;
  }
  return true;
}

/*---------------    START SD FILE SYSTEM   ---------------*/

#ifdef SYSTEM_USES_SD
  bool Storage::startSD(int cspin) {
    logInfo(FN, LN, "Setting up the SD file system.");

    // 1. Mount the physical SD Card hardware partition
    if(!SD.begin(cspin)){                                                                     // start the SD system
      logError(LS, ET::STORAGE, "Card Mount Failed.");
      return false;                                                                           // and exit the function
    }
    _fs = &SD;
    refreshStats();
    String s = "";
    switch(SD.cardType()) {    // Query hardware details safely (Now fully protected by the active capsule)
      case CARD_NONE:
        s = "No SD card attached.";
        break;
      case CARD_MMC:
        s = "SD Card Type: MMC.";
        break;
      case CARD_SD:
        s = "SD Card Type: SDSC.";
        break;
      case CARD_SDHC:
        s = "SD Card Type: SDHC.";
        break;
      default: s = "SD Card Type: UNKNOWN.";
    }
    _state.mounted = true;
    logInfo(FN, LN, s);
    StorageInfo info;
    getInfo(info);
    int cardSize = (int)(SD.cardSize() / (1024 * 1024));                                      // get the size of the SD card
    logInfo(LS, ET::STORAGE, "SD Card Size: " + String(cardSize) + " MB.");
    logInfo(LS, ET::STORAGE, "Total space: " + String(info.totalBytes / (1024 * 1024)) + " MB.");
    logInfo(LS, ET::STORAGE, "Used space:  " + String(info.usedBytes  / (1024 * 1024)) + " MB.");
    logInfo(LS, ET::STORAGE, String(info.freeBytes  / (1024 * 1024)) + " MB.");

    return true;
  }
#endif

/*---------------    START LTTLEFS FILE SYSTEM   ---------------*/

#ifdef SYSTEM_USES_LITTLEFS
  bool Storage::startLittleFS() {
    logInfo(LS, ET::STORAGE, "Setting up the LittleFS file system.");
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {                // 1. Mount the physical hardware partition map
      logInfo(LS, ET::STORAGE, "LittleFS Mount Failed\n");                   // log this
      return false;                                                 // and exit the function
    }
    _state.mounted = true;
    _fs = &LittleFS;
    refreshStats();
    size_t totalBytesRaw = LittleFS.totalBytes();                           // 2. Perform flash partition space diagnostics safely
    size_t usedBytesRaw  = LittleFS.usedBytes();
    float totalKB = totalBytesRaw / 1024.0;
    float usedKB  = usedBytesRaw / 1024.0;
    float freeKB  = totalKB - usedKB;
    logInfo(LS, ET::STORAGE, "LittleFS Total Space: " + String(totalKB, 1) + " KB");   // You can now safely print or save these variables anywhere!
    logInfo(LS, ET::STORAGE, "LittleFS Used Space:  " + String(usedKB, 1) + " KB");
    logInfo(LS, ET::STORAGE, "LittleFS Free Space:  " + String(freeKB, 1) + " KB");
    if (freeKB < 10.0) {                                                      // Low space safety check
      logInfo(LS, ET::STORAGE, "WARNING: Low disk space! Less than 10 KB remaining.");
    }
    return true;
  }
#endif

/*---------------  PUBLIC INTERACE TO REQUEST A DIRECTORY LISTIMG APPEAR IN THE SYSLOG  ---------------*/

void Storage::dirReport(const char *dirname, uint8_t levels) {
    String report;
    report.reserve(2048);                     // Reduce heap reallocations (optional)
    report += "Directory Report\n";
    report += "================\n";
    buildDirReport(report, dirname, levels);
    report += "\nEnd of Report\n";
  logInfo(LS, ET::STORAGE, report);
}

/*---------------  RECURSIVILY BUILD A FORMATTED DIRECTORY REPORT  ---------------*/

void Storage::buildDirReport(String &report, const char *dirname, uint8_t levels) {
  unsigned long t = millis();
  const unsigned long yieldTime = 1000;
  File root = _fs->open(dirname);
  if (!root) {
    report += "\nUnable to open directory: ";
    report += dirname;
    report += "\n";
    return;
  }
  if (!root.isDirectory()) {
    report += "\nNot a directory: ";
    report += dirname;
    report += "\n";
    root.close();
    return;
  }
  report += "\nDirectory: ";
  report += dirname;
  report += "\n\n";

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      report += "  DIR  : ";
      report += file.name();
      report += "\n";
      if (levels > 0) {
          buildDirReport(report, file.name(), levels - 1);
      }
    } else {
      report += "  FILE : ";
      report += file.name();
      report += "    SIZE: ";
      report += file.size();
      report += " bytes\n";
    }
    file = root.openNextFile();
  }
  root.close();
}

/*---------------  LIST DIRECTORY  ---------------*/

String Storage::listDir(const char * dirname, uint8_t levels, char separator) {
  String s = "";
  unsigned long t = millis();
  const unsigned long yieldTime = 1000;
  File root = _fs->open(dirname);
  if (!root) {
      return "Failed to open directory";
  }
  if (!root.isDirectory()) {
      root.close();
      return "Not a directory";
  }
  File file = root.openNextFile();
  while (file) {
    if (millis() - t > yieldTime) {
      yield();
      t = millis();
    }
    if (file.isDirectory()) {
      s += "  DIR : " + String(file.name()) + separator;
      if (levels > 0) {
        s += listDir(file.name(), levels - 1, separator);
      }
    } else {
      s += "  FILE: " + String(file.name()) +
           "  SIZE: " + String(file.size()) + separator;
    }
    file = root.openNextFile();
  }
  root.close();
  return s;
}

/*---------------  CREATE DIRECTORY  ---------------*/

bool Storage::createDir(const char * path) {
  if (_fs->mkdir(path)) {
    refreshStats();
    return true;
  } else {
    return false;
  }
}

/*---------------  DELETE DIRECTORY  ---------------*/

void Storage::removeDir(const char * path) {
  logInfo(LS, ET::STORAGE, "Removing Dir: " + String(path));
  if (_fs->mkdir(path)) {
    refreshStats();
    logInfo(LS, ET::STORAGE, "Dir removed");
  } else {
    logInfo(LS, ET::STORAGE, "rmdir failed");
  }
}

/*---------------  MY READ FILE  ID  ---------------*/

String Storage::readFile(const char * path, bool logFile) {
 if (!logFile) logInfo(LS, ET::STORAGE, "Reading file: " + String(path));                         // if debugging log the file path
 String s = "";                                                                              // create an empty string
 
 // ---> CHANGE: Use the internal class pointer arrow instead of 'fs.'
 File file = _fs->open(path, FILE_READ);                                                       // open the file
 
 if (!file) {                                                                                // if the file was not opened
   logError(LS, ET::STORAGE, "ERROR 100: Failed to open file '" + String(path) + "' for reading");      // log the error
   return "ERROR 100: Failed to open file '" + String(path) + "' for reading";                // and return the error to the calling function
 }
 
 int fSize = file.available();
 if (false) {                                                                                // if debugging
   logInfo(LS, ET::STORAGE, String(path) + ": Available bytes to be read are: " + String(fSize)); // log the bytes available to be read
 }
 
 char c;                                                                                     // declar a char 'c'
 while (file.available()) {                                                                  // while file.available > 0
   c = file.read();                                                                          // read the next char from the file
   s += String(c);                                                                           // and append it to 's'
 }
 file.close();                                                                               // close the file
 
 if (logFile) {
   logInfo(LS, ET::STORAGE, "Reading file: " + String(path) + + ", contents: " + s);              // log the file path and file contents
 }
 return s;                                                                                   // return 's' to the calling function
}

/*---------------  READ FILE SPECIFICALLY FOR USE WITH ESPAsyncWebServer  ---------------*/

size_t Storage::load_data(File f, uint8_t *buffer, size_t maxLen, size_t index) {
  if (f.available()) {
    // FIXED: Log output moved ABOVE the return statement so it actually executes!
    logInfo(LS, ET::STORAGE, "Reading file:'" + String(f.name()) + "' maxLen ='" + String(maxLen) + "'");  // log that the file was read from
    return f.read(buffer, maxLen);                                                                // read maxLen bytes from the file 'f'
  } else {                                                                                        // else, there must be no bytes left in the file to read
    web.setDownloadingFile(false);                                                                    // tell the server the download is complete
    logInfo(LS, ET::STORAGE, "Web page file download '" + String(f.name()) + "' has completed.");          // log the read is complete
    f.close();                                                                                    // close the file
    return 0;                                                                                     // and return the length of the read...zero bytes
  }
  return 0;                                                                                       // and return the length of the read...zero bytes
}

/*---------------  WRITE FILE  IDS 122---------------*/

Storage::WriteResult Storage::writeFile(const char * path, const char * message, int from) {
  File file = _fs->open(path, FILE_WRITE);
  if (!file) {
    logError(LS, ET::STORAGE, "Failed to open file '" + String(path) +
             "' for writing." + conv.fromStr(from));
    return WriteResult::OpenFailed;
  }
  int n = file.print(message);
  size_t expected = strlen(message);
  if (n != expected) {
    logError(LS, ET::STORAGE, "Write failed to file '" + String(path) +
             "'. Wrote " + String(n) + " of " + String(expected) +
             " bytes." + conv.fromStr(from));
    file.close();
    refreshStats();
    return WriteResult::WriteFailed;
  }
  logInfo(LS, ET::STORAGE, String(n) + " bytes written to '" + String(path) +
          "'" + conv.fromStr(from));
  file.close();
  refreshStats();
  return WriteResult::Success;
}
/*---------------  CREATE FILE  ---------------*/

bool Storage::createFile(const char * path, const char * message) {
  File file = _fs->open(path, FILE_WRITE);                                        // open the file in a writing mode
  if (!file) {                                                                    // if the file failed to be opened
   return false;
  }
  int n = file.print(message);                                                    // write the file capturing the number of bytes written
  if (n) {                                                                        // if some bytes were wrtten
    logInfo(LS, ET::STORAGE, String(n) + " bytes written to '" + String(path) + "'");      // log the number of bytes written to file 'path'
  } else {                                                                        // else no bytes were written so
    logInfo(LS, ET::STORAGE, "Write failed to file '" + String(path) + "'");               // log the file failed to be written
    file.close();                                                                 // close the file
    return false;
  }
  file.close();                                                                   // close the file
  refreshStats();
  return true;
}

/*---------------  APPEND TO FILE  ---------------
path            FQN of the file
message         Data to append to the file
return          the number of bytes written to the file
*/

int Storage::appendFile(const char * path, const char * message) {
  File file = _fs->open(path, FILE_APPEND);                                                               // open the 'path' file
  if (!file) {                                                                                            // if it failed to be opened
    logInfo(LS, ET::STORAGE, "Failed to open file '" + String(path) + "' for appending.\n");                       // advise via the serial monitor
    return 0;                                                                                             // and return to the calling function
  }
  int n = file.print(message);                                                                            // append 'message' to the file
  if (n != strlen(message)) {                                                                             // if written bytes are zero
    logInfo(LS, ET::STORAGE, "Append failed to file'" + String(strlen(message)) + "' bytes to '" + String(path));  // log this
  }
  file.close();                                                                                           // close the file
  file.close();                                                                                           // close the file
  refreshStats();
  return n;
}

/*---------------  RENAME FILE  ---------------*/

void Storage::renameFile(const char * path1, const char * path2) {
  if (_fs->rename(path1, path2)) {
    logInfo(LS, ET::STORAGE, "File renamed");
    refreshStats();
  } else {
    logInfo(LS, ET::STORAGE, "Rename failed");
  }
}

/*---------------  DELETE FILE  ---------------*/

void Storage::deleteFile(const char * path, int from) {
 
 if(_fs->exists(path) == 0) {
   logError(LS, ET::STORAGE, "File delete failed.  The file '" + String(path) +
                    "' does not exist." + conv.fromStr(from));
   return;
 }
 if (_fs->remove(path)) {
   logInfo(LS, ET::STORAGE, "File deleted '" + String(path) + "'");
   refreshStats();
   return;
 }
  logError(LS, ET::STORAGE, "Delete failed '" + String(path) + conv.fromStr(from));
}

/*---------------  TEST FILE  ---------------*/

void Storage::testFileIO(const char * path) {
  File file = _fs->open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }
  file = _fs->open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
  refreshStats();
}

/*---------------  GET FILE SIZE  ---------------*/

int Storage::getFileSize(const char * path) {
 int fSize = 0;
 File file = _fs->open(path);                                             // open the file 'path'
 if (!file) {                                                             // if it failed to be opened
   logError(LS, ET::STORAGE, "Failed to open file '" + String(path) + "'.");      // log the failure
   return -1;                                                             // and return a -1 the calling function
 }
 fSize = file.size();                                                     // get the size of the file
 file.close();                                                            // close the file
 return fSize;                                                            // return the file size to the calling function
}

/*---------------    CREATE DIRECTOR IF IT DOES NOT EXIST---------------*/

bool Storage::createDirIfNotExist(String dirName) {
  if (!_fs->exists(dirName.c_str())) {                                                // if the 'dirname' does not exist
    logInfo(LS, ET::STORAGE, "The directory '" + dirName + "' does not exist, creating it.");  // log the action
    bool result = createDir(dirName.c_str());                                         // and create the directory and return the success of this
    if(!result) {                                                                     // if the directory failed to be created then
      logError(LS, ET::STORAGE, "ERROR: Failed to create the directory '" + dirName + "'");    // log the error
    }
    return result;                                                                    // return the result
  } else {                                                                            // else
    logInfo(LS, ET::STORAGE, "The directory '" + dirName + "' exists.");                      // log the status
    refreshStats();
    return true;                                                                      // return true
  }
}

/*---------------    IF FILE DOES/DOES NOT EXIST ---------------*/

Storage::EnsureFileResult Storage::ensureFileExists(const String& fileName, const JsonDocument& doc, int from){
  if (_fs->exists(fileName)) {
    logInfo(LS, ET::STORAGE, "The file '" + fileName + "' exists." + conv.fromStr(from));
    return EnsureFileResult::AlreadyExists;
  }
  WriteResult result = writeJsonFile(fileName.c_str(), doc, from);
  if (result != WriteResult::Success) {
    logError(LS, ET::STORAGE, "Failed to create file '" + fileName + "'" + conv.fromStr(from));
    return EnsureFileResult::Error;
  }
  logInfo(LS, ET::STORAGE, "Successfully created missing file '" + fileName + "'" + conv.fromStr(from));
  return EnsureFileResult::Created;
}

/*---------------  PUBLIC - DOES THIS FILE EXIST IN THE FILE SYSTEM (String)  ---------------*/

bool Storage::exists(const String& path) const {
  return _fs->exists(path);
}

/*---------------  PUBLIC - DOES THIS FILE EXIST IN THE FILE SYSTEM (char)  ---------------*/

bool Storage::exists(const char* path) const {
  return _fs->exists(path);
}

/*---------------  GET RAW FILESYSTEM REFERENCE  ---------------*/

fs::FS& Storage::getFS() {
 return *_fs;     // Follow the pointer arrow, dereference it with (*), and hand back the raw object
}

/*---------------  CALL THE PROPER REFRESH STATS FUNCTION  ---------------*/

void Storage::refreshStats() {
  #ifdef SYSTEM_USES_LITTLEFS
      refreshLittleFSStats();
  #endif
  #ifdef SYSTEM_USES_SD
      refreshSDStats();
  #endif
}

/*---------------  REFRESH THE LITTLEFS STORAGE STATS  ---------------*/

#ifdef  SYSTEM_USES_LITTLEFS
  void Storage::refreshLittleFSStats() {
      _stats.totalBytes = LittleFS.totalBytes();
      _stats.usedBytes  = LittleFS.usedBytes();
      _stats.freeBytes  = _stats.totalBytes - _stats.usedBytes;
  }
#endif

/*---------------  REFRESH THE SD STORAGE STATS  ---------------*/

#ifdef SYSTEM_USES_SD
  void Storage::refreshSDStats() {
      _stats.totalBytes = SD.totalBytes();
      _stats.usedBytes  = SD.usedBytes();
      _stats.freeBytes  = _stats.totalBytes - _stats.usedBytes;
  }
#endif

/*---------------  PUBLIC CALL TO GET DISK STATS  ---------------*/

void Storage::getInfo(StorageInfo& info) const {
    info.totalBytes = _stats.totalBytes;
    info.usedBytes  = _stats.usedBytes;
    info.freeBytes  = _stats.freeBytes;
}

/*---------------  RETURN THE NAME OF THE FILE SYSTEM IN USE  ---------------*/

const char* Storage::fileSystemName() const {
#ifdef SYSTEM_USES_LITTLEFS
    return "LittleFS";
#elif defined(SYSTEM_USES_SD)
    return "SD";
#else
    return "Unknown";
#endif
}

/*-----  WRITE A JSON FILE TO DISK  -----*/

Storage::WriteResult Storage::writeJsonFile(const char* path, const JsonDocument& doc, int from) {
    String json;
    serializeJson(doc, json);
    return writeFile(path, json.c_str(), from);
}

/*-----  READ A JSON FILE FROM DISK  -----*/

bool Storage::readJsonFile(const char* path, JsonDocument& doc, int from) {
    String json = readFile(path, from);
    auto err = deserializeJson(doc, json);
  if (err) {
    logError(LS, ET::STORAGE, "Unable to parse JSON file: " + String(path) + "Error = " + String(err.c_str()));
    return false;
  }
  return true;
}

/*-----  PROCESS AN INCOMING MSG  -----*/

void Storage::processMsg(const JsonDocument& doc) {
  
}
