#include <Adafruit_SH110X.h>
#include <splash.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Wire.h>
#include "MS5837.h"
#include <SPI.h>
#include <LoRa.h>
#include <ArrayList.h>

#define MOTOR_IN1 9
#define MOTOR_IN2 10

MS5837 sensor;
// Define the pins used by the LoRa module
const int csPin = 16;     // LoRa radio chip select
const int resetPin = 17;  // LoRa radio reset
const int irqPin = 21;    // Must be a hardware interrupt pin
 
// LED connection
const int ledPin = 13;
 
// Receive message variables
String contents = "";
String go_up = "up";
String go_down = "down";
String start_mission = "start";
String hold = "hold";
String profiling_data = "P.O.D Data: ";
int profiles_completed = 0;
boolean profile_completed = false;
boolean perform_mission = false;

struct DepthTS {
  unsigned long time;
  double depth;
};

ArrayList<DepthTS> myList(ArrayList<DepthTS>::DYNAMIC);
DepthTS depth_ts;
unsigned long start_time = millis();
unsigned long last_marker = millis();
unsigned long current_time = millis();

void setup() {
 
  // Set LED as output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay(5000);
  Serial.begin(9600);
  digitalWrite(ledPin, LOW);
  delay(2000);
 
  // Setup LoRa module
  LoRa.setPins(csPin, resetPin, irqPin);
  Serial.println("LoRa Receiver");

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  Wire.begin();
  sensor.setModel(MS5837::MS5837_02BA);
  sensor.setFluidDensity(1000);

  while (!sensor.init()) {
    Serial.println("Init failed!");
    Serial.println("Are SDA/SCL connected correctly?");
    Serial.println("Blue Robotics Bar02: White=SDA, Green=SCL");
    Serial.println("\n\n\n");
    delay(5000);
  }

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void onReceive(int packetSize){
  if (packetSize == 0) return;

  while (LoRa.available()) {
    contents += (char)LoRa.read();
  }
  if (contents.equals(go_up)) {
    Serial.println(contents);
    goUp();
  }

  else if (contents.equals(go_down)) {
    Serial.println(contents);
    goDown();
  }

  else if (contents.equals(start_mission)){
    Serial.println(contents);
    perform_mission = true;
  }

  else if (contents.equals("hold")){
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
  }
  contents = "";
}

void loop() {
  digitalWrite(ledPin, HIGH);
  delay(200);
  digitalWrite(ledPin, LOW);
  delay(800);
  displayDepth();
  sensor.read();
  LoRa.beginPacket();
  LoRa.print("P.O.D  ");
  LoRa.print(millis()/1000);
  LoRa.print("  ");
  LoRa.print(sensor.pressure());
  LoRa.print(" KPa");
  LoRa.print("  ");
  LoRa.print(sensor.depth());
  LoRa.print(" meters");
  LoRa.endPacket();
  if(perform_mission){
    perform_mission = false;
    performMission();
  }
  if(profiles_completed <= 2 && profile_completed){
    delay(1000);
    LoRa.beginPacket();
    LoRa.print("P.O.D DATA SENDING!!!");
    LoRa.endPacket();
    delay(500);
    for(int i = 0; i < myList.size(); i++){
      depth_ts = myList.get(i);
      String data = "{";
      data += String(depth_ts.time);
      data += ",";
      data += String(depth_ts.depth);
      data += "}";
      Serial.println(data);
      LoRa.beginPacket();
      LoRa.print(String(data));
      LoRa.endPacket();
      delay(500);
    }
    delay(100);
    Serial.print("Profiling data: ");
    Serial.println(profiling_data);
    profiling_data = "P.O.D data: ";
    myList.clear();
    profile_completed = false;
    if(profiles_completed == 1){
      performMission();
    }
    if(profiles_completed == 2){
      profiles_completed = 0;
    }
  }
  LoRa.receive();
}

void displayDepth() {
  sensor.read();
  Serial.println(sensor.depth());
}
// Print time-series depth data
void print_ts(struct DepthTS ts){
  Serial.print("time: ");
  Serial.print(ts.time);
  Serial.print(", depth: ");
  Serial.println(ts.depth);
}
// Manually go up
void goUp() {
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN1, LOW);
  Serial.println("going up");
}
// Manually go down
void goDown() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  Serial.println("going down");
}

void performMission() {
  Serial.println("Starting Mission!!!");
  digitalWrite(MOTOR_IN1,LOW);
  digitalWrite(MOTOR_IN2,LOW);
  sensor.read();
  float depth = sensor.depth();
  // Descends until P.O.D is 1 meter deep
  while (depth < 1.0){
    digitalWrite(MOTOR_IN1,HIGH);
    displayDepth();
    sensor.read();
    depth = sensor.depth();
    current_time = millis();
    if((current_time-last_marker)>1000){
      depth_ts = {current_time/1000, depth};
      myList.add(depth_ts);
      last_marker = current_time;
    }
    delay(100);
  }
  // Coasts until P.O.D is 2.5 meters deep
  while (sensor.depth() < 2.5){
    digitalWrite(MOTOR_IN1,LOW);
    displayDepth();
    sensor.read();
    depth = sensor.depth();
    current_time = millis();
    if((current_time-last_marker)>1000){
      depth_ts = {current_time/1000, depth};
      myList.add(depth_ts);
      last_marker = current_time;
    } 
    delay(100);     
  }  
  // Ascends until P.O.D is 1.5 meters deep
  while (sensor.depth() > 1.5){
    digitalWrite(MOTOR_IN2, HIGH);
    displayDepth();
    sensor.read();
    depth = sensor.depth();
    current_time = millis();
    if((current_time-last_marker)>1000){
      depth_ts = {current_time/1000, depth};
      myList.add(depth_ts);
      last_marker = current_time;
    }
    delay(100);
  }
  // Coasts until P.O.D is 0.5 meters deep
  while (sensor.depth() > 0.5){
    digitalWrite(MOTOR_IN2,LOW);
    displayDepth();
    sensor.read();
    depth = sensor.depth();
    current_time = millis();
    if((current_time-last_marker)>1000){
      depth_ts = {current_time/1000, depth};
      myList.add(depth_ts);
      last_marker = current_time;
    }
    delay(100);
  }

  digitalWrite(MOTOR_IN1,LOW);
  digitalWrite(MOTOR_IN2,LOW);
  DepthTS ts;
  
  for(int i = 0; i < myList.size(); i++){
    ts = myList.get(i);
    print_ts(myList.get(i));
    profiling_data += "{";
    profiling_data += ts.time;
    profiling_data += ",";
    profiling_data += ts.depth;
    profiling_data += "} ";
  }
  profile_completed = true;
  profiles_completed += 1;
  delay(1000);
  Serial.println("Successfully completed profile");
}