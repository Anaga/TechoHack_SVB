#include "Arduino.h"
#include "SVB_Driver.h"


//####################################
//####################################
void setup() {
    Serial.begin(115200);
    Serial.println("Let's Go!");

  SVBsetup();
}

//####################################
//####################################

int dd = 0;

void loop() { 
  SVBdrive(); 
  delay(25);
  if (abs(getSVB_EncoderPosition() - getSVB_WantedPosition()) < 20){
    Serial.print("Delta: ");
    Serial.println(abs(getSVB_EncoderPosition() - getSVB_WantedPosition()));
    if (dd == 0){
      setSVB_WantedPosition(getSVB_WantedPosition() + (int) 200);
      Serial.print("New wanted: ");
      Serial.println(getSVB_WantedPosition());
      dd = 1;      
    }
    else if (dd == 1){
      setSVB_WantedPosition(getSVB_WantedPosition() - (int) 400);
      Serial.print("New wanted: ");
      Serial.println(getSVB_WantedPosition());
      dd = 2;      
    }
    else if (dd == 2){
      setSVB_WantedPosition(getSVB_WantedPosition() + (int) 200);
      Serial.print("New wanted: ");
      Serial.println(getSVB_WantedPosition());
      dd = 0;      
    }
  }
}

