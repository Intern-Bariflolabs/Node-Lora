#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>
#define SerialMon Serial
#include <String.h>

TinyGsm modem(Serial2);
TinyGsmClient espClient(modem);
PubSubClient client(espClient);

#define MODEM_TX    17
#define MODEM_RX    16

int tx = 25;
int rx = 26;

// MQTT Broker details
const char* mqtt_server = "mqtt.bc-pl.com";
const int mqtt_port = 1883;
const char* mqtt_user = "Bariflolabs";
const char* mqtt_password = "Bariflo@2024";
const char* client_id = "RangeLink";
const char* topic_pub = "409566418471355/data";
const char* topic_sub = "409566418471355";

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
void parseMessage(String message);
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
  digitalWrite(tx, HIGH);
  LoRa.beginPacket();
  LoRa.write(destination);
  LoRa.write(localAddress);
  LoRa.write(msgCount);
  LoRa.write(outgoing.length());
  LoRa.print(outgoing);
  LoRa.endPacket();
  digitalWrite(tx, LOW);
  msgCount++;
  
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;
  digitalWrite(rx, HIGH);
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
    digitalWrite(rx, LOW);
    return;
  }
  if (recipient != localAddress && recipient != 0xA34) {
    Serial.println("This message is not for me.");
    digitalWrite(rx, LOW);
    return;
  }
  Serial.println("Received from: 0x" + String(sender, HEX));
  String devID = String(sender, HEX);
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));  
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  digitalWrite(rx, LOW);
  delay(1000);
  /////////////////////////////////////////////////////////////////////////////////////////////////////
  if(devID == "3d"){
  int idStart = incoming.indexOf("R1") + 2;
  int idEnd = incoming.indexOf(",", idStart);
  String data = incoming.substring(idStart, idEnd);
  Serial.println(data);
  float r_1 = data.toFloat();

  int idStart1 = incoming.indexOf("R2") + 2;
  int idEnd1 = incoming.indexOf(",", idStart1);
  String data1 = incoming.substring(idStart1, idEnd1);
  Serial.println(data1);
  float r_2 = data1.toFloat();

  int idStart2 = incoming.indexOf("Y1") + 2;
  int idEnd2 = incoming.indexOf(",", idStart2);
  String data2 = incoming.substring(idStart2, idEnd2);
  Serial.println(data2);
  float y_1 = data2.toFloat();

  int idStart3 = incoming.indexOf("Y2") + 2;
  int idEnd3 = incoming.indexOf(",", idStart3);
  String data3 = incoming.substring(idStart3, idEnd3);
  Serial.println(data3);
  float y_2 = data3.toFloat();

  int idStart4 = incoming.indexOf("B1") + 2;
  int idEnd4 = incoming.indexOf(",", idStart4);
  String data4 = incoming.substring(idStart4, idEnd4);
  Serial.println(data4);
  float b_1 = data4.toFloat();

  int idStart5 = incoming.indexOf("B2") + 2;
  int idEnd5 = incoming.indexOf(",", idStart5);
  String data5 = incoming.substring(idStart5, idEnd5);
  Serial.println(data5);
  float b_2 = data5.toFloat();
 
  // Fetch network time and publish messages
  if (modem.getNetworkTime(&year, &month, &day, &hour, &minute, &second, &timezone)) {
    char formattedTime[20];
    sprintf(formattedTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    
    StaticJsonDocument<200> doc;
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "R-Ph";
    doc["paramValue"] = r_1;
    doc["deviceId"] = "409566418471355";
    
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);
    
    doc.clear();
    
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "R-Ph";
    doc["paramValue"] = r_2;
    doc["deviceId"] = "538103802554933";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();

    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "Y-Ph";
    doc["paramValue"] = y_1;
    doc["deviceId"] = "409566418471355";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();

    
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "Y-Ph";
    doc["paramValue"] = y_2;
    doc["deviceId"] = "538103802554933";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();

    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "B-Ph";
    doc["paramValue"] = b_1;
    doc["deviceId"] = "409566418471355";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();

    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "B-Ph";
    doc["paramValue"] = b_2;
    doc["deviceId"] = "538103802554933";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();
    
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "SNR";
    doc["paramValue"] = String(LoRa.packetSnr());
    doc["deviceId"] = "409566418471355";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing SNR message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();
    
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "RSSI";
    doc["paramValue"] = String(LoRa.packetRssi());
    doc["deviceId"] = "409566418471355";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing RSSI message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);
  }
  }
