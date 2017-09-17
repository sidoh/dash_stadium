#include <Arduino.h>
#include <StringStream.h>
#include <ArduinoJson.h>

#ifndef _SETTINGS_H_INCLUDED
#define _SETTINGS_H_INCLUDED

#define XQUOTE(x) #x
#define QUOTE(x) XQUOTE(x)

#ifndef FIRMWARE_VARIANT
#define FIRMWARE_VARIANT unknown
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION unknown
#endif

#define SETTINGS_FILE  "/config.json"
#define SETTINGS_TERMINATOR '\0'

#define DEFAULT_MQTT_PORT 1883

class Settings {
public:
  Settings() :
    adminUsername(""),
    adminPassword(""),
    apName("DashStadium"),
    apPassword("qu3c2ER9Ddl"),
    numMonitoredMacs(0),
    monitoredMacs(NULL)
  { }

  ~Settings() {
  }

  static void deserialize(Settings& settings, String json);
  static void load(Settings& settings);

  void save();
  String toJson(const bool prettyPrint = true);
  void serialize(Stream& stream, const bool prettyPrint = false);
  void patch(JsonObject& obj);

  String mqttServer();
  uint16_t mqttPort();
  void setupSoftAP();
  bool hasAuthSettings();

  String adminUsername;
  String adminPassword;
  String _mqttServer;
  String mqttUsername;
  String mqttPassword;
  String mqttTopicPattern;
  String apName;
  String apPassword;

  uint8_t* monitoredMacs;
  String* deviceAliases;
  size_t numMonitoredMacs;
  uint32_t debounceThresholdMs;

  int findMonitoredMac(const uint8_t* mac);

  static void parseMac(const char* s, uint8_t* buffer);
  static void formatMac(const uint8_t* mac, char* buffer);

protected:

  template <typename T>
  void setIfPresent(JsonObject& obj, const char* key, T& var) {
    if (obj.containsKey(key)) {
      var = obj.get<T>(key);
    }
  }
};

#endif
