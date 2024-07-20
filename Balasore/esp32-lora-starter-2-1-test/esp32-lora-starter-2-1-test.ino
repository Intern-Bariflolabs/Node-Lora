// Author @ Biswajit Nayak
// Library //////////////////////////////////////////////////////
#include <SPI.h>
#include <LoRa.h>
//#include <WiFi.h>
#include "EmonLib.h"
EnergyMonitor emon1, emon2, emon3, emon4, emon5, emon6;
// LoRa Pins //////////////////////////////////////////////////////////
#define SS 5    // GPIO5
#define RST 14  // GPIO14
#define DIO0 2  // GPIO2
// CT pins Analog input from CT ////////////////////////////////////////
int CT_r1 = 33; // R1
int CT_y1 = 32; // Y1
int CT_b1 = 35; // B1

int CT_r2 = 34; // R2
int CT_y2 = 39; // Y2
int CT_b2 = 36; // B2
// SSR pins output //////////////////////////////////////////////////////
int strt1 = 25; // start relay for A1
int stop1 = 26; // stop relay for A1

int strt2 = 27; // start relay for A2
int stop2 = 13; // stop relay for A2
// Control pin output //////////////////////////////////////////////////
int control = 17; // control pin for dashboard
/////////////////////////////////////////////////////////////////////
String val;
bool slot1 = false;
String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
//byte localAddress = 0x3D;     // address of this device
byte localAddress = 0xb4;     // address of this device
byte destination = 0xA34;     // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

long lastControlToggle = 0;   // last time the control pin was toggled
long lastControlToggle2 = 10000;   
//const long controlInterval = 3600000; // 1 hour in milliseconds
const long controlInterval = 10000; // 10 sec in milliseconds
const long controlInterval2 = 10000; // 10 sec in milliseconds
bool toggleState = false; 

String Dev_status;

void setup() {
  pinMode(strt1, OUTPUT);
  pinMode(stop1, OUTPUT);
  pinMode(strt2, OUTPUT);
  pinMode(stop2, OUTPUT);

  pinMode(control, OUTPUT);

  emon1.current(CT_r1, 1.6);
  emon2.current(CT_y1, 1.6);
  emon3.current(CT_b1, 1.6);
  emon4.current(CT_r2, 1.6);
  emon5.current(CT_y2, 1.6);
  emon6.current(CT_b2, 1.6);

  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Duplex");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
  Serial.print("Local Address: 0x");
  Serial.println(localAddress, HEX);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastControlToggle >= controlInterval) {
    if(toggleState) {
    digitalWrite(strt1, HIGH);
    //delay(10);
    digitalWrite(strt2, HIGH);
    delay(1000);
    digitalWrite(strt1, LOW);
    digitalWrite(strt2, LOW);
    Serial.println("Aerator ON"); 
    }
    else
    {
    digitalWrite(stop1, HIGH);
    //delay(10);
    digitalWrite(stop2, HIGH);
    delay(1000);
    digitalWrite(stop1, LOW);
    digitalWrite(stop2, LOW);
    Serial.println("Aerator OFF");
    }
    toggleState = !toggleState;
    lastControlToggle = currentMillis;
  }
  double Irmsr1 = emon1.calcIrms(1480);
  double Irmsy1 = emon2.calcIrms(1480);
  double Irmsb1 = emon3.calcIrms(1480);

  double Irmsr2 = emon4.calcIrms(1480);
  double Irmsy2 = emon5.calcIrms(1480);
  double Irmsb2 = emon6.calcIrms(1480);

  double powr1 = Irmsr1 * 420.0;
  //Serial.print(" R1 ");
  //Serial.println(Irmsr1);

  double powy1 = Irmsy1 * 420.0;
  //Serial.print(" Y1 ");
  //Serial.println(Irmsy1);

  double powb1 = Irmsb1 * 420.0;
  //Serial.print(" B1 ");
  //Serial.println(Irmsb1);

  double powr2 = Irmsr2 * 420.0;
  //Serial.print(" R2 ");
  //Serial.println(Irmsr2);

  double powy2 = Irmsy2 * 420.0;
  //Serial.print(" Y2 ");
  //Serial.println(Irmsy2);

  double powb2 = Irmsb2 * 420.0;
  //Serial.print(" B2 ");
  //Serial.println(Irmsb2);
  //delay(100);
  val = String("R1" + String(Irmsr1) + "," + "Y1" + String(Irmsy1) + "," + "B1" + String(Irmsb1) + "," 
               + "R2" + String(Irmsr2) + "," + "Y2" + String(Irmsy2) + "," + "B2" + String(Irmsb2)+","
               + "PR1" + String(powr1) + "," + "PY1" + String(powy1) + "," + "PB1" + String(powb1) + "," 
               + "PR2" + String(powr2) + "," + "PY2" + String(powy2) + "," + "PB2" + String(powb2)+"," + "Status" + Dev_status);
 // Check if any current value exceeds 3A
 if (Irmsr1 > 3 || Irmsy1 > 3 || Irmsb1 > 3 || Irmsr2 > 3 || Irmsy2 > 3 || Irmsb2 > 3) 
{
    //digitalWrite(control, HIGH);
    Serial.println("Device OFF");
    //Dev_status = "False";
} 
else {
    digitalWrite(control, LOW);
    Serial.println("Device ON");
    Dev_status = "True";
  }
  Serial.println(Dev_status);
  if (currentMillis - lastSendTime > interval) {
    sendMessage(val);
    Serial.println("Sending " + val);
    Serial.println(val.length());
    lastSendTime = currentMillis;
    interval = random(2000) + 1000;
  }
  onReceive(LoRa.parsePacket());
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
  int msg2 = outgoing.length();
  int total = 3 + msg2;
  Serial.print("Uplink message length: ");
  Serial.println(total);
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
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}
