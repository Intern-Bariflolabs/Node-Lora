#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>
#define SerialMon Serial

TinyGsm modem(Serial2);
TinyGsmClient espClient(modem);
PubSubClient client(espClient);

#define MODEM_TX    17
#define MODEM_RX    16
#define MODEM_RST   13
// MQTT Broker details
const char* mqtt_server = "4.240.114.7";
const int mqtt_port = 1883;
const char* mqtt_user = "BarifloLabs";
const char* mqtt_password = "Bfl@123";
const char* client_id = "RangeLink";
const char* topic_pub = "26773439927218/data";
const char* topic_sub = "26773439927218";
unsigned long lastResetTime = 0;
const unsigned long resetInterval = 3600000;
int year,month, day, hour, minute, second;
float timezone;

#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];
String incomingMessage;
byte msgCount = 0;
byte localAddress = 0xA34;
byte destination = 0xBC;
long lastSendTime = 0;
int interval = 2000;

// LoRa module pin configuration
#define SS 5
#define RST 14
#define DIO0 2

const char apn[] = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

void setup_gsm() {
  SerialMon.begin(115200);
  delay(10);

  Serial2.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(6000);

  SerialMon.println("Initializing modem...");
  if (!modem.restart()) {
    SerialMon.println("Failed to restart modem, halting");
    while (true);
  }

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);

  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
    SerialMon.print("Local IP: ");
    SerialMon.println(modem.localIP());
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);

    
  }
  Serial.println();

  String incoming = "";
  for (unsigned int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  String displayId = doc["display_id"];
  bool status = doc["status"];
  String status1 = String(status);
  incomingMessage = displayId + "+" + status1;
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

  // Fetch network time and publish messages
  if (modem.getNetworkTime(&year, &month, &day, &hour, &minute, &second, &timezone)) {
    char formattedTime[20];
    sprintf(formattedTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

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
 
}
void resetGSM() {
  SerialMon.println("Resetting GSM modem...");
  digitalWrite(MODEM_RST, LOW);
  delay(1000); // Reset duration
  digitalWrite(MODEM_RST, HIGH);
  SerialMon.println("GSM modem reset completed");
}
void setup() {
  Serial.begin(115200);
  delay(10);

  // pinMode(MODEM_PWRKEY, OUTPUT);
  // pinMode(MODEM_POWER_ON, OUTPUT);
  // digitalWrite(MODEM_PWRKEY, LOW);
  // digitalWrite(MODEM_POWER_ON, HIGH);

  setup_gsm();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  while (!Serial);
  Serial.println("LoRa Duplex");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  
  if (now - lastSendTime > interval) {
    sendMessage(incomingMessage);
    Serial.println("Sending " + incomingMessage);
    lastSendTime = now;
    interval = random(2000) + 1000; 
  }
  
  onReceive(LoRa.parsePacket());
  
  if (now - lastResetTime >= resetInterval) {
    resetGSM();
    lastResetTime = now;
  } 
}
