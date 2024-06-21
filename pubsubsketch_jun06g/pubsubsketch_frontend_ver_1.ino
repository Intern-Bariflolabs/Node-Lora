#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <WebServer.h>

#define EEPROM_SIZE 512

// Initialize web server on port 80
WebServer server(80);

// Credentials
char ssid[50];
char password[50];
char mqtt_server[50];
int mqtt_port;
char mqtt_user[50];
char mqtt_password[50];
char client_id[50];
char topic_pub[50];
char topic_sub[50];
char device_id[50];

// MQTT and NTP clients
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800); // UTC offset for India (5.5 hours)

// Initialize LED Pin
const int ledPin = 2;
long lastMsg = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Configuration</title>
</head>
<body>
    <h2>Configure ESP32</h2>
    <form action="/submit" method="post">
        <label for="ssid">WiFi SSID:</label><br>
        <input type="text" id="ssid" name="ssid"><br><br>
        <label for="password">WiFi Password:</label><br>
        <input type="text" id="password" name="password"><br><br>
        <label for="mqtt_server">MQTT Server:</label><br>
        <input type="text" id="mqtt_server" name="mqtt_server"><br><br>
        <label for="mqtt_port">MQTT Port:</label><br>
        <input type="text" id="mqtt_port" name="mqtt_port"><br><br>
        <label for="mqtt_user">MQTT User:</label><br>
        <input type="text" id="mqtt_user" name="mqtt_user"><br><br>
        <label for="mqtt_password">MQTT Password:</label><br>
        <input type="text" id="mqtt_password" name="mqtt_password"><br><br>
        <label for="client_id">Client ID:</label><br>
        <input type="text" id="client_id" name="client_id"><br><br>
        <label for="topic_pub">Publish Topic:</label><br>
        <input type="text" id="topic_pub" name="topic_pub"><br><br>
        <label for="topic_sub">Subscribe Topic:</label><br>
        <input type="text" id="topic_sub" name="topic_sub"><br><br>
        <label for="device_id">Device ID:</label><br>
        <input type="text" id="device_id" name="device_id"><br><br>
        <input type="submit" value="Save">
    </form>
</body>
</html>
)rawliteral";

void saveCredentials() {
  EEPROM.writeString(0, ssid);
  EEPROM.writeString(50, password);
  EEPROM.writeString(100, mqtt_server);
  EEPROM.put(150, mqtt_port);
  EEPROM.writeString(160, mqtt_user);
  EEPROM.writeString(210, mqtt_password);
  EEPROM.writeString(260, client_id);
  EEPROM.writeString(310, topic_pub);
  EEPROM.writeString(360, topic_sub);
  EEPROM.writeString(410, device_id);
  EEPROM.commit();
}

void loadCredentials() {
  EEPROM.get(0, ssid);
  EEPROM.get(50, password);
  EEPROM.get(100, mqtt_server);
  EEPROM.get(150, mqtt_port);
  EEPROM.get(160, mqtt_user);
  EEPROM.get(210, mqtt_password);
  EEPROM.get(260, client_id);
  EEPROM.get(310, topic_pub);
  EEPROM.get(360, topic_sub);
  EEPROM.get(410, device_id);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) { // 20 seconds timeout
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi. Starting AP mode...");
    WiFi.softAP("ESP32_AP", "12345678");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
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

  // Blink the LED when a message is received
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
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

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleSubmit() {
  if (server.hasArg("ssid")) strcpy(ssid, server.arg("ssid").c_str());
  if (server.hasArg("password")) strcpy(password, server.arg("password").c_str());
  if (server.hasArg("mqtt_server")) strcpy(mqtt_server, server.arg("mqtt_server").c_str());
  if (server.hasArg("mqtt_port")) mqtt_port = server.arg("mqtt_port").toInt();
  if (server.hasArg("mqtt_user")) strcpy(mqtt_user, server.arg("mqtt_user").c_str());
  if (server.hasArg("mqtt_password")) strcpy(mqtt_password, server.arg("mqtt_password").c_str());
  if (server.hasArg("client_id")) strcpy(client_id, server.arg("client_id").c_str());
  if (server.hasArg("topic_pub")) strcpy(topic_pub, server.arg("topic_pub").c_str());
  if (server.hasArg("topic_sub")) strcpy(topic_sub, server.arg("topic_sub").c_str());
  if (server.hasArg("device_id")) strcpy(device_id, server.arg("device_id").c_str());

  saveCredentials();
  server.send(200, "text/plain", "Credentials Saved. Please restart the ESP32.");
}

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadCredentials();

  setup_wifi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.begin();

  if (WiFi.status() == WL_CONNECTED) {
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    timeClient.begin();
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    timeClient.update();

    long now = millis();
    if (now - lastMsg > 10000) {
      lastMsg = now;
      String formattedTime = getFormattedTime();

      StaticJsonDocument<200> doc;
      doc["dataPoint"] = formattedTime;
      doc["paramType"] = "cpu_temp";
      doc["paramValue"] = random(19, 21);
      doc["deviceId"] = device_id;

      char jsonBuffer[512];
      serializeJson(doc, jsonBuffer);

      Serial.print("Publishing message: ");
      Serial.println(jsonBuffer);
      client.publish(topic_pub, jsonBuffer);
    }
  }

  server.handleClient();
}
