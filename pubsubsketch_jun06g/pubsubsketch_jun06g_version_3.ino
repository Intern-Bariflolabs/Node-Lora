#include <WiFi.h>           // For ESP32
//#include <ESP8266WiFi.h>  // Uncomment this for ESP8266
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

// Replace with your network credentials
const char* ssid = "BarifloLabs";
const char* password = "Bfl_wifi0@1234";

// MQTT Broker details
const char* mqtt_server = "4.240.114.7";
const int mqtt_port = 1883;
const char* mqtt_user = "BarifloLabs";
const char* mqtt_password = "Bfl@123";
const char* client_id = "ESP32Client";

const long utcOffsetInSeconds = 19800;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds); // Update interval set to 60 seconds

// Initialize LED Pin
const int ledPin = 2; // GPIO pin for the LED
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
long lastMsg = 0;
int value = 0;

// Define multiple topics and corresponding device IDs
const char* topics_pub[] = { "652935996857548/data", "991295414093719/data", "560698684631269/data","853784473545986/data","548606136678812/data"};
const char* device_ids[] = { "652935996857548", "991295414093719", "560698684631269","853784473545986","548606136678812"};
const int num_topics = sizeof(topics_pub) / sizeof(topics_pub[0]);
const char* topics_sub[] = { "652935996857548", "991295414093719", "560698684631269","853784473545986","548606136678812"};
const int data_pub[] = {20, 21, 22, 23, 24}; // Corrected to array
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Blink the LED when a message is received
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload, length);
  
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract values
  long long displayId = doc["display_id"];
  bool status = doc["status"];
  
  Serial.print("display_id: ");
  Serial.println(displayId);
  Serial.print("status: ");
  Serial.println(status ? "True" : "False");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      for (int i = 0; i < num_topics; i++) {
        client.publish(topics_pub[i], "{\"status\":\"connected\"}");
      }
      // ... and resubscribe
      for (int i = 0; i < num_topics; i++) {
        client.subscribe(topics_sub[i]);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String getFormattedTime() {
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime((time_t*)&epochTime);
  int year = ptm->tm_year + 1900;
  int month = ptm->tm_mon + 1;
  int day = ptm->tm_mday;
  int hour = ptm->tm_hour;
  int minute = ptm->tm_min;
  int second = ptm->tm_sec;

  char formattedTime[20];
  sprintf(formattedTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
  return String(formattedTime);
}

void setup() {
  pinMode(ledPin, OUTPUT); // Initialize the LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  timeClient.begin(); // Initialize NTP client
  timeClient.update();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Update the time from NTP server
  timeClient.update();

  long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;

    // Get the current timestamp from NTP and format it
    String formattedTime = getFormattedTime();

    // Publish JSON message to multiple topics
    for (int i = 0; i < num_topics; i++) {
      // Create JSON object
      StaticJsonDocument<200> doc;
      doc["dataPoint"] = formattedTime;
      doc["paramType"] = "cpu_temp";
      doc["paramValue"] = data_pub[i]; // Using the values from the corrected array
      doc["deviceId"] = device_ids[i];

      char jsonBuffer[512];
      serializeJson(doc, jsonBuffer);

      Serial.print("Publishing message to ");
      Serial.print(topics_pub[i]);
      Serial.print(": ");
      Serial.println(jsonBuffer);
      client.publish(topics_pub[i], jsonBuffer);
    }
  }
}
