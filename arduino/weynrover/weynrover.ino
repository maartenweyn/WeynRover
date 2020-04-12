#include "BLESerial.h"
#include <NewPing.h>

#define TRIGGER_PIN  12
#define ECHO_PIN     12
#define MAX_DISTANCE 200
#define CHECK_SONAR_INTERVAL 50
#define DISTANCE_TO_CLOSE 10


static BLESerial bleSerial;
static int speedLeft = 0;
static int speedRight = 0;
static bool isTurning = false;
unsigned long pingTimer = 0;
static bool useSonar = true;

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial2.begin(38400); // link to Sabertoot 2x12
  bleSerial.begin("Rover"); //Bluetooth device name
  pingTimer = millis() + CHECK_SONAR_INTERVAL;
}



void loop() {
    static bool bleConnected = false;
    static bool toClose = false;
    bool changeSpeed = false;
    
    if (bleSerial.connected()) {
        if (!bleConnected) {
            Serial.println("BLE Connected");
            bleConnected = true;
        }
    } else {
        if (bleConnected) {
            Serial.println("BLE Disconnected");
            bleConnected = false;
            // Stop rover if no controller is connected
            speedLeft = 0;
            speedRight = 0;
            Serial2.write(64 + speedLeft);
            delay(1);
            Serial2.write(192 + speedRight);
        }
    }

   if (useSonar) {
     if (millis() >= pingTimer) {
        pingTimer += CHECK_SONAR_INTERVAL;
        
        int distance = sonar.ping_cm();
        if (distance > 1) {
          toClose = distance < DISTANCE_TO_CLOSE;
          
          Serial.print("Ping: ");
          Serial.print(distance);
          Serial.print("cm ");
          
          Serial.println(toClose);
          if (toClose) changeSpeed = true;
        } else {
          toClose = false;
        }
      }
   }


  // put your main code here, to run repeatedly:
  if (bleSerial.available()) {
    
    byte commandType = bleSerial.read();
    Serial.print("commandType: ");
    Serial.println(commandType);
    
    if (commandType == 1) { // Direction
      char command = (char)bleSerial.read();
      switch (command) {
        case 'f': // Front
          if (isTurning) {
            speedLeft = (speedLeft + speedRight) / 2;
            speedRight = speedLeft;
            isTurning = false;
          } else {
            speedLeft += 5;
            speedRight += 5;
            if (speedLeft > 63) speedLeft = 63;
            if (speedRight > 63) speedRight = 63;
          }
          Serial.print("F: ");
          break;
        case 'b': // Back
          if (isTurning) {
            speedLeft = (speedLeft + speedRight) / 2;
            speedRight = speedLeft;
            isTurning = false;
          } else {
            speedLeft -= 5;
            speedRight -= 5;
            if (speedLeft < -63) speedLeft = -63;
            if (speedRight < -63) speedRight = -63;
          }
          
          Serial.print("B: ");
          break;
        case 'l': // Left
          speedLeft -= 5;
          speedRight += 5;
          if (speedLeft < -63) speedLeft = -63;
          if (speedRight > 63) speedRight = 63;
          isTurning = true;
          Serial.print("L: ");
          break;
        case 'r': // Right
          speedLeft += 5;
          speedRight -= 5;
          if (speedLeft > 63) speedLeft = 63;
          if (speedRight < -63) speedRight = -63;
          isTurning = true;
          Serial.print("R: ");
          break;
        case '-':
          speedLeft = 0;
          speedRight = 0;
          Serial.print("-: ");
          break;
        default:
          Serial.print("Unknown command: ");
          Serial.write(command);
          Serial.println("");
          break;
      }

       changeSpeed = true;
    } else if (commandType == 2) // options
    {
      byte value  = bleSerial.read();
      Serial.print("value 1: ");
      Serial.println(value);
    
      if (value == 1) {
        value  = bleSerial.read();
        Serial.print("value 2: ");
        Serial.println(value);
        if (value == 0) {
          useSonar = false;
          Serial.print("sonar off");
        } else {
          useSonar = true;
          Serial.print("sonar on");
        }
      }
    }
  }

  if (changeSpeed) {
    if (toClose) {
      if (speedLeft + speedRight > 0) {
        speedLeft = 0;
        speedRight = 0;
      }
    }

    Serial.print(speedLeft);
    Serial.print(" ");
    Serial.println(speedRight);

    Serial2.write(64 + speedLeft);
    delay(1);
    Serial2.write(192 + speedRight); 
  }
}
