#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>           // For ESP32
//#include <ESP8266WiFi.h>  // Uncomment this for ESP8266
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Replace with your network credentials
const char* ssid = "BarifloLabs";
const char* password = "Bfl_wifi0@1234";
//const char* ssid = "OnePlus";
//const char* password = "Bfl_wifi0@1234";
// MQTT Broker details
const char* mqtt_server = "4.240.114.7";
const int mqtt_port = 1883;
const char* mqtt_user = "BarifloLabs";
const char* mqtt_password = "Bfl@123";
const char* client_id = "RangeLink";
const char* topic_pub = "26773439927218/data";
const char* topic_sub = "26773439927218";
const long utcOffsetInSeconds = 19800;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds); // Update interval set to 60 seconds

#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
long lastMsg = 0;
int value = 0;
String extractedData;        // extracted data from JSON
String extractedData2;        // extracted data from JSON
String incomingMessage;
String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xA34;    // address of this device
byte destination = 0xBC;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends
long lastPublishTime = 0;     // last publish time

// LoRa module pin configuration
#define SS 5    // GPIO5
#define RST 14  // GPIO14
#define DIO0 2  // GPIO2

void setup_wifi() {
  delay(10);
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
///////////////////////////////////////////////////////////////////  calback ///////////////////////////////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // Convert payload to String
  String incoming = "";
  for (unsigned int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }

 /* // Parse JSON data
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, incoming);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract data from JSON
 /* extractedData = doc["status"]; // Assuming the JSON has a field named "data"
  Serial.print("Extracted Data: ");
  Serial.println(String(extractedData));
  //const char* data = doc["status"]; // Assuming the JSON has a field named "data"
  byte data1 = doc["display_id"];
  byte data2 = doc["status"]; 
  extractedData = String(data2);
  extractedData2 = String(data1); 
 
  Serial.print("Extracted Data: ");
  Serial.println(extractedData);
  Serial.print("Extracted Data1: ");
  Serial.println(extractedData2);

  // Save extracted data to global variable for sending via LoRa
  incomingMessage = extractedData;*/

  
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
  //long long displayId = doc["display_id"];
  String displayId = doc["display_id"];
  bool status = doc["status"];
  String status1 = String(status);
  incomingMessage = displayId+"+"+status1;
  Serial.print("display_id: ");
  Serial.println(displayId);
  Serial.print("status: ");
  Serial.println(status ? "True" : "False");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(client_id, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.publish(topic_pub, "{\"status\":\"connected\"}");
      client.subscribe(topic_sub);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
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
///////////////////////////////////////////sendMessage & Subscribe//////////////////////////////////////////
void sendMessage(String outgoing) {
  LoRa.beginPacket();
  LoRa.write(destination);
  LoRa.write(localAddress);
  LoRa.write(msgCount);
  LoRa.write(outgoing.length());
  LoRa.print(outgoing);
  LoRa.endPacket();
  msgCount++;
}
///////////////////////////////////////////OnReceive & Publish//////////////////////////////////////////
void onReceive(int packetSize) {
  if (packetSize == 0) return;
  int recipient = LoRa.read();
  byte sender = LoRa.read();
  byte incomingMsgId = LoRa.read();
  byte incomingLength = LoRa.read();
  String incoming = "";
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
  if (incomingLength != incoming.length()) {
    Serial.println("error: message length does not match length");
    return;
  }
  if (recipient != localAddress && recipient != 0xA34) {
    Serial.println("This message is not for me.");
    return;
  }
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));  
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  delay(1000);
  // Prepare and publish data to MQTT
  String formattedTime = getFormattedTime();
  StaticJsonDocument<200> doc;
  
  doc["dataPoint"] = formattedTime;
  doc["paramType"] = "Data";
  doc["paramValue"] = incoming;
  doc["deviceId"] = "26773439927218";
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  
  Serial.print("Publishing received data message: ");
  Serial.println(jsonBuffer);
  client.publish(topic_pub, jsonBuffer);
  
  doc.clear();
  
  
  doc["dataPoint"] = formattedTime;
  doc["paramType"] = "SNR";
  doc["paramValue"] = String(LoRa.packetSnr());
  doc["deviceId"] = "26773439927218";
  
  serializeJson(doc, jsonBuffer);
  
  Serial.print("Publishing SNR message: ");
  Serial.println(jsonBuffer);
  client.publish(topic_pub, jsonBuffer);
  
  doc.clear();
  
  doc["dataPoint"] = formattedTime;
  doc["paramType"] = "RSSI";
  doc["paramValue"] = String(LoRa.packetRssi());
  doc["deviceId"] = "26773439927218";
  
  serializeJson(doc, jsonBuffer);
  
  Serial.print("Publishing RSSI message: ");
  Serial.println(jsonBuffer);
  client.publish(topic_pub, jsonBuffer);
}
//////////////////////////////////////////////////////// setup ////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  timeClient.begin();
  timeClient.update();
  while (!Serial);
  Serial.println("LoRa Duplex");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
}
//////////////////////////////////////////////////////// main loop ////////////////////////////////////////////////
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  timeClient.update();
  long now = millis();
  
  if (now - lastSendTime > interval) {
    extractedData;   
    sendMessage(incomingMessage);
    Serial.println("Sending " + incomingMessage);
    lastSendTime = now;
    interval = random(2000) + 1000; 
  }
  
  onReceive(LoRa.parsePacket());
}
