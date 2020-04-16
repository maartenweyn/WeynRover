    
/*
  WeynRover
  
  Basic control software for the an ESP32 connected to a Sabertooth motor driver and Ping Sonar Sensor

  The circuit:
  * IO12: Data pin of Parallax Sonar Sensor
  * IO15 (=UART2 TX): S1 of Sabertooth 2x12 motor driver

  April 2020
  BMaarten Weyn - Weyn.Tech

  https://github.com/maartenweyn/WeynRover

*/


#include "BLESerial.h"
#include <NewPing.h>

#define SONAR_TRIGGER_PIN  12
#define SONAR_ECHO_PIN     12 // Parallax Ping sensor is a 1 pin sonar
#define SONAR_MAX_DISTANCE 200


#define CHECK_SONAR_INTERVAL 50 // Every 50 ms we check the distance measured by the Sonar sensor
#define DISTANCE_TO_CLOSE 10    // When we are closer then 10 cm we will stop




static int speedLeft = 0;       // Speed left motor  (-63 to 63)
static int speedRight = 0;      // Speed right motor (-63 to 63)

static bool isTurning = false;  // Flag to indication if the left and right motors are not equal (hence turning)

static unsigned long pingTimer = 0;    // Timer to keep track of the next done sonar ping
static bool useSonar = true;    // Flag to indicate if we will use the sonar

static BLESerial bleSerial;     // The handle for Bluetooth Low Energy Serial service
static NewPing sonar(SONAR_TRIGGER_PIN, SONAR_ECHO_PIN, SONAR_MAX_DISTANCE); // Handle for the sonar


// Initialization
void setup() {
  Serial.begin(115200);     // Initialize serial for debugging
  Serial2.begin(38400);     // Initialize the TX UART for the Sabertoot 2x12 - Using UART2 TX, hense IO15
  
  bleSerial.begin("Rover"); //start BLE Serial with  device name
  
  pingTimer = millis() + CHECK_SONAR_INTERVAL;   // Initialize next sonar ping
}


void setRoverSpeed(int left, int right) {
  Serial.printf("New speed %d, %d\n", left, right);
      
  Serial2.write(64 + left);
  delay(1);
  Serial2.write(192 + right);
}

// Main loop
void loop() {
    static bool bleConnected = false; // persitent flag with connecton status
    static bool toClose = false;      // persistent flag to indicate if we are to close to an object and should not move forward
    
    bool changeSpeed = false; //flag to indicate if we have to change the moter speed (e.g. on an incomming new command)

    // Print if BLE connection is changed and stop rover if connection is gone
    if (bleSerial.connected()) {
        if (!bleConnected) { // if not connected
            Serial.println("BLE Connected");
            bleConnected = true;
        }
    } else {
        if (bleConnected) { // if connected
            Serial.println("BLE Disconnected");
            bleConnected = false;
            
            // Stop rover if no controller is connected
            speedLeft = 0;
            speedRight = 0;
            setRoverSpeed(speedLeft, speedRight);
        }
    }

   if (useSonar) { // if we are using the sonar
     if (millis() >= pingTimer) { // check if we need to measure 
        pingTimer += CHECK_SONAR_INTERVAL; // set new timestamp when to measure
        
        int distance = sonar.ping_cm();
        if (distance > 1) { // if range > max it  will give a range of 0, some errornous measurements of 1
          toClose = distance < DISTANCE_TO_CLOSE; // if object it to close
          
          Serial.printf("Ping: %d cm: %d\n", distance, toClose);
          
          if (toClose) changeSpeed = true; // if we are to close flag that we have to change the moter speed
        } else {
          toClose = false;
        }
      }
   }


  // if new data is comming from the BLE serial
  // format of the data
  // byte 1: datatype
  //  - 1 == direction command in byte 2
  //  - 2 == options in byte 2 and 3 (e.g. sonar on/off 
  if (bleSerial.available()) {
    byte commandType = bleSerial.read(); // read first byte
    Serial.printf("commandType: %d\n", commandType);
    
    if (commandType == 1) { // Direction command
      char command = (char) bleSerial.read(); //read direction command as second byte
      switch (command) {
        case 'f': // Front
          if (isTurning) { // if rover was turing go straight ahead again with average speed of the 2 motors
            speedLeft = (speedLeft + speedRight) / 2;
            speedRight = speedLeft;
            isTurning = false;
          } else { // otherwise increase both motor speeds
            speedLeft += 5;
            speedRight += 5;
            if (speedLeft > 63) speedLeft = 63;  // limit the range to max 63
            if (speedRight > 63) speedRight = 63;
          }
          Serial.print("F: ");
          break;
        case 'b': // Back
          if (isTurning) { // if rover was turing go straight backwards again with average speed of the 2 motors
            speedLeft = (speedLeft + speedRight) / 2;
            speedRight = speedLeft;
            isTurning = false;
          } else { // otherwise decrease both motor speeds
            speedLeft -= 5;
            speedRight -= 5;
            if (speedLeft < -63) speedLeft = -63;   // limit the range to max -63
            if (speedRight < -63) speedRight = -63;
          }
          Serial.print("B: ");
          break;
        case 'l': // Left - decrese speed left motor, increase speed of the right motor
          speedLeft -= 5;
          speedRight += 5;
          if (speedLeft < -63) speedLeft = -63;
          if (speedRight > 63) speedRight = 63;
          isTurning = true;
          Serial.print("L: ");
          break;
        case 'r': // Right - decrese speed right motor, increase speed of the left motor
          speedLeft += 5;
          speedRight -= 5;
          if (speedLeft > 63) speedLeft = 63;
          if (speedRight < -63) speedRight = -63;
          isTurning = true;
          Serial.print("R: ");
          break;
        case '-': // stop
          speedLeft = 0;
          speedRight = 0;
          Serial.print("-: ");
          break;
        default:
          Serial.printf("Unknown command: %s\n", command);
          break;
      }

       changeSpeed = true; // set flag to update motor speed
    } else if (commandType == 2) {// options
      byte value  = bleSerial.read();
      Serial.printf("value 1: %d\n", value);
    
      if (value == 1) { // sonar on/off
        value  = bleSerial.read();
        Serial.printf("value 2: %d\n", value);
        if (value == 0) {
          useSonar = false;
          Serial.println("sonar off");
        } else {
          useSonar = true;
          Serial.println("sonar on");
        }
      }
    }
  }

  if (changeSpeed) { // if motor speed has to be changed
    if (toClose) { // if to close
      if (speedLeft + speedRight > 0) { // only stop motors if we are going forward
        speedLeft = 0;
        speedRight = 0;
      }
    }
    setRoverSpeed (speedLeft, speedRight);
  }
}
