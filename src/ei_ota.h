#pragma once


//
//  myOTA_uodate.h
//  
//
//  Created by Stephen McKeon on 6/28/24.
//

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

struct OtaState
{
    bool active = false;
    size_t contentLength = 0;
    size_t bytesReceived = 0;
};


class EiOTA {
public:
  bool startup();
  void evtLoop();

  void hdlNewFW_updatePg(String s, AsyncWebSocketClient* client);

  void handleDoUpdate(AsyncWebServerRequest* request,
                      const String& filename,
                      size_t index,
                      uint8_t* data,
                      size_t len,
                      bool final);

private:
  OtaState _state;
  void printProgress(size_t prg, size_t sz);

  int _updateProgress = 0;
};

extern EiOTA ota;
