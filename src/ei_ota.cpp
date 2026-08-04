//
//  myOTA_update.cpp
//  REQUIREMENTS:
//    'firmwareUpdate.html' version 1.0 or greater is required.  This works on an ESP32, tested, and should work on an ESP8266 (untested).
//    firmwareUpdate returns 'iAmAFWUpdatePage' when launched.
//    The applications 'webSocketEvent()' function must have a call to:
//      'hdlNewFW_updatePg(s, client, constDataPtr()' to handle the opening of a new 'firmwareUpdate.html' web page
//  Created by Stephen McKeon on 6/28/24.
//

#include <Update.h>

#include <ei_appPolicy.h>
#include <ei_logging.h>
#include <ei_web.h>
#include <ei_ota.h>


int updateProgress = 0;                                                 // keeps track of the software udate percentage



EiOTA ota;

/*----- EiOTA SET UP -----*/

bool EiOTA::setup() {
  _updateHtmlFile = appDirs.htmlDir + "/firmwareUpdate.html";
  return true;
}
/*----- EiOTA EVENT LOOP -----*/

bool EiOTA::startup() {
  return true;
}

void EiOTA::evtLoop() {
  
}


/*--------------- HANDLE NEW FIRMWARE UPDATE PAGE ---------------*/

void EiOTA::hdlNewFW_updatePg(String s, AsyncWebSocketClient *client) {
  logInfo(LS, ET::OTA,"Firmware update page attaching.");                              // log it
  web.sendWS_msg("newHeader:" + String(appIDs.pageHeader), client);               // set header ID
  web.sendWS_msg("newPgTitle:" + String(appIDs.pageTitle), client);               // set the page title

}

 /*--------------- UPDATES THE CODE (REPLACES IT) VIA A WIFI CONNECTION ---------------*/

 void EiOTA::handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
   if (!index) {
     // Correctly capture content length on the first chunk
     _state.contentLength = request->contentLength();
     updateProgress = 0; // Reset progress tracker
     
     // Determine if we are updating a file system partition or firmware core code
     int cmd = (filename.indexOf("LittleFS") > -1 || filename.indexOf("SPIFFS") > -1) ? U_SPIFFS : U_FLASH;
     
     logInfo(LS, ET::OTA,"Starting Update. Total Size Estimate: " + String(_state.contentLength) + " bytes");

     if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
       Update.printError(Serial);
     }
   }
   
   // Write the current data chunk to flash memory
   if (Update.write(data, len) != len) {
     Update.printError(Serial);
   } else {
     // SUCCESSFUL WRITE: Fire the progress bar updates for ESP32
     // Pass the current accumulated bytes written (index + len) and total expected length
     printProgress(index + len, _state.contentLength);
   }
   
   if (final) {
     if (!Update.end(true)) {
       Update.printError(Serial);
     } else {
       web.sendWS_msg("Progress: 100", NULL);
       logInfo(LS, ET::OTA,"Flash verification success. Flushing network queues...");
       
       // 1. Force clear all local serial logging channels
       Serial.flush();
       
       // 2. THE NON-BLOCKING DELAY: Yield control back to the network stack for 3 full seconds.
       // This guarantees the Wi-Fi radio successfully transmits the final 200 OK packet to your Mac.
       unsigned long startFlushTime = millis();
       while (millis() - startFlushTime < 3000) {
         yield(); // Keeps the single-core background network handlers active
       }
       
       // 3. Safe reboot now that the browser has closed the transaction cleanly
       logInfo(LS, ET::OTA,"Rebooting device now!");
       ESP.restart();
     }
   }
 }

 /*--------------- UPDATES THE PROGRESS BAR VIA WEBSOCKET ---------------*/

 void EiOTA::printProgress(size_t prg, size_t sz) {
   // Prevent fatal crash if system failed to read content length header
   if (sz <= 0) return;

   int percentDone = (prg * 100) / sz;
   
   // Only send a WebSocket packet if the percentage has actually changed
   if (percentDone != updateProgress) {
     updateProgress = percentDone;
     
     // Fire the message out. Your JavaScript parses "Progress:X" to move the bar
     web.sendWS_msg("Progress:" + String(percentDone), NULL);
     logInfo(LS, ET::OTA,"Progress: " + String(percentDone) + "%");
   }
 }

