#include <SPI.h>
#include <LoRa.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

#define EEPROM_SIZE 512

#define SS D8    // GPIO8
#define RST D0   // GPIO0
#define DIO0 D1  // GPIO1

ESP8266WebServer server(80);

// Credentials
char ap_ssid[50];
char ap_password[50];
char node_id[50];
char subscribe_id[50]; // Add subscribe ID

// LoRa communication parameters
String outgoing;
byte msgCount = 0;
byte localAddress;
byte destination = 0xA34;
long lastSendTime = 0;
int interval = 2000;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP Configuration</title>
</head>
<body>
    <h2>Configure ESP</h2>
    <form action="/submit" method="post">
        <label for="ap_ssid">AP SSID:</label><br>
        <input type="text" id="ap_ssid" name="ap_ssid"><br><br>
        <label for="ap_password">AP Password:</label><br>
        <input type="text" id="ap_password" name="ap_password"><br><br>
        <label for="node_id">Node ID:</label><br>
        <input type="text" id="node_id" name="node_id"><br><br>
        <label for="subscribe_id">Subscribe ID:</label><br>
        <input type="text" id="subscribe_id" name="subscribe_id"><br><br>
        <input type="submit" value="Save">
    </form>
</body>
</html>
)rawliteral";

void saveCredentials() {
  EEPROM.put(0, ap_ssid);
  EEPROM.put(50, ap_password);
  EEPROM.put(100, node_id);
  EEPROM.put(150, subscribe_id); // Save subscribe ID
  EEPROM.commit();
}

void loadCredentials() {
  EEPROM.get(0, ap_ssid);
  EEPROM.get(50, ap_password);
  EEPROM.get(100, node_id);
  EEPROM.get(150, subscribe_id); // Load subscribe ID

  if (ap_ssid[0] == 0 || ap_password[0] == 0 || node_id[0] == 0 || subscribe_id[0] == 0) {
    strcpy(ap_ssid, "NodeMCU");
    strcpy(ap_password, "12345678");
    strcpy(node_id, "0x70");
    strcpy(subscribe_id, "26773439927218");
  }
}

void setup_ap() {
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleSubmit() {
  if (server.hasArg("ap_ssid")) strcpy(ap_ssid, server.arg("ap_ssid").c_str());
  if (server.hasArg("ap_password")) strcpy(ap_password, server.arg("ap_password").c_str());
  if (server.hasArg("node_id")) strcpy(node_id, server.arg("node_id").c_str());
  if (server.hasArg("subscribe_id")) strcpy(subscribe_id, server.arg("subscribe_id").c_str());

  saveCredentials();
  localAddress = (byte)strtol(node_id, NULL, 16); // Update localAddress from node_id
  server.send(200, "text/plain", "Credentials Saved. Please restart the ESP.");
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  EEPROM.begin(EEPROM_SIZE);
  loadCredentials();

  setup_ap();

  Serial.println("LoRa Duplex");

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }

  Serial.println("LoRa init succeeded.");

  localAddress = (byte)strtol(node_id, NULL, 16); // Convert node_id to byte
  Serial.print("Local Address: 0x");
  Serial.println(localAddress, HEX);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.begin();
}

void loop() {
  if (millis() - lastSendTime > interval) {
    int randomValue = random(18, 21);
    String message = String(randomValue) ;
    sendMessage(message);
    Serial.println("Sending " + message);
    lastSendTime = millis();
    interval = random(2000) + 1000;
  }

  onReceive(LoRa.parsePacket());
  server.handleClient();
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

  if (recipient != localAddress && recipient != 0xBC) {
    Serial.println("This message is not for me.");
    return;
  }

  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);

  // Parse incoming message
  int delimiterPos = incoming.indexOf('+');
  if (delimiterPos != -1) {
    String receivedID = incoming.substring(0, delimiterPos);
    char status = incoming.charAt(delimiterPos + 1);

    if (receivedID.equals(subscribe_id)) {
      if (status == '1') {
        Serial.println("True");
      } else if (status == '0') {
        Serial.println("False");
      } else {
        Serial.println("Invalid status");
      }
    } else {
      Serial.println("Subscribed message is not for me.");
    }
  } else {
    Serial.println("Invalid message format");
  }

  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}

byte generateLocalAddressFromMAC() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  byte address = mac[5];
  return address;
}
