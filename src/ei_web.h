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
#include <ei_webProvisioning.h>

struct WebClient {
  AsyncWebSocketClient* client = nullptr;   // Connection identity
  uint32_t clientId = 0;
  String pgName;
  IPAddress ip;
  uint32_t connectedAt = 0;
  uint32_t lastActivity = 0;
};

class AsyncWebSocket;
class AsyncWebSocketClient;

class Web
{
public:
  bool setup();
  bool startup();
  void evtLoop();
  void setDownloadingFile(bool downloading) { _downloadingFile = downloading; }
  void sendWS_msg(const String& message, AsyncWebSocketClient* client);
  void webPubMsg(const JsonDocument& doc);
  bool downloadingFile() const;
  const char *getContentType(const String &path) const;
  void onWsEvent(AsyncWebSocket* server,
                        AsyncWebSocketClient* client,
                        AwsEventType type,
                        void* arg,
                        uint8_t* data,
                        size_t len);
  bool processMsg(const JsonDocument& doc);
  bool isPageConnected(const String& page);

private:
  
  WebClient* _clients = nullptr;
  uint8_t _maxWebClients = 0;
  
  AsyncWebServer _server{80};
  AsyncWebSocket _ws{"/ws"};
  String _incomingFilePath;
  String _downloadLocation;
  bool _downloadingFile = false;
  const Source _source = Source::WEB;
  static constexpr uint8_t MAX_WEB_CLIENTS = 10;

  bool handleIncomingFile(String s);
  bool validateTxtMsg(uint8_t* data, size_t len, JsonDocument& doc);
  bool verifyRecMsg(const JsonDocument& doc, const String& json, const char* eventType);
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
  void processConnect(const JsonDocument& doc);
  void registerClient(AsyncWebSocketClient* client);
  
};

extern Web web;

