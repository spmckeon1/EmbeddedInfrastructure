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
#include <ei_system.h>
#include <ei_time.h>

struct WebClient {
  AsyncWebSocketClient* client;   // Connection identity
  uint32_t clientId;

  String pgName;                  // Registration
  IPAddress ip;                   // Network

  uint32_t connectedAt;           // Statistics
  uint32_t lastActivity;
};

class AsyncWebSocket;
class AsyncWebSocketClient;

class Web
{
public:
  void start();
  bool startup();
  void evtLoop();
  void setDownloadingFile(bool downloading) { _downloadingFile = downloading; }
  void sendWS_msg(const String& message, AsyncWebSocketClient* client);
  void hdlWiFiSetupEvent(String s,AsyncWebSocketClient* client);
  void initNewWiFiPg(String s, AsyncWebSocketClient *client);
  bool downloadingFile() const;
  const char *getContentType(const String &path) const;
  void processMsg(const JsonDocument& doc);

private:
  AsyncWebServer _server{80};
  AsyncWebSocket _ws{"/ws"};
  String _incomingFilePath;
  String _downloadLocation;
  bool _downloadingFile = false;
  
  static constexpr uint8_t MAX_WEB_CLIENTS = 10;
  WebClient _clients[MAX_WEB_CLIENTS];


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
  
  WebClient* findFreeClient();
  WebClient* findClient(AsyncWebSocketClient* client);
  void clearClient(WebClient* client);
  WebClient* addClient(AsyncWebSocketClient* client);
  bool setClientPage(AsyncWebSocketClient* client, const String& pgName);

};

extern Web web;