//////////////////////////////////////////////////////////////////////////////////////////////////
  else if(devID == "b4"){
  int idStart0 = incoming.indexOf("R1") + 2;
  int idEnd0 = incoming.indexOf(",", idStart0);
  String data0 = incoming.substring(idStart0, idEnd0);
  Serial.println(data0);
  float r_11 = data0.toFloat();

  int idStart11 = incoming.indexOf("R2") + 2;
  int idEnd11 = incoming.indexOf(",", idStart11);
  String data11 = incoming.substring(idStart11, idEnd11);
  Serial.println(data11);
  float r_22 = data11.toFloat();

  int idStart22 = incoming.indexOf("Y1") + 2;
  int idEnd22 = incoming.indexOf(",", idStart22);
  String data22 = incoming.substring(idStart22, idEnd22);
  Serial.println(data22);
  float y_11 = data22.toFloat();

  int idStart33 = incoming.indexOf("Y2") + 2;
  int idEnd33 = incoming.indexOf(",", idStart33);
  String data33 = incoming.substring(idStart33, idEnd33);
  Serial.println(data33);
  float y_22 = data33.toFloat();

  int idStart44 = incoming.indexOf("B1") + 2;
  int idEnd44 = incoming.indexOf(",", idStart44);
  String data44 = incoming.substring(idStart44, idEnd44);
  Serial.println(data44);
  float b_11 = data44.toFloat();

  int idStart55 = incoming.indexOf("B2") + 2;
  int idEnd55 = incoming.indexOf(",", idStart55);
  String data55 = incoming.substring(idStart55, idEnd55);
  Serial.println(data55);
  float b_22 = data55.toFloat();
 
  // Fetch network time and publish messages
  if (modem.getNetworkTime(&year, &month, &day, &hour, &minute, &second, &timezone)) {
    char formattedTime[20];
    sprintf(formattedTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    
    StaticJsonDocument<200> doc;
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "R-Ph";
    doc["paramValue"] = r_11;
    doc["deviceId"] = "322350327358606";
    
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);
    
    doc.clear();
    
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "R-Ph";
    doc["paramValue"] = r_22;
    doc["deviceId"] = "787433500510266";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();

    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "Y-Ph";
    doc["paramValue"] = y_11;
    doc["deviceId"] = "322350327358606";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();

    
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "Y-Ph";
    doc["paramValue"] = y_22;
    doc["deviceId"] = "787433500510266";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();

    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "B-Ph";
    doc["paramValue"] = b_11;
    doc["deviceId"] = "322350327358606";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();

    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "B-Ph";
    doc["paramValue"] = b_22;
    doc["deviceId"] = "787433500510266";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing received data message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();
    
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "SNR";
    doc["paramValue"] = String(LoRa.packetSnr());
    doc["deviceId"] = "322350327358606";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing SNR message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);

    doc.clear();
    
    doc["dataPoint"] = formattedTime;
    doc["paramType"] = "RSSI";
    doc["paramValue"] = String(LoRa.packetRssi());
    doc["deviceId"] = "322350327358606";
    
    serializeJson(doc, jsonBuffer);
    
    Serial.print("Publishing RSSI message: ");
    Serial.println(jsonBuffer);
    client.publish(topic_pub, jsonBuffer);
  }
  }
 /////////////////////////////////////////////////////////////////////////////////////////////////
 else {
    Serial.println("Unknown device ID");
  }
}

void setup() {
  pinMode(tx, OUTPUT);
  pinMode(rx, OUTPUT);
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
    interval = random(2000) + 100; 
  }
  
  onReceive(LoRa.parsePacket());
  delay(10);
}
