#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <Settings.h>
#include <FS.h>
#include <ESP8266mDNS.h>

#include <Size.h>
#include <IntParsing.h>
#include <TokenIterator.h>
#include <MqttClient.h>
#include <DashStadiumHttpServer.h>

extern "C" {
#include <user_interface.h>
}

WiFiEventHandler probeHandler;
WiFiEventHandler connectedHandler;

enum DashEventType {
  DASH_EVENT_PROBE_REQUEST = 0,
  DASH_EVENT_CONNECTED = 1
};
static const char* EVENT_NAMES[] = {"probe_request", "connected"};

Settings settings;
MqttClient* mqttClient = NULL;
unsigned long* lastSeenTimes[2] = {NULL, NULL};
DashStadiumHttpServer webServer(settings);

void triggerEvent(const DashEventType& evtType, const uint8_t* mac) {
  int macIx = settings.findMonitoredMac(mac);

  webServer.handleWifiEvent(EVENT_NAMES[evtType], mac);

  if (macIx != -1) {
    unsigned long timestamp = millis();

    if ((lastSeenTimes[evtType][macIx] + settings.debounceThresholdMs) < timestamp) {
      char formattedMac[25];
      Settings::formatMac(mac, formattedMac);
      mqttClient->sendUpdate(EVENT_NAMES[evtType], formattedMac, settings.deviceAliases[macIx].c_str());
    }

    lastSeenTimes[evtType][macIx] = timestamp;
  }
}

void onProbeRequestPrint(const WiFiEventSoftAPModeProbeRequestReceived& evt) {
  triggerEvent(DASH_EVENT_PROBE_REQUEST, evt.mac);
}

void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  triggerEvent(DASH_EVENT_CONNECTED, evt.mac);
}

void applySettings() {
  if (settings.mqttServer().length() > 0) {
    if (mqttClient) {
      delete mqttClient;
    }

    mqttClient = new MqttClient(settings);
    mqttClient->begin();
  }

  if (*lastSeenTimes) {
    for (size_t i = 0; i < size(EVENT_NAMES); i++) {
      delete lastSeenTimes[i];
    }
  }

  for (size_t i = 0; i < size(EVENT_NAMES); i++) {
    lastSeenTimes[i] = new unsigned long[settings.numMonitoredMacs];
    memset(lastSeenTimes[i], 0, sizeof(unsigned long)*settings.numMonitoredMacs);
  }

  settings.setupSoftAP();
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

  if (! MDNS.begin("dash-stadium")) {
    Serial.println(F("Error setting up MDNS responder"));
  }
  MDNS.addService("http", "tcp", 80);

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
