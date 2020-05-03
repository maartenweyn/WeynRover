    
/*
  WeynRover Controller
  
  Basic control software a generic controller with buttons

  The circuit:
  * all pins in button_pins are connected to a buttons which are connected to the ground through a resistor

  April 2020
  Maarten Weyn - Weyn.Tech

  https://github.com/maartenweyn/WeynRover

*/


#include <BLEDevice.h> 

#define NR_OF_BUTTONS 8
const int button_pins[NR_OF_BUTTONS] = { 25, 26, 27, 16, 13,  4,  5, 2};
const int button_ids[NR_OF_BUTTONS]  = {  0,  1,  2,  3, 10, 11, 12, 13};

int buttonState[NR_OF_BUTTONS] = {0}; 

///BLE related

static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID    charUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
String My_BLE_Name = "Rover"; 
static BLEAddress *Server_BLE_Address = NULL;
static BLERemoteCharacteristic* pRemoteCharacteristic;


#define SEND_PING_NTERVAL 500

BLEScan* pBLEScan; 
boolean paired = false;

unsigned long pingTimer = 0;


// Connecto the the Rover BLESerial
bool connectToserver (BLEAddress pAddress){
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    // Connect to the BLE Server.
    pClient->connect(pAddress);
    Serial.println(" - Connected to rover");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService != nullptr)
    {
      Serial.println(" - Found our service");
    }
    else {
      return false;
    }

    std::map<std::string, BLERemoteCharacteristic*>* pCharacteristics = pRemoteService-> getCharacteristics();

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic != nullptr) {
      Serial.println(" - Found our characteristic");
      return true;
    } else {
      Serial.println(" - Cannot find characteristic");
      return true;
    }
}

// Get the Address of the Rover
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Scan Result: %s \n", advertisedDevice.toString().c_str());

      Serial.printf("Scan Result Name: %s \n", advertisedDevice.getName().c_str());
      if (strcmp(advertisedDevice.getName().c_str(), My_BLE_Name.c_str()) == 0) {
        Server_BLE_Address = new BLEAddress(advertisedDevice.getAddress());
      }
    }
};

void setup() {
  Serial.begin(115200);
  
  for (int i = 0; i<NR_OF_BUTTONS; i++)
    pinMode(button_pins[i], INPUT_PULLUP);


  // Scan for the rover
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); //Call the class that is defined above 
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(100);  
  
  while (!ble_loop()) {}; // do not go to the andruino loop if not connected to the rover

  // ping timer is used to send a heartbeat message to the rover
  pingTimer = millis() + SEND_PING_NTERVAL;
}


// Method to scan and find the rover's BLESerial
bool ble_loop() {
  Server_BLE_Address = NULL;
  BLEScanResults foundDevices = pBLEScan->start(3); //Scan for 3 seconds to find the Fitness band 

  if (foundDevices.getCount() >= 1)
  {
    if (Server_BLE_Address && !paired)
    {
      Serial.println("Found Device :-)... connecting to Server as client");
      if (connectToserver(*Server_BLE_Address))
      {
        paired = true;
        Serial.println("Paired");
      }
      else
      {
        Serial.println("Pairing failed");
      }
    } else if (!Server_BLE_Address && paired)
    {
      Serial.println("Our device went out of range");
      paired = false;
      ESP.restart();
    }
    else
    {
      Serial.println("Device not found");
      paired = false;
    }
  } 

  return paired;
}

void loop() {

  static byte command[2] = {3, 0};

  // Check if buttons are pressed
  for (int i = 0; i<NR_OF_BUTTONS; i++) {    
    int state = digitalRead(button_pins[i]);

    // If pressed send to BLE and delay the heartbeat
    if (!state && buttonState[i]) {
      Serial.print("Button ");
      Serial.print(button_ids[i]);
      Serial.println(" pressed");


      command[1] = button_ids[i];
      pRemoteCharacteristic->writeValue(command, 2);
      pingTimer = millis() + SEND_PING_NTERVAL;
      
    }

    buttonState[i] = state;
  }

  // if heartbeat interval ellapsed, send heartbeat
  if (millis() >= pingTimer) {
       pingTimer += SEND_PING_NTERVAL;
       byte pingcommand[2] = {4, 0};
        
      pRemoteCharacteristic->writeValue(pingcommand, 2);
  }

  delay(100);
}
