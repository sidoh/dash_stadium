#include <FS.h>
#include <WiFiUDP.h>
#include <IntParsing.h>
#include <Settings.h>
#include <DashStadiumHttpServer.h>
#include <TokenIterator.h>

void DashStadiumHttpServer::begin() {
  applySettings(settings);

  server.on("/firmware", HTTP_POST,
    [this](){ handleFirmwareUpload(); },
    [this](){ handleFirmwareIncrement(); }
  );
  server.on("/about", [this]() { handleAbout(); });
  server.on("/settings", HTTP_GET, [this]() { serveFile(SETTINGS_FILE); });
  server.on("/settings", HTTP_PUT, [this]() { handleUpdateSettings(); });

  server.begin();
}

void DashStadiumHttpServer::handleFirmwareIncrement() {
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    WiFiUDP::stopAll();
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if(!Update.begin(maxSketchSpace)){//start with max available size
      Update.printError(Serial);
    }
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
      Update.printError(Serial);
    }
  } else if(upload.status == UPLOAD_FILE_END){
    if(Update.end(true)){ //true to set the size to the current progress
    } else {
      Update.printError(Serial);
    }
  }
  yield();
}

void DashStadiumHttpServer::handleFirmwareUpload() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");

  if (Update.hasError()) {
    server.send_P(
      500,
      TEXT_PLAIN,
      PSTR("Failed updating firmware. Check serial logs for more information. You may need to re-flash the device.")
    );
  } else {
    server.send_P(
      200,
      TEXT_PLAIN,
      PSTR("Success. Device will now reboot.")
    );
  }

  ESP.restart();
}

void DashStadiumHttpServer::handleClient() {
  server.handleClient();
}

void DashStadiumHttpServer::on(const char* path, HTTPMethod method, ESP8266WebServer::THandlerFunction handler) {
  server.on(path, method, handler);
}

void DashStadiumHttpServer::applySettings(Settings& settings) {
  if (settings.hasAuthSettings()) {
    server.requireAuthentication(settings.adminUsername, settings.adminPassword);
  } else {
    server.disableAuthentication();
  }
}

void DashStadiumHttpServer::onSettingsSaved(SettingsSavedHandler handler) {
  this->settingsSavedHandler = handler;
}

void DashStadiumHttpServer::handleAbout() {
  DynamicJsonBuffer buffer;
  JsonObject& response = buffer.createObject();

  response["version"] = QUOTE(FIRMWARE_VERSION);
  response["variant"] = QUOTE(FIRMWARE_VARIANT);
  response["free_heap"] = ESP.getFreeHeap();
  response["arduino_version"] = ESP.getCoreVersion();
  response["reset_reason"] = ESP.getResetReason();

  String body;
  response.printTo(body);

  server.send(200, APPLICATION_JSON, body);
}

ESP8266WebServer::THandlerFunction DashStadiumHttpServer::handleServeFile(
  const char* filename,
  const char* contentType,
  const char* defaultText) {

  return [this, filename, contentType, defaultText]() {
    if (!serveFile(filename)) {
      if (defaultText) {
        server.send(200, contentType, defaultText);
      } else {
        server.send(404);
      }
    }
  };
}

bool DashStadiumHttpServer::serveFile(const char* file, const char* contentType) {
  if (SPIFFS.exists(file)) {
    File f = SPIFFS.open(file, "r");
    server.streamFile(f, contentType);
    f.close();
    return true;
  }

  return false;
}

ESP8266WebServer::THandlerFunction DashStadiumHttpServer::handleUpdateFile(const char* filename) {
  return [this, filename]() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
      updateFile = SPIFFS.open(filename, "w");
    } else if(upload.status == UPLOAD_FILE_WRITE){
      if (updateFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Serial.println(F("Error updating web file"));
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      updateFile.close();
    }
  };
}

void DashStadiumHttpServer::handleUpdateSettings() {
  DynamicJsonBuffer buffer;
  const String& rawSettings = server.arg("plain");
  JsonObject& parsedSettings = buffer.parse(rawSettings);

  if (parsedSettings.success()) {
    settings.patch(parsedSettings);
    settings.save();

    this->applySettings(settings);

    if (this->settingsSavedHandler) {
      this->settingsSavedHandler();
    }

    server.send(200, APPLICATION_JSON, "true");
  } else {
    server.send(400, APPLICATION_JSON, "\"Invalid JSON\"");
  }
}

ESP8266WebServer::THandlerFunction DashStadiumHttpServer::handleServe_P(const char* data, size_t length) {
  return [this, data, length]() {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Content-Length", String(length));
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.setContentLength(length);
    server.sendContent_P(data, length);
    server.client().stop();
  };
}
