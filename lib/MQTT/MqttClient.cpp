#include <MqttClient.h>
#include <TokenIterator.h>
#include <UrlTokenBindings.h>
#include <IntParsing.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>

MqttClient::MqttClient(Settings& settings)
  : settings(settings),
    lastConnectAttempt(0)
{
  String strDomain = settings.mqttServer();
  this->domain = new char[strDomain.length() + 1];
  strcpy(this->domain, strDomain.c_str());

  this->mqttClient = new PubSubClient(tcpClient);
}

MqttClient::~MqttClient() {
  mqttClient->disconnect();
  delete this->domain;
}

void MqttClient::begin() {
#ifdef MQTT_DEBUG
  const char* server = settings._mqttServer.c_str();
  printf(
    "MqttClient - Connecting to: %s\n",
    server
  );
#endif

  mqttClient->setServer(this->domain, settings.mqttPort());
  mqttClient->setCallback(
    [this](char* topic, byte* payload, int length) {
      this->publishCallback(topic, payload, length);
    }
  );
  reconnect();
}

bool MqttClient::connect() {
  char nameBuffer[30];
  sprintf_P(nameBuffer, PSTR("dash-stadium-%u"), ESP.getChipId());

#ifdef MQTT_DEBUG
    Serial.println(F("MqttClient - connecting"));
#endif

  if (settings.mqttUsername.length() > 0) {
    return mqttClient->connect(
      nameBuffer,
      settings.mqttUsername.c_str(),
      settings.mqttPassword.c_str()
    );
  } else {
    return mqttClient->connect(nameBuffer);
  }
}

void MqttClient::reconnect() {
  if (lastConnectAttempt > 0 && (millis() - lastConnectAttempt) < MQTT_CONNECTION_ATTEMPT_FREQUENCY) {
    return;
  }

  if (! mqttClient->connected()) {
    if (connect()) {
      subscribe();

#ifdef MQTT_DEBUG
      Serial.println(F("MqttClient - Successfully connected to MQTT server"));
#endif
    } else {
      Serial.println(F("ERROR: Failed to connect to MQTT server"));
    }
  }

  lastConnectAttempt = millis();
}

void MqttClient::handleClient() {
  reconnect();
  mqttClient->loop();
}

void MqttClient::sendUpdate(const char* eventType, const char* macAddr, const char* deviceAlias) {
  String topic = settings.mqttTopicPattern;

  if (topic.length() == 0) {
    return;
  }

  topic.replace(":event_type", eventType);
  topic.replace(":mac_addr", macAddr);
  topic.replace(":device_alias", deviceAlias);

#ifdef MQTT_DEBUG
  printf("MqttClient - publishing update to %s: %s\n", topic.c_str(), DASH_MQTT_PAYLOAD);
#endif

  mqttClient->publish(topic.c_str(), DASH_MQTT_PAYLOAD);
}

void MqttClient::subscribe() {
  // PubSubClient won't connect unless we subscribe to something...
  mqttClient->subscribe("fake_topic_12349081209348");
}

void MqttClient::publishCallback(char* topic, byte* payload, int length) {
}
