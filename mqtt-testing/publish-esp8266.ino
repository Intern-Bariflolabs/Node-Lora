#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "BarifloLabs";
const char* password = "Bfl_wifi0@1234";

// MQTT Broker details
const char* mqtt_server = "mqtt.bc-pl.com";
const int mqtt_port = 1883;
const char* mqtt_user = "Bariflolabs";
const char* mqtt_password = "Bariflo@2024";

// MQTT topic to publish to
const char* mqtt_topic = "test/topic";

// Device ID and virtual pin
const char* deviceID = "111";
const char* virtualpin = "0";

// Initialize the WiFi and MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  // Start connecting to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Wait for connection
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an initial message
      StaticJsonDocument<200> doc;
      doc[deviceID] = true;
      doc["virtualpin"] = virtualpin;
      char buffer[256];
      serializeJson(doc, buffer);
      client.publish(mqtt_topic, buffer);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  if (WiFi.status() == WL_CONNECTED) {
    client.setServer(mqtt_server, mqtt_port);
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    // Publish a message every 10 seconds
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > 10000) {
      lastPublish = millis();
      StaticJsonDocument<200> doc;
      doc[deviceID] = true;
      doc["virtualpin"] = virtualpin;
      char buffer[256];
      serializeJson(doc, buffer);
      client.publish(mqtt_topic, buffer);
    }
  } else {
    Serial.println("WiFi connection lost, attempting to reconnect...");
    setup_wifi();
  }
}
