// Include required libraries
#include <SPI.h>
#include <LoRa.h>

// Define the pins used by the LoRa module
const int csPin = 16;     // LoRa radio chip select
const int resetPin = 17;  // LoRa radio reset
const int irqPin = 21;    // Must be a hardware interrupt pin

// Message counter
byte buttonCounter = 0;

const int upPin = 5;
const int downPin = 6;
const int startPin = 9;
String contents = "";

void setup() {

  Serial.begin(115200);
  // Setup LoRa module
  LoRa.setPins(csPin, resetPin, irqPin); 

  Serial.println("LoRa Sender Test");

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  pinMode(upPin, INPUT_PULLUP);
  pinMode(downPin, INPUT_PULLUP);
  pinMode(startPin, INPUT_PULLUP);
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void onReceive(int packetSize){
  if (packetSize == 0) return;
  while (LoRa.available()){
    contents += (char) LoRa.read();
  }
  
  if(contents.substring(0,5).equals("P.O.D")){
    Serial.print(contents);
    Serial.print(" w/ RSSI: ");
    Serial.println(LoRa.packetRssi());
  }
  if(contents.substring(0,1).equals("{")){
    Serial.println(contents);
  }
  //Serial.println(contents);
  contents = "";
}

void loop() {
  // Send packet
  if(digitalRead(upPin) == LOW){
    LoRa.beginPacket();
    LoRa.print("up");
    LoRa.endPacket();
    Serial.print("Up pressed: ");
    Serial.println(buttonCounter);
    buttonCounter++;
  }

  else if(digitalRead(downPin) == LOW) {
    LoRa.beginPacket();
    LoRa.print("down");
    LoRa.endPacket();
    Serial.print("Down pressed: ");
    Serial.println(buttonCounter);
    buttonCounter++;
  }

  else if(digitalRead(startPin) == LOW){
    LoRa.beginPacket();
    LoRa.print("start");
    LoRa.endPacket();
    Serial.print("Start pressed: ");
    Serial.println(buttonCounter);
    buttonCounter++;
  }

  else {
    LoRa.beginPacket();
    LoRa.print("hold");
    LoRa.endPacket();
  }

  LoRa.receive();
  delay(200);
}