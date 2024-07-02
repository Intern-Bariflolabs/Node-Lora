#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <LoRa.h>

#define EEPROM_SIZE 2048
#define MAX_TOPICS 10
#define MAX_DEVICE_ID_LENGTH 50

#define MODEM_TX 17
#define MODEM_RX 16

WebServer server(80);

char mqtt_server[50];
int mqtt_port;
char mqtt_user[50];
char mqtt_password[50];
char client_id[50];
char device_id[50];
int num_topics;
char topics_pub[MAX_TOPICS][50];
char topics_sub[MAX_TOPICS][50];
char device_ids[MAX_TOPICS][MAX_DEVICE_ID_LENGTH];

const long utcOffsetInSeconds = 19800;

TinyGsm modem(Serial2);
TinyGsmClient gsmClient(modem);
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

const char apn[] = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];

String incomingMessage;
byte msgCount = 0;
byte localAddress = 0xA34;
byte destination = 0xBC;
long lastSendTime = 0;
int interval = 2000;

#define SS 5    // GPIO5
#define RST 14  // GPIO14
#define DIO0 2  // GPIO2

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
                topicFields += '<label for="device_id' + i + '">Device ID ' + i + ':</label><br>';
                topicFields += '<input type="text" id="device_id' + i + '" name="device_id' + i + '"><br><br>';
            }
            document.getElementById('topics').innerHTML = topicFields;
        }
    </script>
</head>
<body>
    <h2>Configure ESP32</h2>
    <form action="/submit" method="post">
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
        <select id="num_topics" name="num_topics" onchange="updateTopicFields()" required>
            <option value="select">select</option>
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

void setup_gsm() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    delay(6000);

    Serial.println("Initializing modem...");
    if (!modem.restart()) {
        Serial.println("Failed to restart modem, halting");
        while (true);
    }

    String modemInfo = modem.getModemInfo();
    Serial.print("Modem Info: ");
    Serial.println(modemInfo);

    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
        Serial.println(" fail");
        delay(10000);
        return;
    }
    Serial.println(" success");

    if (modem.isNetworkConnected()) {
        Serial.println("Network connected");
    }

    Serial.print(F("Connecting to "));
    Serial.print(apn);

    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        Serial.println(" fail");
        delay(10000);
        return;
    }
    Serial.println(" success");

    if (modem.isGprsConnected()) {
        Serial.print("Local IP: ");
        Serial.println(modem.localIP());
    }

    // Setup WiFi access point for configuration
    WiFi.softAP("ESP32_AP", "12345678");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    String displayId = doc["display_id"];
    bool status = doc["status"];
    incomingMessage = displayId + "+" + String(status);

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
    char formattedTime[20];
    sprintf(formattedTime, "%02d:%02d:%02d", (ptm->tm_hour + 5) % 24, ptm->tm_min, ptm->tm_sec);
    return String(formattedTime);
}

void handleRoot() {
    String html = FPSTR(index_html);
    server.send(200, "text/html", html);
}

void handleSubmit() {
    Serial.println("Handling form submission");
    mqtt_server[0] = '\0';
    mqtt_port = server.arg("mqtt_port").toInt();
    strcpy(mqtt_server, server.arg("mqtt_server").c_str());
    strcpy(mqtt_user, server.arg("mqtt_user").c_str());
    strcpy(mqtt_password, server.arg("mqtt_password").c_str());
    strcpy(client_id, server.arg("client_id").c_str());
    num_topics = server.arg("num_topics").toInt();

    for (int i = 1; i <= num_topics; i++) {
        strcpy(topics_pub[i - 1], server.arg("topic_pub" + String(i)).c_str());
        strcpy(topics_sub[i - 1], server.arg("topic_sub" + String(i)).c_str());
        strcpy(device_ids[i - 1], server.arg("device_id" + String(i)).c_str());
    }

    saveCredentials();
    loadCredentials();

    String response = "<html><body><h1>Configuration Saved</h1><p>Restart device for changes to take effect.</p></body></html>";
    server.send(200, "text/html", response);
}

void sendMessage(String outgoing) {
    if (client.publish(topics_pub[0], (char*)outgoing.c_str())) {
        Serial.println("Publish ok");
    } else {
        Serial.println("Publish failed");
    }
}

void onReceive(int packetSize) {
    if (packetSize == 0) return;

    while (LoRa.available()) {
        incomingMessage = "";
        incomingMessage = LoRa.readString();
        Serial.println("Received: " + incomingMessage);
    }
}

void setup() {
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    loadCredentials();
    setup_gsm();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);
    server.begin();

    if (WiFi.status() == WL_CONNECTED) {
        client.setServer(mqtt_server, mqtt_port);
        client.setCallback(callback);
        timeClient.begin();
        timeClient.update();

        Serial.println("LoRa Duplex");
        LoRa.setPins(SS, RST, DIO0);

        if (!LoRa.begin(433E6)) {
            Serial.println("LoRa init failed. Check your connections.");
            while (true);
        }
        Serial.println("LoRa init succeeded.");
    }
}

void loop() {
    server.handleClient();
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    timeClient.update();
    long now = millis();

    // Perform periodic tasks here
    if (now - lastSendTime > interval) {
        sendMessage(getFormattedTime());
        lastSendTime = now;
        interval = random(2000) + 1000;
    }

    // Handle LoRa messages
    onReceive(LoRa.parsePacket());
}
