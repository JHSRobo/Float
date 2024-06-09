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

String profiling_data = "P.O.D Data: ";
String data_msg;
int profile_count = 0;
boolean profile_completed = false;
boolean perform_mission = false;
boolean aborted_mission = false;

struct DepthTS {
  unsigned long time;
  double depth;
  double pressure;
};

ArrayList<DepthTS> myList(ArrayList<DepthTS>::DYNAMIC);
DepthTS depth_ts;
unsigned long start_time = millis();
unsigned long last_marker = millis();
unsigned long current_time = millis();
unsigned long command_start;

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
  if (contents.equals("up")) {
    Serial.println(contents);
    goUp();
  }

  else if (contents.equals("down")) {
    Serial.println(contents);
    goDown();
  }

  else if (contents.equals("start")){
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
  depth_ts = readData();
  Serial.println(depth_ts.depth);
  sendData(depth_ts, "", "");
  
  if(perform_mission){
    perform_mission = false;
    performMission();
  }
  if(profile_completed){
    data_msg = "---start of profile " + String(profile_count) + " data---";
    for(int i = 0; i < 10; i++){
      sendMessage(data_msg);
      delay(500);
    }
    String profile_suffix = " " + String(profile_count);
    for(int i = 0; i < myList.size(); i++){
      depth_ts = myList.get(i);
      sendData(depth_ts, "*", profile_suffix);
      delay(500);
    }
    Serial.println(myList.size());
    data_msg = "---end---";
    for(int i = 0; i < 10; i++){
      sendMessage(data_msg);
      delay(500);
    }
    profile_completed = false;
    myList.clear();
  }
  LoRa.receive();
}

DepthTS readData() {
  DepthTS data;
  sensor.read();
  data.time = millis()/1000;
  data.depth = sensor.depth();
  data.pressure = sensor.pressure()/10;
  return data;
}
void sendData(DepthTS ts, String prefix, String suffix){
  String msg = prefix + "EX15 " + String(ts.time) + " sec " + String(ts.depth) + " m " + String(ts.pressure) + " kPa" + suffix;
  sendMessage(msg);
}

void sendMessage(String msg){
  LoRa.beginPacket();
  LoRa.print(String(msg));
  LoRa.endPacket();
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

void abortMission(String status){
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  aborted_mission = true;
  if(status.equals("pumping down")){
    Serial.println("Pumping Down Abort");
    delay(5000);
    digitalWrite(MOTOR_IN2, HIGH);
    delay(50000);
  }
  if(status.equals("coasting down")){
    Serial.println("Coasting Down Abort");
    digitalWrite(MOTOR_IN2, HIGH);
    delay(30000);
  }
  if(status.equals("pumping up")){
    Serial.println("Pumping Up Abort");
    delay(30000);
  }
  if(status.equals("coasting up")){
    Serial.println("Coasting Up Abort");
    delay(10000);
  }
  Serial.println("Finished aborted mission");
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  delay(1000);
}

void performMission() {
  Serial.println("Starting Mission!!!");
  digitalWrite(MOTOR_IN1,LOW);
  digitalWrite(MOTOR_IN2,LOW);
  sensor.read();
  double depth = sensor.depth();
  while((depth>0.5) || (depth<-0.5)){
    sensor.read();
    depth = sensor.depth();
  }
  double previous_depth;
  double pressure = sensor.pressure();
  // Descends until P.O.D is 1 meter deep
  command_start = millis();
  while ((depth < 1.0) && !aborted_mission){
    digitalWrite(MOTOR_IN1,HIGH);
    displayDepth();
    current_time = millis();
    if((current_time-last_marker)>1000){
      depth_ts = readData();
      myList.add(depth_ts);
      last_marker = current_time;
    }
    if((current_time-command_start)>60000){
      abortMission("pumping down");
      break;
    }
    previous_depth = depth;
    sensor.read();
    depth = sensor.depth();
    while(abs(depth-previous_depth)>1.0){
      sensor.read();
      depth = sensor.depth();
      current_time = millis();
      if((current_time-command_start)>60000){
        abortMission("pumping down");
        break;
      }
    }
    delay(100);
  }
  sensor.read();
  // Coasts until P.O.D is 2.5 meters deep
  // 30000
  command_start = millis();
  while ((sensor.depth() < 2.5) && !aborted_mission){
    digitalWrite(MOTOR_IN1,LOW);
    displayDepth();
    current_time = millis();
    if((current_time-last_marker)>1000){
      depth_ts = readData();
      myList.add(depth_ts);
      last_marker = current_time;
    }

    if((current_time-command_start)>30000){
      abortMission("coasting down");
      break;
    }
    previous_depth = depth;
    sensor.read();
    depth = sensor.depth();
    while(abs(depth-previous_depth)>1.0){
      sensor.read();
      depth = sensor.depth();
      current_time = millis();
      if((current_time-command_start)>30000){
        abortMission("coasting down");
        break;
      }
    }
    delay(100);   
  }  
  sensor.read();
  // Ascends until P.O.D is 1.5 meters deep
  command_start = millis();
  while ((sensor.depth() > 1.5) && !aborted_mission){
    digitalWrite(MOTOR_IN2, HIGH);
    displayDepth();
    current_time = millis();
    if((current_time-last_marker)>1000){
      depth_ts = readData();
      myList.add(depth_ts);
      last_marker = current_time;
    }

    if((current_time - command_start) > 60000){
      abortMission("pumping up");
      break;
    }
    previous_depth = depth;
    sensor.read();
    depth = sensor.depth();
    while(abs(depth-previous_depth)>1.0){
      sensor.read();
      depth = sensor.depth();
      current_time = millis();
      if((current_time-command_start)>60000){
        abortMission("pumping up");
        break;
      }
    }
    delay(100);
  }
  sensor.read();
  // Coasts until P.O.D is 0.5 meters deep
  command_start = millis();
  while ((sensor.depth() > 0.5) && !aborted_mission){
    digitalWrite(MOTOR_IN2,LOW);
    displayDepth();
    current_time = millis();
    if((current_time-last_marker)>1000){
      depth_ts = readData();
      myList.add(depth_ts);
      last_marker = current_time;
    }
    if((current_time-command_start) > 30000){
      abortMission("coasting up");
      break;
    }
    previous_depth = depth;
    sensor.read();
    depth = sensor.depth();
    while(abs(depth-previous_depth)>1.0){
      sensor.read();
      depth = sensor.depth();
      current_time = millis();
      if((current_time-command_start)>30000){
        abortMission("coasting up");
        break;
      }
    }
    delay(100);
  }
  delay(1000);
  for(int i = 0; i < 5; i++){
    depth_ts = readData();
    myList.add(depth_ts);
    delay(1000);
  }
  depth_ts = readData();
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
  profile_count += 1;
  aborted_mission = false;
  delay(1000);
  Serial.println("Successfully completed profile");
}