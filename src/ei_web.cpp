//
//  web.c
//  
//
//  Created by Stephen McKeon on 7/21/26.
//

// EmbeddedInfrastructure intentionally supports one active file upload.
// Reject additional uploads until the current upload completes.

#include <ESPAsyncWebServer.h>

#include <ei_logging.h>
#include <ei_mqtt.h>
#include <ei_network.h>
#include <ei_types.h>
#include "ei_web.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Web web;

/*-----    WHEN WIFI STARTS   -----*/

static void onWifiConnected() {
  web.start();
}

/*-----    START THE WEBSERVER   -----*/

void Web::start() {
  logInfo(LS, ET::WEB, "Starting web server...");
  _server.begin();
  logInfo(LS, ET::WEB, "Web server started.");
}

/*-----    DO THE Web STARTUP ACTIONS   -----*/

bool Web::startup()
{
  if (!startWebServer())
      return false;
  if (!startWebSocket())
      return false;
  eiEvents.on(EiEvent::WifiConnected, onWifiConnected);
  return true;
}

/*-----    DO THE Web EVENT LOOP   -----*/

void Web::evtLoop() {
    // Nothing required at the moment.
    // Future:
    //   - websocket housekeeping
    //   - upload timeouts
    //   - client cleanup
}

/*-----    This is the little wrapper that gets from the C-style callback into the class.   -----*

static void onWsEvent(AsyncWebSocket* server,
                      AsyncWebSocketClient* client,
                      AwsEventType type,
                      void* arg,
                      uint8_t* data,
                      size_t len)
{
    web.onWsEvent(server, client, type, arg, data, len);
}

/*-----    HANDLE THE WEB EVENT   -----*

void Web::onWsEvent(AsyncWebSocket* server,
                      AsyncWebSocketClient* client,
                      AwsEventType type,
                      void* arg,
                      uint8_t* data,
                      size_t len)
{
    switch (type) {

    case WS_EVT_DATA:
    {
        String s((char*)data, len);
        hdlWiFiSetupEvent(s, client);
        break;
    }

    case WS_EVT_CONNECT:
        // Optional logging
        break;

    case WS_EVT_DISCONNECT:
        // Optional logging
        break;

    default:
        break;
    }
}

/*-----    SEND A MESSAGE TO A WEB PAGE   -----*/

void Web::sendWS_msg(const String& message, AsyncWebSocketClient* client) {
  if (client != nullptr)
    client->text(message);
  else
    ws.textAll(message);
}

/*-----    DISPATCH AN INCOMNG WIFI SETUP EVENT   -----*/

void Web::hdlWiFiSetupEvent(String s, AsyncWebSocketClient* client) {
  logInfo(LS, ET::WEB,"Incoming string: " + s);
  if (handleConfigurationUpdate(s))      return;
  if (handleIncomingFile(s))             return;
  if (handleDownloadLocation(s, client)) return;
  if (handleFileSizeRequest(s, client))  return;
}

/*-----    HANDLE THE WIFI SETUP CONFIGURATION EVENT   -----*/

bool Web::handleConfigurationUpdate(String s) {
  if (s.indexOf("cfgDataJSON:") == -1) return false;
  int jsonStartPos = s.indexOf("cfgDataJSON:") + 12;
  String jsonPayload = s.substring(jsonStartPos);
  logInfo(LS, ET::WEB, "Isolated JSON Text Block: '" + jsonPayload);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonPayload);
  if (error) {
    logError(LS, ET::WEB, "ERROR: Web form JSON parsing failed! Reason: " + String(error.c_str()));
    return true;
  }
  if (!network.configureFromJson(doc)) return true;
  if (!mqtt.configureFromJson(doc)) return true;
  logInfo(LS, ET::WEB, "Web configuration successfully updated.");
  return true;
}

/*-----    HANDLE THE INCOMING FLE   -----*/

bool Web::handleIncomingFile(String s) {
  if (s.indexOf("incomingFile") == -1) return false;
  String ts = s.substring(0, 25);
  if (ts.indexOf("incomingFileComplete") != -1) {
    Serial.println("file download is complete.");
    logInfo(LS, ET::WEB, "Downloaded '" + _incomingFilePath +
            "' - '" +
            String(storage.getFileSize(_incomingFilePath.c_str())) +
            " bytes");
    _incomingFilePath = "";
  } else {
    _incomingFilePath = s.substring(s.indexOf(":") + 1,
                       s.indexOf("|"));
    storage.appendFile(
      _incomingFilePath.c_str(),
      s.substring(s.indexOf("|") + 1).c_str());
  }

  return true;
}

/*-----    HANDLE THE DOWNLOAD LOCATION EVENT   -----*/

bool Web::handleDownloadLocation(String s, AsyncWebSocketClient* client) {
  if (s.indexOf("downloadLocation:") == -1) return false;
  _downloadLocation = s.substring(s.lastIndexOf(":") + 1);
  logInfo(LS, ET::WEB, "Download location = '" + _downloadLocation + "'");
  sendWS_msg("DownLoadLocRec:" + _downloadLocation, client);
  return true;
}


bool Web::handleFileSizeRequest(String s, AsyncWebSocketClient* client) {
  if (s.indexOf("fileSizePlease:") == -1) return false;
  client->text("requestedFileSizeIs:" + storage.getFileSize(_incomingFilePath.c_str()));
  return true;
}

/*-----    HANDLE A NEW WIFI SETUP WEB PAGE   -----*/

