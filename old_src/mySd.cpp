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

#include <ArduinoTrace.h>
#include <commonItems_ESP32.h>
#include <mySd.h>           // This pulls in your MySDClass blueprint

// ============================================================================
// 🎯 THE CRITICAL PHYSICAL ALLOCATION LINE
// ============================================================================
MySDClass myFileSys;        // This brings the object to life in memory!

// ============================================================================
// CLASS LIFECYCLE MANAGEMENT
// ============================================================================

// Constructor: Automatically clears the internal pointer at boot
MySDClass::MySDClass() {
  _fs = nullptr;
}

// Handoff: Captures the active filesystem address (LittleFS or SD)
void MySDClass::begin(fs::FS &fsInstance) {
  _fs = &fsInstance;
}


/*---------------    START SD FILE SYSTEM   ---------------*/

#ifdef SYSTEM_USES_SD_CARD

#ifdef SYSTEM_USES_SD_CARD

bool startSD(int cspin) {
  mySP("IN startFileSystem().\n", FN, LN);
  
  // 1. Mount the physical SD Card hardware partition
  if(!SD.begin(cspin)){                                                                     // start the SD system
    mySP("Card Mount Failed.\n", FN, LN);                                                   // if it failed to start log this
    return false;                                                                           // and exit the function
  }
  
  // ==========================================================================
  // 🎯 THE CLASS HANDOFF (Moved up for total safety and symmetry!)
  // ==========================================================================
  myFileSys.begin(SD);
  
  // 2. Query hardware details safely (Now fully protected by the active capsule)
  String s = "";
  switch(SD.cardType()) {
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
    default:
      s = "SD Card Type: UNKNOWN.";
  }
  mySP(s + "\n", FN, LN);
  
  int cardSize = (int)(SD.cardSize() / (1024 * 1024));                                      // get the size of the SD card
  mySP("SD Card Size: " + String(cardSize) + "MB.\n", FN, LN);                              // log the size of the SD card
  mySP("Total space: " + String((int)(SD.totalBytes() / (1048576))) + "MB.\n", FN, LN);     // log the total space
  
  return true;
}

#endif // SYSTEM_USES_SD_CARD

#endif // SYSTEM_USES_SD_CARD

/*---------------    START LTTLEFS FILE SYSTEM   ---------------*/

#ifdef SYSTEM_USES_LITTLEFS

bool startLittleFS() {
  mySP(F("Setting up the LittleFS file system.\n"), FN, LN);
  
  // 1. Mount the physical hardware partition map
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {                                               // load LittleFS
    mySP(F("LittleFS Mount Failed\n"), FN, LN);                                                 // log this
    return false;                                                                               // and exit the function
  }
  
  // ==========================================================================
  // 🎯 THE CLASS HANDOFF (Moved up for total safety)
  // ==========================================================================
  myFileSys.begin(LittleFS);
  
  // 2. Perform flash partition space diagnostics safely
  size_t totalBytesRaw = LittleFS.totalBytes();
  size_t usedBytesRaw  = LittleFS.usedBytes();
  float totalKB = totalBytesRaw / 1024.0;
  float usedKB  = usedBytesRaw / 1024.0;
  freeKB  = totalKB - usedKB;
  
  // You can now safely print or save these variables anywhere!
  mySP("LittleFS Total Space: " + String(totalKB, 1) + " KB\n", FN, LN);
  mySP("LittleFS Used Space:  " + String(usedKB, 1) + " KB\n", FN, LN);
  mySP("LittleFS Free Space:  " + String(freeKB, 1) + " KB\n", FN, LN);

  // Low space safety check
  if (freeKB < 10.0) {
    mySP("WARNING: Low disk space! Less than 10 KB remaining.\n", FN, LN);
  }
 
  return true;
}

#endif // SYSTEM_USES_LITTLEFS
/*---------------    DIRECTORY TO DISK FILE   ---------------*/

