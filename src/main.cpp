#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <Settings.h>
#include <FS.h>

#include <TokenIterator.h>
#include <MqttClient.h>
#include <DashStadiumHttpServer.h>

WiFiEventHandler probeHandler;
WiFiEventHandler connectedHandler;

Settings settings;
MqttClient* mqttClient = NULL;
unsigned long* lastSeenTimes = NULL;
DashStadiumHttpServer webServer(settings);

String macToString(const unsigned char* mac) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void triggerEvent(const char* eventName, const uint8_t* mac) {
  int macIx = settings.findMonitoredMac(mac);

  if (macIx != -1) {
    String formattedMac = macToString(mac);
    printf("Event triggered - %s for %s\n", eventName, formattedMac.c_str());

    unsigned long timestamp = millis();

    if ((lastSeenTimes[macIx] + settings.debounceThresholdMs) < timestamp) {
      mqttClient->sendUpdate(eventName, formattedMac.c_str());
    } else {
      Serial.println(F("Debouncing update, not sending repeat"));
    }

    lastSeenTimes[macIx] = timestamp;
  }
}

void onProbeRequestPrint(const WiFiEventSoftAPModeProbeRequestReceived& evt) {
  triggerEvent("probe_request", evt.mac);
}

void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  triggerEvent("connected", evt.mac);
}

void applySettings() {
  if (settings.mqttServer().length() > 0) {
    if (mqttClient) {
      delete mqttClient;
    }

    mqttClient = new MqttClient(settings);
    mqttClient->begin();
  }

  if (lastSeenTimes) {
    delete lastSeenTimes;
  }
  lastSeenTimes = new unsigned long[settings.numMonitoredMacs];
  memset(lastSeenTimes, 0, sizeof(unsigned long)*settings.numMonitoredMacs);
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  Settings::load(settings);

  WiFiManager wifiManager;
  wifiManager.autoConnect();

  WiFi.mode(WIFI_AP_STA);
  settings.setupSoftAP();

  probeHandler = WiFi.onSoftAPModeProbeRequestReceived(onProbeRequestPrint);
  connectedHandler = WiFi.onSoftAPModeStationConnected(onStationConnected);

  webServer.onSettingsSaved(applySettings);
  webServer.begin();
  applySettings();
}

void loop(){
  if (mqttClient) {
    mqttClient->handleClient();
  }
  webServer.handleClient();
}
