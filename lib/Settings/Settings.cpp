#include <Settings.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <algorithm>
#include <ESP8266WiFi.h>
#include <IntParsing.h>

#define PORT_POSITION(s) ( s.indexOf(':') )

bool Settings::hasAuthSettings() {
  return adminUsername.length() > 0 && adminPassword.length() > 0;
}

void Settings::deserialize(Settings& settings, String json) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& parsedSettings = jsonBuffer.parseObject(json);
  settings.patch(parsedSettings);
}

void Settings::patch(JsonObject& parsedSettings) {
  if (parsedSettings.success()) {
    this->setIfPresent(parsedSettings, "admin_username", adminUsername);
    this->setIfPresent(parsedSettings, "admin_password", adminPassword);
    this->setIfPresent(parsedSettings, "mqtt_server", _mqttServer);
    this->setIfPresent(parsedSettings, "mqtt_username", mqttUsername);
    this->setIfPresent(parsedSettings, "mqtt_password", mqttPassword);
    this->setIfPresent(parsedSettings, "mqtt_topic_pattern", mqttTopicPattern);
    this->setIfPresent(parsedSettings, "ap_name", apName);
    this->setIfPresent(parsedSettings, "ap_password", apPassword);
    this->setIfPresent(parsedSettings, "debounce_threshold_ms", debounceThresholdMs);

    if (parsedSettings.containsKey("monitored_macs")) {
      JsonArray& macs = parsedSettings["monitored_macs"];

      if (this->monitoredMacs != NULL) {
        delete this->monitoredMacs;
        delete this->deviceAliases;
      }

      this->numMonitoredMacs = macs.size();
      this->monitoredMacs = new uint8_t[this->numMonitoredMacs * 6];
      this->deviceAliases = new String[this->numMonitoredMacs];

      for (size_t i = 0; i < this->numMonitoredMacs; i++) {
        JsonArray& config = macs[i];

        if (!config.success()) {
          continue;
        }

        const char* s = config[0];
        parseMac(s, this->monitoredMacs+(i*6));
        this->deviceAliases[i] = config.get<String>(1);
      }
    }
  }
}

void Settings::load(Settings& settings) {
  if (SPIFFS.exists(SETTINGS_FILE)) {
    File f = SPIFFS.open(SETTINGS_FILE, "r");
    String settingsContents = f.readStringUntil(SETTINGS_TERMINATOR);
    f.close();

    deserialize(settings, settingsContents);
  } else {
    settings.save();
  }
}

String Settings::toJson(const bool prettyPrint) {
  String buffer = "";
  StringStream s(buffer);
  serialize(s, prettyPrint);
  return buffer;
}

void Settings::save() {
  File f = SPIFFS.open(SETTINGS_FILE, "w");

  if (!f) {
    Serial.println(F("Opening settings file failed"));
  } else {
    serialize(f);
    f.close();
  }
}

void Settings::serialize(Stream& stream, const bool prettyPrint) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["admin_username"] = this->adminUsername;
  root["admin_password"] = this->adminPassword;
  root["mqtt_server"] = this->_mqttServer;
  root["mqtt_username"] = this->mqttUsername;
  root["mqtt_password"] = this->mqttPassword;
  root["mqtt_topic_pattern"] = this->mqttTopicPattern;
  root["ap_name"] = this->apName;
  root["ap_password"] = this->apPassword;
  root["debounce_threshold_ms"] = this->debounceThresholdMs;

  if (this->monitoredMacs) {
    JsonArray& macs = jsonBuffer.createArray();
    char macBuffer[25];
    memset(macBuffer, 0, 25);

    for (size_t i = 0; i < this->numMonitoredMacs; i++) {
      JsonArray& config = jsonBuffer.createArray();
      formatMac(this->monitoredMacs+(6*i), macBuffer);
      config.add(String(macBuffer));
      config.add(String(this->deviceAliases[i]));

      macs.add(config);
    }
    root["monitored_macs"] = macs;
  }

  if (prettyPrint) {
    root.prettyPrintTo(stream);
  } else {
    root.printTo(stream);
  }
}

String Settings::mqttServer() {
  int pos = PORT_POSITION(_mqttServer);

  if (pos == -1) {
    return _mqttServer;
  } else {
    return _mqttServer.substring(0, pos);
  }
}

uint16_t Settings::mqttPort() {
  int pos = PORT_POSITION(_mqttServer);

  if (pos == -1) {
    return DEFAULT_MQTT_PORT;
  } else {
    return atoi(_mqttServer.c_str() + pos + 1);
  }
}

void Settings::setupSoftAP() {
  if (apPassword.length() > 0) {
    WiFi.softAP(apName.c_str(), apPassword.c_str());
  } else {
    WiFi.softAP(apName.c_str());
  }
}

int Settings::findMonitoredMac(const uint8_t *mac) {
  if (this->monitoredMacs == NULL) {
    return -1;
  }

  for (size_t i = 0; i < this->numMonitoredMacs; i++) {
    bool found = true;
    uint8_t* monMac = this->monitoredMacs+(i*6);

    for (size_t j = 0; j < 6; j++) {
      if (mac[j] != monMac[j]) {
        found = false;
        break;
      }
    }

    if (found) {
      return i;
    }
  }

  return -1;
}

void Settings::formatMac(const uint8_t* mac, char* buffer) {
  IntParsing::bytesToHexStr(mac, 6, buffer, 25, ':');
}

void Settings::parseMac(const char *s, uint8_t *buffer) {
  IntParsing::parseDelimitedBytes(s, buffer, 6, ':');
}