void Web::initNewWiFiPg(String s, AsyncWebSocketClient *client) {
  String outboundData = "";
  String CUID = s.substring(s.lastIndexOf(":") + 1);                                              // split the clients UID out of the string
  String ip = client->remoteIP().toString();                                                      // get its IP address
  logInfo(LS, ET::WEB, "A new WiFi setup web page has joined.  ClientId:'"+String(client->id())+           // log the new web page that joined
              "', Type :'WiFi setup', IP Address:'"+ ip + "', CUID:'" + CUID + "'");
  sendWS_msg("serverip:" + network.getIPAddress(), client);                                       // send the client the servers ip address
  sendWS_msg("clientip:" + ip, client);                                                           // send the client its ip address
  sendWS_msg(String("pgHeader:") + appIDs.pageHeader, client);                                    // send the page header
//  sendWS_msg("listDir:" + storage.readFile(allDirectoriesToFile.c_str(), true), client);
  gatherWiFiSetupData(outboundData);
  sendWS_msg("initWebDataJSON:" + outboundData, client);                                          // send the web page the info needed to setup the data fields
 }

/*---------------  GATHER WIFI SETUP DATA  ---------------*/

void Web::gatherWiFiSetupData(String& jsonOutput) {
    JsonDocument doc;

    const NetworkConfig& networkCfg = network.config();
    const MqttConfig& mqttCfg = mqtt.config();

    doc["networkSsid"] = networkCfg.ssid;
    doc["networkPass"] = networkCfg.password;

    doc["mqttServer"] = mqttCfg.host;
    doc["mqttPort"]   = mqttCfg.port;
    doc["mqttUser"]   = mqttCfg.brokerUser;
    doc["mqttPass"]   = mqttCfg.brokerPwd;

    serializeJson(doc, jsonOutput);
}

/*---------------  START THE WEB SERVER  ---------------*/

bool Web::startWebServer() {
  logWarn(LS, ET::WEB, "startWebServer() TDB...");
  _server.on("/post", AsyncWebRequestMethod::HTTP_POST,
      [](AsyncWebServerRequest *request) { },
      [this](AsyncWebServerRequest *request,
             const String& filename,
             size_t index,
             uint8_t *data,
             size_t len,
             bool final)
      {
          handlePostFile(request, filename, index, data, len, final);
      });
  _server.onNotFound(
      [this](AsyncWebServerRequest *request)
      {
          if (tryServeStaticFile(request))
              return;

          request->send(404, "text/plain", "404 - File Not Found");
      });
  
  logInfo(LS, ET::WEB, "Starting web server...");
  //registerRoutes();                                               // Register built-in HTTP routes.
  //registerApplicationRoutes();                                    // Allow the application to register additional routes.
  _server.begin();                                                // Start accepting HTTP requests.
  logInfo(LS, ET::WEB, "Web server started.");
  return true;
}

/*---------------  START THE WEB SOCKET  ---------------*/

bool Web::startWebSocket() {
  logWarn(LS, ET::WEB, "startWebSocket() TDB...");
  return true;
}

/*---------------  PUBLIC: LETR A REQUESTER KNOW IF A DOWNLOAD IS OCCURING  ---------------*/

bool Web::downloadingFile() const
{
    return _downloadingFile;
}

/*---------------  HANDLE THE POST FILE DOWNLOAD ACTIONS  ---------------*/

void Web::handlePostFile(AsyncWebServerRequest* request,
                         const String& filename,
                         size_t index,
                         uint8_t* data,
                         size_t len,
                         bool final)
{
  static File file;
  if (index == 0) {
    if (_downloadingFile) {
      logWarn(LS, ET::WEB, "Rejecting upload. Another upload is already in progress.");
      request->send(409);
      return;
    }
    _downloadingFile = true;
    _incomingFilePath = _downloadLocation + "/" + filename;
    storage.deleteFile(_incomingFilePath.c_str(), LN);
    logInfo(LS, ET::WEB, "Downloading '" + filename + "' to '" + _incomingFilePath + "'.");
    file = storage.getFS().open(_incomingFilePath.c_str(), FILE_WRITE);
    if (!file) {
      _downloadingFile = false;
      request->send(500);
      return;
    }
  }
  if (!file.write(data, len)) {
    logError(LS, ET::WEB, "Failed writing '" + _incomingFilePath + "'.");
  }
  if (final) {
    file.close();
    logInfo(LS, ET::WEB, "Download complete. File size = " + String(storage.getFileSize(_incomingFilePath.c_str())));
    _incomingFilePath = "";
    _downloadingFile = false;
    request->send(200);
  }
}


/*---------------  IF THE WEB SERVER RECEIVES A REQUEST FOR A NON-DEFINED
                   ROUTE THE SEE IF STORAGE HAS A FILE THAT MATCHES IT  ---------------*/

bool Web::tryServeStaticFile(AsyncWebServerRequest *request) {
  String path = request->url();
  logInfo(LS, ET::WEB, "Static file request: '" + path + "' from " + request->client()->remoteIP().toString());
  if (!storage.exists(path)) return false;
  request->send(storage.getFS(), path, getContentType(path));
  return true;
}

const char *Web::getContentType(const String &path) const {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css"))  return "text/css";
    if (path.endsWith(".js"))   return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".txt"))  return "text/plain";
    if (path.endsWith(".png"))  return "image/png";
    if (path.endsWith(".jpg"))  return "image/jpeg";
    if (path.endsWith(".gif"))  return "image/gif";
    if (path.endsWith(".ico"))  return "image/x-icon";

    return "application/octet-stream";
}

/*-----  PROCESS AN INCOMING MSG  -----*/

void Web::processMsg(const JsonDocument& doc) {
  
}
