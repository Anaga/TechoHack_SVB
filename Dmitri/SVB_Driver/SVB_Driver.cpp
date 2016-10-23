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
#define mIdle 5

// Safety parameters (mall motor)
#define small 0
#define overLoad 38
#define calibCurrentX 28
#define calibCurrentY 28
const int smallNumReadings = 10;    // <----how many samples r 
int smallKeepSetpoint = false;

// Safety parameters (nig motor)
#define big 1
#define bigOverLoad 80
#define bigCalibCurrentX 28
#define bigCalibCurrentY 28
const int bigNumReadings = 10;    // <----how many samples r 
int bigKeepSetpoint = false;

// Variable initialization
volatile int encoderPos = 0;
volatile int wantedPos = 0;
int mode = mUncalibrated; //alustame kalibreerimisest
volatile int leftBoundary = 0;
volatile int rightBoundary = 0;
int keepSetpoint = false;

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

void motorRun(signed int pwm, int motorNumber){

//Protection
  if (motorNumber == small){
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
  else if (motorNumber == big){
    if (average > bigOverLoad){
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
void doMoveIt(int motorNumber){
//  int _pwm = map((encoderPos - wantedPos),1,250,80,255);

  int a = (wantedPos - encoderPos);
  Serial.print("delta: ");
  Serial.println(a);

  int abs_a = abs(a);
  Serial.print("abs_delta: ");
  Serial.println(abs_a);
  
  if (a > 20){
    motorRun(255, motorNumber);
    Serial.println("Going RIGHT");
  }
  else if (a < -20){
    motorRun(-255, motorNumber);
    Serial.println("Going LEFT");
  }    
  else if (abs_a > 10){
    motorBreak();    
  }
  else{
    motorStop();
    if (!keepSetpoint) {
        mode = mIdle;
    }
  }
}

//#################################
void doCalibrate(int motorNumber){
  if (motorNumber == small){
    encoderPos = 0;

  Serial.println("Start calibration!");

  //left direction
  do 
  {
    motorRun(-255, motorNumber);
      delay(50);
    calcAverage(analogRead(motorCurrent));
    Serial.print(encoderPos);
    Serial.print(" ");
    Serial.println(average);
  } while (average < calibCurrentX);
  
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
    motorRun(255, motorNumber);
      delay(50);
    calcAverage(analogRead(motorCurrent));
      Serial.print(encoderPos);
      Serial.print(" ");
      Serial.println(average);
  } while (average < calibCurrentY);
  
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
  else if (motorNumber == big){
    encoderPos = 0;

  Serial.println("Start calibration!");

  //left direction
  do 
  {
    motorRun(-255, motorNumber);
      delay(50);
    calcAverage(analogRead(motorCurrent));
    Serial.print(encoderPos);
    Serial.print(" ");
    Serial.println(average);
  } while (average < bigCalibCurrentX);
  
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
    motorRun(255, motorNumber);
      delay(50);
    calcAverage(analogRead(motorCurrent));
      Serial.print(encoderPos);
      Serial.print(" ");
      Serial.println(average);
  } while (average < bigCalibCurrentY);
  
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

void SVBdrive(int motorNumber){
  calcAverage(analogRead(motorCurrent));

  Serial.print("Mode: ");
  Serial.print(mode);
  Serial.print("Motor: ");
  if(motorNumber == small){
    Serial.print("small");
  } else if (motorNumber == big){
    Serial.print("big");
  }
  Serial.print(motorNumber);
  Serial.print(" Avg: ");
  Serial.print(average);
  Serial.print(" Pos: ");
  Serial.println(encoderPos);

  if(motorNumber == small){
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
      doCalibrate(motorNumber);
    }
    break;
    case mNormal:
    {
      doMoveIt(motorNumber);
      }
    break;
    case mIdle:
    {
    }
    break;
    }
  delay(1);
  } else if (motorNumber == big){
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
      //doCalibrate(motorNumber);
      leftBoundary = 0;
      rightBoundary = 4400;
      wantedPos = rightBoundary / (int) 2;
      mode = mNormal;
    }
    break;
    case mNormal:
    {
      doMoveIt(motorNumber);
    }
    break;
    case mIdle:
    {
    }
    break;
  }
  delay(1);
  }
  
}

void SVBsetup(int motorNumber){
  if (motorNumber == big){
    for (int thisReading = 0; thisReading < bigNumReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  } 
  else if (motorNumber == small){
    for (int thisReading = 0; thisReading < smallNumReadings; thisReading++) {
    readings[thisReading] = 0;
  }
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
    if (mode == mIdle) {
        mode = mNormal;
    }
}

uint16_t getSVB_RelativeSetpoint (void)
{
    return map (getSVB_WantedPosition(), leftBoundary, rightBoundary, 0, 100);
}

uint16_t getSVB_RelativePosition (void)
{
    return map (getSVB_EncoderPosition(), leftBoundary, rightBoundary, 0, 100);    
}

int getSVB_Mode (void)
{
    return mode;
}



