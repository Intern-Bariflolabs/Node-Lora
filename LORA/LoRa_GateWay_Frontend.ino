#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Default network credentials
char ssid[32] = "BarifloLabs";
char password[32] = "Bfl_wifi0@1234";
// MQTT Broker details
char mqtt_server[32] = "4.240.114.7";
int mqtt_port = 1883;
char mqtt_user[32] = "BarifloLabs";
char mqtt_password[32] = "Bfl@123";
char client_id[32] = "RangeLink";

#define MAX_DEVICES 10
char device_id[MAX_DEVICES][32];
char topic_pub[MAX_DEVICES][32];
char topic_sub[MAX_DEVICES][32];
int num_devices = 1; // Default to one device
const long utcOffsetInSeconds = 19800;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds); // Update interval set to 60 seconds
WebServer server(80); // Create a web server on port 80

#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
long lastMsg = 0;
int value = 0;
String extractedData;        // extracted data from JSON
String extractedData2;       // extracted data from JSON
String incomingMessage;
String outgoing;             // outgoing message
byte msgCount = 0;           // count of outgoing messages
byte localAddress = 0xA34;   // address of this device
byte destination = 0xBC;     // destination to send to
long lastSendTime = 0;       // last send time
int interval = 2000;         // interval between sends
long lastPublishTime = 0;    // last publish time

// LoRa module pin configuration
#define SS 5    // GPIO5
#define RST 14  // GPIO14
#define DIO0 2  // GPIO2

void handleRoot() {
  String html = "<html>\
  <body>\
    <h1>Configuration</h1>\
    <form action=\"/configure\" method=\"POST\">\
      SSID: <input type=\"text\" name=\"ssid\" value=\"" + String(ssid) + "\"><br>\
      Password: <input type=\"text\" name=\"password\" value=\"" + String(password) + "\"><br>\
      MQTT Server: <input type=\"text\" name=\"mqtt_server\" value=\"" + String(mqtt_server) + "\"><br>\
      MQTT Port: <input type=\"number\" name=\"mqtt_port\" value=\"" + String(mqtt_port) + "\"><br>\
      MQTT User: <input type=\"text\" name=\"mqtt_user\" value=\"" + String(mqtt_user) + "\"><br>\
      MQTT Password: <input type=\"text\" name=\"mqtt_password\" value=\"" + String(mqtt_password) + "\"><br>\
      Client ID: <input type=\"text\" name=\"client_id\" value=\"" + String(client_id) + "\"><br>\
      Number of Devices: <input type=\"number\" name=\"num_devices\" value=\"" + String(num_devices) + "\" min=\"1\" max=\"" + String(MAX_DEVICES) + "\"><br>\
      <div id=\"devices\">";
      
  for (int i = 0; i < num_devices; i++) {
    html += "Device " + String(i + 1) + " ID: <input type=\"text\" name=\"device_id" + String(i) + "\" value=\"" + String(device_id[i]) + "\"><br>\
             Device " + String(i + 1) + " Topic Pub: <input type=\"text\" name=\"topic_pub" + String(i) + "\" value=\"" + String(topic_pub[i]) + "\"><br>\
             Device " + String(i + 1) + " Topic Sub: <input type=\"text\" name=\"topic_sub" + String(i) + "\" value=\"" + String(topic_sub[i]) + "\"><br>";
  }

  html += "</div>\
      <input type=\"submit\" value=\"Save\">\
    </form>\
    <script>\
      document.querySelector('input[name=\"num_devices\"]').addEventListener('input', function(e) {\
        let numDevices = e.target.value;\
        let devicesDiv = document.getElementById('devices');\
        devicesDiv.innerHTML = '';\
        for (let i = 0; i < numDevices; i++) {\
          devicesDiv.innerHTML += 'Device ' + (i + 1) + ' ID: <input type=\"text\" name=\"device_id' + i + '\" value=\"' + (i < " + String(num_devices) + " ? '" + String(device_id[i]) + "' : '') + '\"><br>\
                                 Device ' + (i + 1) + ' Topic Pub: <input type=\"text\" name=\"topic_pub' + i + '\" value=\"' + (i < " + String(num_devices) + " ? '" + String(topic_pub[i]) + "' : '') + '\"><br>\
                                 Device ' + (i + 1) + ' Topic Sub: <input type=\"text\" name=\"topic_sub' + i + '\" value=\"' + (i < " + String(num_devices) + " ? '" + String(topic_sub[i]) + "' : '') + '\"><br>';\
        }\
      });\
    </script>\
  </body>\
  </html>";
  server.send(200, "text/html", html);
}

