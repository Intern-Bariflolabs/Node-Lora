#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <WebServer.h>

#define EEPROM_SIZE 2048
#define MAX_TOPICS 10
#define MAX_DEVICE_ID_LENGTH 50
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
char device_id[50];
int num_topics;
char topics_pub[10][50];
char topics_sub[10][50];
char device_ids[MAX_TOPICS][MAX_DEVICE_ID_LENGTH];
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
    <script>
        function updateTopicFields() {
            var numTopics = document.getElementById('num_topics').value;
            var topicFields = '';
            for (var i = 1; i <= numTopics; i++) {
                topicFields += '<label for="topic_pub' + i + '">Publish Topic ' + i + ':</label><br>';
                topicFields += '<input type="text" id="topic_pub' + i + '" name="topic_pub' + i + '"><br><br>';
                topicFields += '<label for="topic_sub' + i + '">Subscribe Topic ' + i + ':</label><br>';
                topicFields += '<input type="text" id="topic_sub' + i + '" name="topic_sub' + i + '"><br><br>';
                topicFields += '<label for="device_id' + i + '">Device ID ' + i + ':</label><br>'; // New input for device ID
                topicFields += '<input type="text" id="device_id' + i + '" name="device_id' + i + '"><br><br>'; // New input for device ID
            }
            document.getElementById('topics').innerHTML = topicFields;
        }
    </script>
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
        
        <label for="num_topics">Number of Topics:</label><br>
        <select id="num_topics" name="num_topics" onchange="updateTopicFields()" required/>
            <option value = "select">select</option>
            <option value="1">1</option>
            <option value="2">2</option>
            <option value="3">3</option>
            <option value="4">4</option>
            <option value="5">5</option>
            <option value="6">6</option>
            <option value="7">7</option>
            <option value="8">8</option>
            <option value="9">9</option>
            <option value="10">10</option>
        </select><br><br>
        <div id="topics">
            </div>
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
  EEPROM.writeString(310, device_id);
  EEPROM.put(360, num_topics);
  for (int i = 0; i < num_topics; i++) {
    EEPROM.writeString(370 + i * 50, topics_pub[i]);
    EEPROM.writeString(870 + i * 50, topics_sub[i]);
  }
  for (int i = 0; i < num_topics; i++) {
    EEPROM.writeString(1370 + i * 50, device_ids[i]);
  }
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
  EEPROM.get(310, device_id);
  EEPROM.get(360, num_topics);
  for (int i = 0; i < num_topics; i++) {
    EEPROM.get(370 + i * 50, topics_pub[i]);
    EEPROM.get(870 + i * 50, topics_sub[i]);
  }
  for (int i = 0; i < num_topics; i++) {
    EEPROM.get(1370 + i * 50, device_ids[i]);
  }
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
    WiFi.softAP("ESP32_AP", "12345678");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
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
      for (int i = 0; i < num_topics; i++) {
        client.subscribe(topics_sub[i]);
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
  if (server.hasArg("device_id")) strcpy(device_id, server.arg("device_id").c_str());
  if (server.hasArg("num_topics")) num_topics = server.arg("num_topics").toInt();

  for (int i = 0; i < num_topics; i++) {
    String topic_pub_arg = "topic_pub" + String(i + 1);
    String topic_sub_arg = "topic_sub" + String(i + 1);
    String device_id_arg = "device_id" + String(i + 1);
    if (server.hasArg(topic_pub_arg)) strcpy(topics_pub[i], server.arg(topic_pub_arg).c_str());
    if (server.hasArg(topic_sub_arg)) strcpy(topics_sub[i], server.arg(topic_sub_arg).c_str());
    if (server.hasArg(device_id_arg)) strcpy(device_ids[i], server.arg(device_id_arg).c_str()); 
  }

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
      for (int i = 0; i < num_topics; i++) {
      StaticJsonDocument<200> doc;
      doc["dataPoint"] = formattedTime;
      doc["paramType"] = "cpu_temp";
      doc["paramValue"] = random(19, 21);
      doc["deviceId"] = device_ids[i];

      char jsonBuffer[512];
      serializeJson(doc, jsonBuffer);

      Serial.print("Publishing message: ");
      Serial.println(jsonBuffer);
      
      client.publish(topics_pub[i], jsonBuffer);
      }
    }
  }

  server.handleClient();
}
//  doc["deviceId"] = device_id; --> at this point every publish and subscribe topic have different device id so add a array and in the frontend change it to take input of multiple device id as like multiple topic publish and subscribe , but device id is comptely different parameter 

