// Include required libraries
#include <SPI.h>
#include <LoRa.h>

// Define the pins used by the LoRa module
const int csPin = 16;     // LoRa radio chip select
const int resetPin = 17;  // LoRa radio reset
const int irqPin = 21;    // Must be a hardware interrupt pin

// Message counter
byte buttonCounter = 0;

const int upPin = 6;
const int downPin = 5;
const int startPin = 9;
// const int resetFloatPin = 10;
bool receivedHeader = false;
String contents = "";

void setup() {

  Serial.begin(9600);
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
  // pinMode(resetFloatPin, INPUT_PULLUP);
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void onReceive(int packetSize){
  if (packetSize == 0) return;
  while (LoRa.available()){
    contents += (char) LoRa.read();
  }
  
  if(contents.substring(0,4).equals("EX15")){
    displayContents(contents);
    receivedHeader = false;
  }
  if(contents.substring(0,3).equals("---") && !receivedHeader){
    displayContents(contents);
    receivedHeader = true;
  }
  if(contents.substring(0,5).equals("*EX15")){
    displayContents(contents);
    receivedHeader = false;
  }
  contents = "";
}

void loop() {
  // Send packet
  if(digitalRead(upPin) == HIGH){
    sendMessage("up");
    Serial.print("Up pressed: ");
    Serial.println(buttonCounter);
    buttonCounter++;
  }

  else if(digitalRead(downPin) == HIGH) {
    sendMessage("down");
    Serial.print("Down pressed: ");
    Serial.println(buttonCounter);
    buttonCounter++;
  }

  else if(digitalRead(startPin) == HIGH){
    sendMessage("start");
    Serial.print("Start pressed: ");
    Serial.println(buttonCounter);
    buttonCounter++;
  }

  // else if(digitalRead(resetFloatPin) == LOW){
  //   sendMessage("reset");
  // }

  else {
    sendMessage("hold");
  }

  LoRa.receive();
  delay(500);
}

void sendMessage(String msg){
  LoRa.beginPacket();
  LoRa.print(String(msg));
  LoRa.endPacket();
}

void displayContents(String contents){
  Serial.print(contents);
  Serial.print(" w/ RSSI: ");
  Serial.println(LoRa.packetRssi());
}