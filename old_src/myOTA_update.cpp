//
//  myOTA_update.cpp
//  REQUIREMENTS:
//    'firmwareUpdate.html' version 1.0 or greater is required.  This works on an ESP32, tested, and should work on an ESP8266 (untested).
//    firmwareUpdate returns 'iAmAFWUpdatePage' when launched.
//    The applications 'webSocketEvent()' function must have a call to:
//      'hdlNewFW_updatePg(s, client, constDataPtr()' to handle the opening of a new 'firmwareUpdate.html' web page
//  Created by Stephen McKeon on 6/28/24.
//

#include <stdio.h>
#include <myOTA_update.h>

int updateProgress = 0;                                                 // keeps track of the software udate percentage
Ticker goBackToMainWebPg;                               // delays the listening to of MQTT msgs after mqtt reconnect


/*--------------- HANDLE NEW FIRMWARE UPDATE PAGE ---------------*/

void hdlNewFW_updatePg(String s, AsyncWebSocketClient *client, const_dataPtr cdp) {
  mySP("Firmware update page attaching.\n", FN, LN);                            // log it
  sendWS_msg(FN, "newHeader:" + cdp->pageHeader, client);                       // set header ID
  sendWS_msg(FN, "newPgTitle:" + cdp->pgTitle, client);                         // set the page title

}

 /*--------------- UPDATES THE CODE (REPLACES IT) VIA A WIFI CONNECTION ---------------*/

 void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
   if (!index) {
     // Correctly capture content length on the first chunk
     content_len = request->contentLength();
     updateProgress = 0; // Reset progress tracker
     
     // Determine if we are updating a file system partition or firmware core code
     int cmd = (filename.indexOf("LittleFS") > -1 || filename.indexOf("SPIFFS") > -1) ? U_PART : U_FLASH;
     
     mySP("Starting Update. Total Size Estimate: " + String(content_len) + " bytes\n", FN, LN);

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
     printProgress(index + len, content_len);
   }
   
   if (final) {
     if (!Update.end(true)) {
       Update.printError(Serial);
     } else {
       sendWS_msg(FN, "Progress: 100", NULL);
       mySP("Flash verification success. Flushing network queues...\n", FN, LN);
       
       // 1. Force clear all local serial logging channels
       Serial.flush();
       
       // 2. THE NON-BLOCKING DELAY: Yield control back to the network stack for 3 full seconds.
       // This guarantees the Wi-Fi radio successfully transmits the final 200 OK packet to your Mac.
       unsigned long startFlushTime = millis();
       while (millis() - startFlushTime < 3000) {
         yield(); // Keeps the single-core background network handlers active
       }
       
       // 3. Safe reboot now that the browser has closed the transaction cleanly
       mySP("Rebooting device now!\n", FN, LN);
       ESP.restart();
     }
   }
 }

 /*--------------- UPDATES THE PROGRESS BAR VIA WEBSOCKET ---------------*/

 void printProgress(size_t prg, size_t sz) {
   // Prevent fatal crash if system failed to read content length header
   if (sz <= 0) return;

   int percentDone = (prg * 100) / sz;
   
   // Only send a WebSocket packet if the percentage has actually changed
   if (percentDone != updateProgress) {
     updateProgress = percentDone;
     
     // Fire the message out. Your JavaScript parses "Progress:X" to move the bar
     sendWS_msg(FN, "Progress:" + String(percentDone), NULL);
     mySP("Progress: " + String(percentDone) + "%\n", FN, LN);
   }
 }

/*--------------- UPDATES THE CODE (REPLACES IT) VIA A WIFI CONNECTION ---------------*

void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  static int lastPercentDone = 0;
  if (!index) {
    content_len = request->contentLength();
    int cmd = (filename.indexOf("LittleFS") > -1) ? U_PART : U_FLASH;           // if filename includes LittleFS, update the LittleFS partition
#ifdef ESP8266
    Update.runAsync(true);
    if (!Update.begin(content_len, cmd)) {
#else
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
#endif
      Update.printError(Serial);
    }
  }
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
#ifdef ESP8266
  } else {
    int percentDone = (Update.progress() * 100) / Update.size();
    if(percentDone != lastPercentDone) {
      sendWS_msg(FN, "Progress:" + String(percentDone), NULL);
      mySP("Progress:" + String(percentDone) + "\n", FN, LN);
      lastPercentDone = percentDone;
    }
#endif
  }
  if (final) {
    if (!Update.end(true)) {
      Update.printError(Serial);
    } else {
      sendWS_msg(FN, "Progress: 100", NULL);
      mySP("Progress: 100\n", FN, LN);
      mySP("Restarting device...\n", FN, LN);
      requestReboot("Firmware update complete, requesting a reboot", false);    // request the server reboot
    }
  }
}

/*--------------- UPDATES THE CODE (REPLACES IT) VIA A WIFI CONNECTION ---------------*

  void printProgress(size_t prg, size_t sz) {
    int percentDone = (prg * 100) / content_len;
    if (percentDone != updateProgress) {
      sendWS_msg(FN, "Progress:" + String(percentDone), NULL);
      mySP("Progress:" + String(percentDone) + "\n", FN, LN);
      updateProgress = percentDone;
    }
  }

*/
