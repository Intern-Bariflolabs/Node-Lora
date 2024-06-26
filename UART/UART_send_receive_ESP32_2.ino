#include <HardwareSerial.h>

// Use UART2 (you can use UART0, UART1 or UART2)
HardwareSerial MySerial(2);

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000; // 2 seconds

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Initialize UART2 at 9600 baud rate
  MySerial.begin(9600, SERIAL_8N1, 16, 17); // GPIO16: RX, GPIO17: TX

  Serial.println("ESP32_2 ready to communicate");
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if it's time to send data
  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis;
    MySerial.println("Hello from ESP32_2");
    Serial.println("Sent: Hello from ESP32_2");
  }

  // Check if data is received from ESP32_1
  if (MySerial.available()) {
    String received = MySerial.readString();
    Serial.println("Received from ESP32_1: " + received);
  }
}