void MySDClass::dirToDiskFile(String dirname, String filename, int depth) {
  deleteFile(filename.c_str(), LN);
  writeFile(filename.c_str(), (listDir(dirname.c_str(), depth, ',')).c_str(), LN);
  int fSize = getFileSize(filename.c_str());
  mySP("Created new dirToDiskFile().  File name: '" + filename + "', size = '" + String(fSize) + "'\n", FN, LN);
}

/*---------------  LIST DIRECTORY TO FILE  ---------------*/

void MySDClass::listDirToFile(listDirStruct_ptr dl) {
  unsigned long exitAfter = 500;
  unsigned long t = millis();
  String dirListContents = "";
  
  if(!dl->inProcess) {
    // ---> FIXED: Dropped old global 'FS_IN_USE' parameter
    deleteFile(dl->fName.c_str(), LN);
    
    dl->root = _fs->open(dl->dirName);
    if (!dl->root) {
      writeFile(dl->dirName.c_str(), "Failed to open directory", LN);
      mySP("Failed to open directory\n", FN, LN);
      return;
    }
    
    // ---> FIXED: SD needs an explicit isDirectory check, LittleFS handles it via path maps
    #ifdef SYSTEM_USES_SD_CARD
    if (!dl->root.isDirectory()) {
      writeFile(dl->dirName.c_str(), (dl->dirName + " is not a directory").c_str(), LN);
      mySP(dl->dirName + " is not a directory\n", FN, LN);
      return;
    }
    #endif
    
    dl->inProcess = true;
  }

  // ==========================================================================
  // PROFILE 1: SD CARD SEQUENTIAL SCANNING
  // ==========================================================================
  #ifdef SYSTEM_USES_SD_CARD
  File file = dl->root.openNextFile();
  while (file) {
    if (millis() - t > exitAfter) {
      // ---> FIXED: Appends straight through the capsule tool
      appendFile(dl->fName.c_str(), dirListContents.c_str());
      file.close();
      return;
    }
    dirListContents += "  FILE: " + String(file.name()) + "  SIZE: " + String(file.size()) + ",";
    file = dl->root.openNextFile();
  }
  #endif

  // ==========================================================================
  // PROFILE 2: LITTLEFS FLASH SCANNING (Fully Synchronized!)
  // ==========================================================================
  #ifdef SYSTEM_USES_LITTLEFS
  File file = dl->root.openNextFile(); // 🟢 Fixed: Uses openNextFile() sequentially
  while (file) {
    if (millis() - t > exitAfter) {
      appendFile(dl->fName.c_str(), dirListContents.c_str());
      file.close();
      return;
    }
    dirListContents += "  FILE: " + String(file.name()) + "  SIZE: " + String(file.size()) + ",";
    file = dl->root.openNextFile(); // 🟢 Fixed: Moves iterator forward safely
  }
  #endif

  // ---> FIXED: Final append drops old 'FS_IN_USE' variable
  appendFile(dl->fName.c_str(), dirListContents.c_str());
  dl->inProcess = false;
  dl->complete = true;
  dl->root.close();
  
  mySP("listDirToFile() for '" + dl->dirName + "' is now complete. " +
        getFileSize(dl->fName.c_str()) + " bytes written to " + dl->fName + "\n", FN, LN);
}

/*---------------  LIST DIRECTORY  ---------------*/

String MySDClass::listDir(const char * dirname, uint8_t levels, char separator) {
  String s = "";
  unsigned long t = millis();
  unsigned long yieldTime = 1000;
  
  File root = _fs->open(dirname);
  if (!root) {
    return "Failed to open directory";
  }

#ifdef SYSTEM_USES_SD_CARD
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
      s += "  DIR : " + String(file.name()) + ",";
      if (levels) {
        s += listDir(file.name(), levels - 1, separator);
      }
    } else {
      s += "  FILE: " + String(file.name()) + "  SIZE: " + String(file.size()) + separator;
    }
    file = root.openNextFile();
  }
  #endif // SYSTEM_USES_SD_CARD

  #ifdef SYSTEM_USES_LITTLEFS
  // Flat directory translation loop for pure flash partitioning maps
  // 🟢 FIXED: Changed openFile("r") to openNextFile() using 'root'
  File file = root.openNextFile();
  while (file) {
    if (millis() - t > yieldTime) {
      yield();
      t = millis();
    }
    s += "  FILE: " + String(file.name()) + "  SIZE: " + String(file.size()) + separator;
    
    // 🎯 FIXED: Changed trailing openFile("r") to openNextFile() using 'root'
    file = root.openNextFile();
  }
  #endif
  yield();
  root.close();
  return s;
}

