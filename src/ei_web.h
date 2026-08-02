#pragma once
//
//  ei_web.h
//
//
//  Created by Stephen McKeon on 7/21/26.
//

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ei_types.h>

class AsyncWebSocket;
class AsyncWebSocketClient;

class Web
{
public:
  bool startup();
  void evtLoop();
  void setDownloadingFile(bool downloading) { _downloadingFile = downloading; }
  void sendWS_msg(const String& message, AsyncWebSocketClient* client);
  void hdlWiFiSetupEvent(String s,AsyncWebSocketClient* client);
  void initNewWiFiPg(String s, AsyncWebSocketClient *client);
  bool downloadingFile() const;;


private:
  AsyncWebServer _server{80};
  AsyncWebSocket _ws{"/ws"};
  String _incomingFilePath;
  String _downloadLocation;
  bool _downloadingFile = false;

  bool handleConfigurationUpdate(String s);
  bool handleIncomingFile(String s);
  void gatherWiFiSetupData(String &jsonOutput);
  bool handleDownloadLocation(String s, AsyncWebSocketClient* client);
  bool handleFileSizeRequest(String s, AsyncWebSocketClient* client);
  bool startWebServer();
  void handlePostFile(AsyncWebServerRequest* request,
                      const String& filename,
                      size_t index,
                      uint8_t* data,
                      size_t len,
                      bool final);
  bool startWebSocket();
  bool tryServeStaticFile(AsyncWebServerRequest *request);
};

extern Web web;

