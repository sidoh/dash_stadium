#include <WebServer.h>
#include <Settings.h>
#include <WebSocketsServer.h>

#ifndef _MILIGHT_HTTP_SERVER
#define _MILIGHT_HTTP_SERVER

#define MAX_DOWNLOAD_ATTEMPTS 3

typedef std::function<void(void)> SettingsSavedHandler;

const char TEXT_PLAIN[] PROGMEM = "text/plain";
const char APPLICATION_JSON[] = "application/json";

class DashStadiumHttpServer {
public:
  DashStadiumHttpServer(Settings& settings)
    : server(WebServer(80)),
      wsServer(WebSocketsServer(81)),
      settings(settings),
      settingsSavedHandler(NULL)
  { }

  void begin();
  void handleClient();
  void on(const char* path, HTTPMethod method, ESP8266WebServer::THandlerFunction handler);
  void onSettingsSaved(SettingsSavedHandler handler);
  void handleWifiEvent(const char* eventType, const uint8_t* macAddr);

protected:
  ESP8266WebServer::THandlerFunction handleServeFile(
    const char* filename,
    const char* contentType,
    const char* defaultText = NULL);

  bool serveFile(const char* file, const char* contentType = "text/html");
  ESP8266WebServer::THandlerFunction handleUpdateFile(const char* filename);
  ESP8266WebServer::THandlerFunction handleServe_P(const char* data, size_t length);
  void applySettings(Settings& settings);

  void handleAbout();
  void handleUpdateSettings();
  void handleFirmwareUpload();
  void handleFirmwareIncrement();

  void handleWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

  WebServer server;
  WebSocketsServer wsServer;
  Settings& settings;
  SettingsSavedHandler settingsSavedHandler;
  File updateFile;
  size_t numWsClients;

};

#endif