/*---------------  CREATE DIRECTORY  ---------------*/

bool MySDClass::createDir(const char * path) {
  if (_fs->mkdir(path)) {
    return true;
  } else {
    return false;
  }
}

/*---------------  DELETE DIRECTORY  ---------------*/

void MySDClass::removeDir(const char * path) {
  mySP("Removing Dir: " + String(path) + "\n", FN, LN);
  if (_fs->mkdir(path)) {
    mySP("Dir removed\n", FN, LN);
  } else {
    mySP("rmdir failed\n", FN, LN);
  }
}

/*---------------  MY READ FILE  ID  ---------------*/

String MySDClass::readFile(const char * path, bool logFile) {
  if (!logFile) mySP("Reading file: " + String(path) + "\n", FN, LN);                         // if debugging log the file path
  String s = "";                                                                              // create an empty string
  
  // ---> CHANGE: Use the internal class pointer arrow instead of 'fs.'
  File file = _fs->open(path, FILE_READ);                                                       // open the file
  
  if (!file) {                                                                                // if the file was not opened
    mySP("ERROR 100: Failed to open file '" + String(path) + "' for reading\n", FN, LN);      // log the error
    return "ERROR 100: Failed to open file '" + String(path) + "' for reading";                // and return the error to the calling function
  }
  
  int fSize = file.available();
  if (false) {                                                                                // if debugging
    mySP(String(path) + ": Available bytes to be read are: " + String(fSize) + "\n", FN, LN); // log the bytes available to be read
  }
  
  char c;                                                                                     // declar a char 'c'
  while (file.available()) {                                                                  // while file.available > 0
    c = file.read();                                                                          // read the next char from the file
    s += String(c);                                                                           // and append it to 's'
  }
  file.close();                                                                               // close the file
  
  if (logFile) {
    mySP("Reading file: " + String(path) + + ", contents: " + s + "\n", FN, LN);              // log the file path and file contents
  }
  return s;                                                                                   // return 's' to the calling function
}

/*---------------  READ FILE SPECIFICALLY FOR USE WITH ESPAsyncWebServer  ---------------*/

 size_t MySDClass::load_data(File f, uint8_t *buffer, size_t maxLen, size_t index) {
   if (f.available()) {
     // FIXED: Log output moved ABOVE the return statement so it actually executes!
     mySP("Reading file:'" + String(f.name()) + "' maxLen ='" + String(maxLen) + "'\n", FN, LN);   // log that the file was read from
     return f.read(buffer, maxLen);                                                                // read maxLen bytes from the file 'f'
   } else {                                                                                        // else, there must be no bytes left in the file to read
     downloadingFile = false;                                                                      // tell the server the download is complete
     mySP("Web page file download '" + String(f.name()) + "' has completed.\n", FN, LN);           // log the read is complete
     f.close();                                                                                    // close the file
     return 0;                                                                                     // and return the length of the read...zero bytes
   }
   return 0;                                                                                       // and return the length of the read...zero bytes
 }

/*---------------  WRITE FILE  IDS 122---------------*/

