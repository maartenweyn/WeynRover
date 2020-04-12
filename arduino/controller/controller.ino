#include "BLESerial.h"

#define NR_OF_BUTTONS 8
const int button_pins[NR_OF_BUTTONS] = { 25, 26, 27, 16, 13,  4,  5, 2};
const int button_ids[NR_OF_BUTTONS]  = {  0,  1,  2,  3, 10, 11, 12, 13};

int buttonState[NR_OF_BUTTONS] = {0}; 

static BLESerial bleSerial;

void setup() {
  Serial.begin(115200);
  
  for (int i = 0; i<NR_OF_BUTTONS; i++)
    pinMode(button_pins[i], INPUT_PULLUP);

   
  bleSerial.begin("RoverController");
}

  
void loop() {
  for (int i = 0; i<NR_OF_BUTTONS; i++) {    
    int state = digitalRead(button_pins[i]);

    if (!state && buttonState[i]) {
      Serial.print("Button ");
      Serial.print(button_ids[i]);
      Serial.println(" pressed");
    }

    buttonState[i] = state;
  }

  delay(100);
}
