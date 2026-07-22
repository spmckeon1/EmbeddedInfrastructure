//
//  ei-startup.cpp
//  
//
//  Created by Stephen McKeon on 7/17/26.
//

#include <Arduino.h>
#include <ei_storage.h>
#include <ei_startup.h>

/*---------------    COMMON FILE SYSTEM STARTUP ACTIONS   ---------------*/

//  STARTUP FILE CHECKS ARE BEING MOVED TO THE RESPONSIBLE SYSTE TO HANDLE
//void startupFileChk() {
  
//  storage.createDirIfNotExist(appDir);                                       // ensure each of the directories needed exist n the SD
//  storage.createDirIfNotExist(dirLists);                                     // make sure the dir list directory exists
//  storage.createDirIfNotExist(HTMLFsDir);                                    // make sure the html directory exists
//  storage.ensureFileExists(allDirectoriesToFile.c_str(), DIRS_IN_USE, LN);         // used by the update page to know what directories to advertise
//  storage.ensureFileExists(lastBootTimeFname.c_str(), "", LN);                     // if the boot file time file does not exist then create it as an empty file
//  storage.ensureFileExists(UPD_HTML_FILE.c_str(), "", LN);                         // if the firmware update file does not exist then log the action needed
//  if(storage.ensureFileExistsLazy(netInfoFname, []() {
//    String out; LogInDataToStr(out, logInDataPtr, true);
//    return out;}, LN) == FILE_ALREADY_EXISTS) {
//      readNetInfoFromDisk(netInfoFname);
//  }
//}