void handleConfigure() {
  if (server.method() == HTTP_POST) {
    strncpy(ssid, server.arg("ssid").c_str(), sizeof(ssid));
    strncpy(password, server.arg("password").c_str(), sizeof(password));
    strncpy(mqtt_server, server.arg("mqtt_server").c_str(), sizeof(mqtt_server));
    mqtt_port = server.arg("mqtt_port").toInt();
    strncpy(mqtt_user, server.arg("mqtt_user").c_str(), sizeof(mqtt_user));
    strncpy(mqtt_password, server.arg("mqtt_password").c_str(), sizeof(mqtt_password));
    strncpy(client_id, server.arg("client_id").c_str(), sizeof(client_id));
    num_devices = server.arg("num_devices").toInt();
    
    for (int i = 0; i < num_devices; i++) {
      String idArg = "device_id" + String(i);
      String pubArg = "topic_pub" + String(i);
      String subArg = "topic_sub" + String(i);
      strncpy(device_id[i], server.arg(idArg).c_str(), sizeof(device_id[i]));
      strncpy(topic_pub[i], server.arg(pubArg).c_str(), sizeof(topic_pub[i]));
      strncpy(topic_sub[i], server.arg(subArg).c_str(), sizeof(topic_sub[i]));
    }

    String response = "Configuration Saved. Rebooting...";
    server.send(200, "text/html", response);
    delay(2000);
    ESP.restart();
  } else {
    server.send(405, "text/html", "Method Not Allowed");
  }
}

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
  
  // Find the device index by matching device_id with topic_sub
  int deviceIndex = -1;
  for (int i = 0; i < num_devices; i++) {
    if (strcmp(topic_sub[i], topic) == 0) {
      deviceIndex = i;
      break;
    }
  }

  if (deviceIndex != -1) {
    // Publish to the corresponding topic_pub
    client.publish(topic_pub[deviceIndex], incoming.c_str());
    Serial.println("Published to topic: " + String(topic_pub[deviceIndex]));
  } else {
    Serial.println("No matching device found for topic: " + String(topic));
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(client_id, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      for (int i = 0; i < num_devices; i++) {
        client.subscribe(topic_sub[i]);
      }
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
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int year = ptm->tm_year + 1900;
  int month = ptm->tm_mon + 1;
  int day = ptm->tm_mday;
  int hour = ptm->tm_hour;
  int minute = ptm->tm_min;
  int second = ptm->tm_sec;
  char formattedTime[25];
  sprintf(formattedTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
  return String(formattedTime);
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
  
  String formattedTime = getFormattedTime();
  StaticJsonDocument<200> doc;

  doc["dataPoint"] = formattedTime;
  doc["paramType"] = "Data";
  doc["paramValue"] = incoming;
  doc["deviceId"] = device_id[0]; // Assuming the first device ID for this example
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  
  Serial.print("Publishing received data message: ");
  Serial.println(jsonBuffer);
  client.publish(topic_pub[0], jsonBuffer);
  
  doc.clear();

  doc["dataPoint"] = formattedTime;
  doc["paramType"] = "SNR";
  doc["paramValue"] = String(LoRa.packetSnr());
  doc["deviceId"] = device_id[0]; // Assuming the first device ID for this example
  
  serializeJson(doc, jsonBuffer);
  
  Serial.print("Publishing SNR message: ");
  Serial.println(jsonBuffer);
  client.publish(topic_pub[0], jsonBuffer);
  
  doc.clear();
  
  doc["dataPoint"] = formattedTime;
  doc["paramType"] = "RSSI";
  doc["paramValue"] = String(LoRa.packetRssi());
  doc["deviceId"] = device_id[0]; // Assuming the first device ID for this example
  
  serializeJson(doc, jsonBuffer);
  
  Serial.print("Publishing RSSI message: ");
  Serial.println(jsonBuffer);
  client.publish(topic_pub[0], jsonBuffer);
}

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
  
  server.on("/", handleRoot);
  server.on("/configure", HTTP_POST, handleConfigure);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
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
