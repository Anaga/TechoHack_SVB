#include "SVB_Driver.h"
#include "Arduino.h"

//Operating board pins
// Encoder connections
#define encoderPinA  13
#define encoderPinB  12

// Mootori H-bridge
#define motorA 14
#define motorB 16

// Operating modes etc
#define mNormal 0
#define mError 9
#define mUncalibrated 1

// Safety parameters
#define overLoad 38
#define calibCurrent 30

// Variable initialization
volatile int encoderPos = 0;
volatile int wantedPos = 100;
int mode = mUncalibrated; //alustame kalibreerimisest
volatile int leftBoundary = 0;
volatile int rightBoundary = 0;

// Calculate avg motor current
const int numReadings = 10;    // <----how many samples r used
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

// Liigutamine ja tagasiside8
int motorCurrent = A0;          // Mootori voolutugevuse analoogsisend
volatile int motorSpeed = 255;           // mootori kiirus PWM-ga

//####################################

void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   */
  if (digitalRead(encoderPinA) == digitalRead(encoderPinB)) {
    encoderPos++;
  } else {
    encoderPos--;
  }
}
//####################################

void calcAverage(int lastRead){
  
  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = lastRead;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
}

//######################################

void motorRun(signed int pwm){

//Protection
  if (average > overLoad){
  motorStop();
  mode = mError;
  Serial.print("Overload protection at:");  
  Serial.println(average);
  return;
  }
  
// To the RIGHT
  if (pwm > 0){   
    digitalWrite(motorB, LOW);
    digitalWrite(motorA, pwm);
  }
// To the LEFT
  else if (pwm < 0){
    digitalWrite(motorB, pwm);
    digitalWrite(motorA, LOW);
  }
  else{
    motorStop();
  }
}

void motorStop(){
  digitalWrite(motorA, LOW);
  digitalWrite(motorB, LOW);
}
void motorBreak(){
  digitalWrite(motorA, HIGH);
  digitalWrite(motorB, HIGH);
}

//void motorStop(){
//  analogWrite(motorEna, 0);
//  digitalWrite(motorA, HIGH);
//  digitalWrite(motorB, HIGH);
//  //averageSettleDown();
//}
//void motorBreak(){
//  analogWrite(motorEna, 255);
//  digitalWrite(motorA, HIGH);
//  digitalWrite(motorB, HIGH);
//  //averageSettleDown();
//}

//###################################
void doMoveIt(){
//  int _pwm = map((encoderPos - wantedPos),1,250,80,255);

  int a = (wantedPos - encoderPos);
  Serial.print("delta: ");
  Serial.println(a);

  int abs_a = abs(a);
  Serial.print("abs_delta: ");
  Serial.println(abs_a);
  
  if (a > 20){
    motorRun(255);
    Serial.println("Going RIGHT");
  }
  else if (a < -20){
    motorRun(-255);
    Serial.println("Going LEFT");
  }    
  else if (abs_a > 10){
    motorBreak();    
  }
  else{
    motorStop();
  }
}

//#################################
void doCalibrate(){
  
  encoderPos = 0;

  Serial.println("Start calibration!");

  //left direction
  do 
  {
    motorRun(-255);
      delay(50);
    calcAverage(analogRead(motorCurrent));
    Serial.print(encoderPos);
    Serial.print(" ");
    Serial.println(average);
  } while (average < calibCurrent);
  
  encoderPos = 0;
  leftBoundary = encoderPos;
  motorStop();
  Serial.println("Left Calibrated");

  //average settle down
  averageSettleDown();
  
  Serial.println("Settled");

  //right direction
  do 
  {
    motorRun(255);
      delay(50);
    calcAverage(analogRead(motorCurrent));
      Serial.print(encoderPos);
      Serial.print(" ");
      Serial.println(average);
  } while (average < calibCurrent);
  
  rightBoundary = encoderPos;

  motorStop();
  Serial.println("Right Calibrated");

  //average settle down
  averageSettleDown();

  wantedPos = rightBoundary / (int) 2; //Go to mid position
  
  Serial.print("leftBoundary: ");
  Serial.print(leftBoundary);
  Serial.print("   rightBoundary: ");
  Serial.print(rightBoundary);
  Serial.print("   wantedPos: ");
  Serial.println(wantedPos);

  mode = mNormal;
}

void averageSettleDown(){
  do 
  {
    calcAverage(analogRead(motorCurrent));
      Serial.print(encoderPos);
      Serial.print(" ");
      Serial.println(average);
      delay(50);
  } while (average > 15);
}

void SVBdrive(){
  calcAverage(analogRead(motorCurrent));

  Serial.print(mode);
  Serial.print(" Avg: ");
  Serial.print(average);
  Serial.print(" Pos: ");
  Serial.println(encoderPos);

  switch (mode){
    case mError:
    {
      motorStop();
      delay(100);
      mode = mNormal;
    }
    break;
    case mUncalibrated:
    {
      doCalibrate();
    }
    break;
    case mNormal:
    {
      doMoveIt();
    }
    break;
  }
  delay(1);
}

void SVBsetup(){
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  pinMode(encoderPinA, INPUT); 
  pinMode(encoderPinB, INPUT);
  pinMode(motorA, OUTPUT);
  pinMode(motorB, OUTPUT); 

  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoder, CHANGE);  // encoder pin on interrupt 0 - pin 2  

}

int getSVB_WantedPosition(){
  return wantedPos;
}

int getSVB_EncoderPosition(){
  return encoderPos;
}

void setSVB_WantedPosition(int value){
  wantedPos = value;
}

void setSVB_RelativeSetpoint (uint16_t sp)
{
    int wanted = map (sp, 0, 100, leftBoundary, rightBoundary);
    setSVB_WantedPosition (wanted);
}

uint16_t getSVB_RelativeSetpoint (void)
{
    return map (getSVB_WantedPosition(), leftBoundary, rightBoundary, 0, 100);
}

uint16_t getSVB_RelativePosition (void)
{
    return map (getSVB_EncoderPosition(), leftBoundary, rightBoundary, 0, 100);    
}

