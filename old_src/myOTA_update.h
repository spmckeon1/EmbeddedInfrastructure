//
//  myOTA_uodate.h
//  
//
//  Created by Stephen McKeon on 6/28/24.
//

#ifndef __MYOTA_UPDATE_H__
#define __MYOTA_UPDATE_H__

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <myStuff.h>
#include <commonItems_ESP32.h>

void hdlNewFW_updatePg(String s, AsyncWebSocketClient *client, const_dataPtr cdp);
void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
void printProgress(size_t prg, size_t sz);


#endif /* __MYOTA_UPDATE_H__ */
