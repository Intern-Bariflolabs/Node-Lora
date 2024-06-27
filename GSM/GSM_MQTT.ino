#define TINY_GSM_MODEM_BG96

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
#define MODEM_RST   5
#define MODEM_PWRKEY 4
#define MODEM_POWER_ON  23
#define MODEM_TX    17
#define MODEM_RX    16

// Define TinyGsm modem
TinyGsm modem(Serial2);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

// Your GPRS credentials
const char apn[] = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT credentials
const char* broker = "4.240.114.7";
const int mqtt_port_non_ssl = 1883;
const int mqtt_port_ssl =  8883; // Common SSL port
const char* mqtt_user = "BarifloLabs";
const char* mqtt_password = "Bfl@123";
const char* client_id = "ESP32Client";
const char* topic_pub = "26773439927218/data";
const char* topic_sub = "26773439927218";

uint32_t lastReconnectAttempt = 0;

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();
}

boolean mqttConnect(int port) {
  SerialMon.print("Connecting to MQTT broker ");
  SerialMon.print(broker);
  SerialMon.print(" on port ");
  SerialMon.print(port);

  mqtt.setServer(broker, port);

  // Connect to MQTT Broker
  boolean status = mqtt.connect(client_id, mqtt_user, mqtt_password);

  if (status == false) {
    SerialMon.print(" fail, rc=");
    SerialMon.print(mqtt.state());
    SerialMon.println(" try again in 5 seconds");
    return false;
  }
  SerialMon.println(" success");
  mqtt.publish(topic_pub, "{\"status\":\"connected\"}");
  mqtt.subscribe(topic_sub);
  return mqtt.connected();
}

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);

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

  // MQTT Broker setup
  mqtt.setCallback(mqttCallback);
}

void loop() {
  // Check if modem is connected to the network
  if (!modem.isNetworkConnected()) {
    SerialMon.println("Network disconnected");
    if (!modem.waitForNetwork(180000L, true)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    if (modem.isNetworkConnected()) {
      SerialMon.println("Network re-connected");
    }
  }

  // Check if MQTT is connected
  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");

    // Attempt to reconnect modem if not already connected
    if (!modem.isNetworkConnected()) {
      SerialMon.println("Connecting to network...");
      if (!modem.waitForNetwork(180000L, true)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
      }
      if (modem.isNetworkConnected()) {
        SerialMon.println("Network connected");
      }
    }

    // Attempt MQTT connection
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      SerialMon.println("Attempting to connect to MQTT broker...");
      if (!modem.isNetworkConnected()) {
      SerialMon.println("Connecting to network...");
      if (!modem.waitForNetwork(180000L, true)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
      }
      if (modem.isNetworkConnected()) {
        SerialMon.println("Network connected");
      }
    }

      if (mqttConnect(mqtt_port_non_ssl)) {
        lastReconnectAttempt = 0;
      } else {
        SerialMon.println("Attempting to connect to MQTT broker with SSL...");
        if (!modem.isNetworkConnected()) {
          SerialMon.println("Connecting to network...");
        if (!modem.waitForNetwork(180000L, true)) {
          SerialMon.println(" fail");
         delay(10000);
         return;
      }
      if (modem.isNetworkConnected()) {
        SerialMon.println("Network connected");
      }
    }

        if (mqttConnect(mqtt_port_ssl)) {
          lastReconnectAttempt = 0;
        }
      }
    }
  } else {
    // MQTT is connected, maintain the connection
    mqtt.loop();

    // Publish messages periodically
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > 10000) {
      lastPublish = millis();

      StaticJsonDocument<200> doc;
      doc["dataPoint"] = millis();
      doc["paramType"] = "cpu_temp";
      doc["paramValue"] = random(19, 21);
      doc["deviceId"] = "26773439927218";

      char jsonBuffer[512];
      serializeJson(doc, jsonBuffer);

      SerialMon.print("Publishing message: ");
      SerialMon.println(jsonBuffer);
      mqtt.publish(topic_pub, jsonBuffer);
    }
  }

  delay(100); // Allow some delay to handle other tasks
}
