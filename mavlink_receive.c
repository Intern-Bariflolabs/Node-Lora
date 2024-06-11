#include <WiFi.h>
#include <WiFiUdp.h>
#include <MAVLink.h> // Include the generated MAVLink headers

// Wi-Fi credentials
const char* ssid = "BarifloLabs";
const char* password = "Bfl_wifi0@1234";

// Local UDP port to listen on
unsigned int localUdpPort = 14550;

WiFiUDP udp;

void setup() {
    Serial.begin(115200);
    
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to the WiFi network");

    // Print the local IP address
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Begin UDP communication
    if (!udp.begin(localUdpPort)) {
        Serial.println("Failed to begin UDP communication");
        while (1) {
            delay(1000);
        }
    }
    Serial.println("UDP communication started. Listening on port: " + String(localUdpPort));
}

void loop() {
    // Check for incoming UDP packets
    int packetSize = udp.parsePacket();
    if (packetSize) {
        Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
        
        // Read packet into buffer
        uint8_t packetBuffer[255]; // Adjust the buffer size as needed
        int len = udp.read(packetBuffer, 255);

        // Decode the MAVLink message
        mavlink_message_t msg;
        mavlink_status_t status;
        
        for (int i = 0; i < len; ++i) {
            if (mavlink_parse_char(MAVLINK_COMM_0, packetBuffer[i], &msg, &status)) {
                // Successfully decoded a MAVLink message
                Serial.println("Received MAVLink message:");
                Serial.printf("Message ID: %d\n", msg.msgid);
                
                // Handle the COMMAND_LONG message
                if (msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                    mavlink_command_long_t command;
                    mavlink_msg_command_long_decode(&msg, &command);
                    
                    Serial.println("COMMAND_LONG Message:");
                    Serial.printf("target_system: %d\n", command.target_system);
                    Serial.printf("target_component: %d\n", command.target_component);
                    Serial.printf("command: %d\n", command.command);
                    Serial.printf("confirmation: %d\n", command.confirmation);
                    Serial.printf("param1: %f\n", command.param1);
                    Serial.printf("param2: %f\n", command.param2);
                    Serial.printf("param3: %f\n", command.param3);
                    Serial.printf("param4: %f\n", command.param4);
                    Serial.printf("param5: %f\n", command.param5);
                    Serial.printf("param6: %f\n", command.param6);
                    Serial.printf("param7: %f\n", command.param7);
                }
            }
        }
    }
}