int MySDClass::writeFile(const char * path, const char * message, int from) {
  // ---> CHANGE: Use the internal class pointer arrow instead of 'fs.'
  File file = _fs->open(path, FILE_WRITE);                                        // open the file in a writing mode
 
  if (!file) {                                                                  // if the file failed to be opened
    mySP(getLogTime() + ", Failed to open file '" + String(path) +              // log that  the file failed to open
        "' for writing.\nj", FN,  from);
    return -1;                                                                  // and return to the calling function
  }
 int n = file.print(message);                                                  // write the file capturing the number of bytes written
  if (n == (String(message)).length()) {                                        // if the number of bytes written differes from the size of the message being written
    mySP(String(n) + " bytes written to '" + String(path) + "\n", FN, from);    // log the number of bytes written to file 'path'
  } else {                                                                      // else no bytes were written so
    mySP("Write failed to file '" + String(path) + "n = " + String(n) + "\n", FN, from); // log the file failed to be written
  }
  file.close();                                                                 // close the file
  return n;
}

/*---------------  CREATE FILE  ---------------*/

bool MySDClass::createFile(const char * path, const char * message) {
  File file = _fs->open(path, FILE_WRITE);                                        // open the file in a writing mode
  if (!file) {                                                                  // if the file failed to be opened
    return false;
  }
  int n = file.print(message);                                                  // write the file capturing the number of bytes written
  if (n) {                                                                      // if some bytes were wrtten
    mySP(String(n) + " bytes written to '" + String(path) + "'\n", FN, LN);     // log the number of bytes written to file 'path'
  } else {                                                                      // else no bytes were written so
    mySP("Write failed to file '" + String(path) + "'\n", FN, LN);              // log the file failed to be written
    file.close();                                                                 // close the file
    return false;
  }
  file.close();                                                                 // close the file
  return true;
}

/*---------------  APPEND TO FILE  ID 129---------------
path            FQN of the file
message         Data to append to the file
return          the number of bytes written to the file
*/

int MySDClass::appendFile(const char * path, const char * message) {
  File file = _fs->open(path, FILE_APPEND);                                                                       // open the 'path' file
  if (!file) {                                                                                                  // if it failed to be opened
    mySP(getLogTime() + ",Failed to open file '" + String(path) + "' for appending.\n", FN, LN);                // advise via the serial monitor
    return 0;                                                                                                   // and return to the calling function
  }
  int n = file.print(message);                                                                                  // append 'message' to the file
  if (n != strlen(message)) {                                                                                   // if written bytes are zero
    mySP("Append failed to file'" + String(strlen(message)) + "' bytes to '" + String(path) + "'\n", FN, LN);   // log this
  }
  file.close();                                                                                                 // close the file
  return n;
}

/*---------------  RENAME FILE  ---------------*/

void MySDClass::renameFile(const char * path1, const char * path2) {
  if (_fs->rename(path1, path2)) {
    mySP("File renamed\n", FN, LN);
  } else {
    mySP("Rename failed", FN, LN);
  }
}

/*---------------  DELETE FILE  ID 132---------------*/

void MySDClass::deleteFile(const char * path, int from) {
  
  if(_fs->exists(path) == 0) {
    mySP("File delete failed.  The file '" + String(path) + "' does not exist.\n", FN, from);
    return;
  }
  if (_fs->remove(path)) {
    mySP("File deleted '" + String(path) + "'\n", FN, from);
    return;
  }
  mySP("Delete failed '" + String(path) + "'\n", FN, from);
}

/*---------------  TEST FILE  ---------------*/

void MySDClass::testFileIO(const char * path) {
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
}

/*---------------  GET FILE SIZE  ---------------*/

int MySDClass::getFileSize(const char * path) {
  int fSize = 0;
  File file = _fs->open(path);                                                // open the file 'path'
  if (!file) {                                                              // if it failed to be opened
    mySP("At " + getLogTime() + ",Failed to open file '" + String(path)     // log the failure
         + "'.\n", FN, LN);
    return -1;                                                              // and return a -1 the calling function
  }
  fSize = file.size();                                                      // get the size of the file
  file.close();                                                             // close the file
  return fSize;                                                             // return the file size to the calling function
}

/*---------------    CREATE DIRECTOR IF IT DOES NOT EXIST---------------*/
// 5/199/2024 - added the parameter 'fs::FS & fs'

bool MySDClass::createDirIfNotExist(String dirName) {
  if (!_fs->exists(dirName.c_str())) {                                                // if the 'dirname' does not exist
    mySP("The directory '" + dirName + "' does not exist, creating it.\n", FN, LN); // log the action
    bool result = createDir(dirName.c_str());                                   // and create the directory and return the success of this
    if(!result) {                                                                   // if the directory failed to be created then
      mySP("ERROR: Failed to create the directory '" + dirName + "'\n", FN, LN);    // log the error
    }
    return result;                                                                  // return the result
  } else {                                                                          // else
    mySP("The directory '" + dirName + "' exists.\n", FN, LN);                      // log the status
    return true;                                                                    // return true
  }
}

/*--------------- NEW LAZY FILE INITIALIZATION FUNCTION ---------------*/
/*
 String (*dataGenerator)() is a pointer to a function that takes no
 parameters and returns a String containing the default contents for
 the file.
*/

int MySDClass::ensureFileExistsLazy(String fname, String (*dataGenerator)(), int from) {
  // 1. Create a variable to hold our final answer, defaulting to an error state
  int statusResult = FILE_ERROR;
  
  // SCENARIO 1: The file already exists on disk
  if (_fs->exists(fname)) {
    mySP("The file '" + fname + "' exists.\n", FN, from);
    statusResult = FILE_ALREADY_EXISTS; // Explicitly set it here
    return statusResult;
  }

  // SCENARIO 2: The file is missing, generate defaults
  String data = dataGenerator();
  if (data == "") {
    mySP("The file '" + fname + "' does not exist.\n", FN, from);
    return FILE_ERROR;
  }

  // Write the default data using your standardized routine
  int bytesWritten = writeFile(fname.c_str(), data.c_str(), from);
  
  // 3. Explicitly verify the output of your write action
  if (bytesWritten >= 0) {
    mySP("Successfully created missing file: '" + fname + "'\n", FN, from);
    statusResult = FILE_WAS_CREATED; // Explicitly set it ONLY upon a successful write!
  } else {
    mySP("ERROR: Failed to create file '" + fname + "'\n", FN, from);
    statusResult = FILE_ERROR;
  }

  return statusResult; // Return the explicitly tracked state
}


/*---------------    IF FILE DOES/DOES NOT EXIST ---------------*/

bool MySDClass::ensureFileExists(String fname, String data, int from) {
  if(!_fs->exists(fname)) {                                                       // if the file does not exist
    if(data == "") {                                                            // if 'data' is an empty string
      mySP("The file '" + fname + "' does not exist. Please copy it to the "   // log the error and ask for resolution
           + fsInUse + " file system\n", FN, from);
      return false;
    } else {                                                                    // data is not an empty string
      bool success = createFile(fname.c_str(), data.c_str());               // so create the file and write 'data' into it
      if(!success) {                                                            // if the file failed to be created and populated
        mySP("ERROR: Failed to create the file '" + fname + "'\n", FN, from);
      } else {
        mySP("Successfully created missing file: '" + fname + "'\n", FN, from);
      }
      return success; // Returns true if the file was successfully created fresh
    }
  } else {
    mySP("The file '" + fname + "' exists.\n", FN, from);                       // log that it exists
    return true;                                                                // the file existed so return true
  }
}
/*---------------    PRINT DIRECTORY LIST STRUCT   ---------------*/

void MySDClass::printDL_struct(listDirStruct_ptr dl, int from) {
  mySP("Directory list structure:"
       "\n    fName = " + dl->fName +
       "\n    dirName = " + dl->dirName +
       "\n    header = " + dl->header +
       "\n    root = " + String(dl->root?"in use":"not in use") +
       "\n    depth = " + String(dl->depth) +
       "\n    inProcess = " + String(dl->inProcess?"In process":"Not in process") +
       "\n    complete = " + String(dl->complete?"Complete":"Not complete") +
       "\n", FN, from);
}

/*---------------  GET RAW FILESYSTEM REFERENCE  ---------------*/

fs::FS& MySDClass::getFS() {
  // Follow the pointer arrow, dereference it with (*), and hand back the raw object
  return *_fs;
}
