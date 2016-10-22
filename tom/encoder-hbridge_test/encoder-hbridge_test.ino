
#define encoderPinA  13
#define encoderPinB  12
#define motorA 14
#define motorB 16

#define overLoad 40
#define calibCurrent 18

volatile int encoderPos = 0;
volatile int wantedPos = 100;
int currentCommand = 0;

int keyPress = A0;       // klahvide analoogsisend

// Keskmistamine
const int numReadings = 10;    // üle mitme sample mootori voolu keskmistatakse
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

int motorCurrent = A0;          // Mootori voolutugevuse analoogsisend
int motorSpeed = 255;           // mootori kiirus PWM-ga

    // Mootori H-bridge



void setup() {
   Serial.begin(115200);
   Serial.println("Let's Go!");

  // initsialiseerime massiivi
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  pinMode(encoderPinA, INPUT); 
  pinMode(encoderPinB, INPUT); 
  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoder, CHANGE);  // encoder pin on interrupt 0 - pin 2
  pinMode(motorA, OUTPUT);
  pinMode(motorB, OUTPUT);

}

void loop() {

motorRun(255);
calcAverage(analogRead(motorCurrent));
Serial.print("Motorcurrent: ");
Serial.print(motorCurrent);
Serial.print("  EncoderPos: ");
Serial.println(encoderPos);
delay(500);
motorStop();
calcAverage(analogRead(motorCurrent));
Serial.print("Motorcurrent: ");
Serial.print(motorCurrent);
Serial.print("  EncoderPos: ");
Serial.println(encoderPos);
delay(500);
motorRun(-255);
calcAverage(analogRead(motorCurrent));
Serial.print("Motorcurrent: ");
Serial.print(motorCurrent);
Serial.print("  EncoderPos: ");
Serial.println(encoderPos);
delay(500);
motorStop();
calcAverage(analogRead(motorCurrent));
Serial.print("Motorcurrent: ");
Serial.print(motorCurrent);
Serial.print("  EncoderPos: ");
Serial.println(encoderPos);
delay(500);


    delay(1); 
}

void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   *
   * For more information on speeding up this process, see
   * [Reference/PortManipulation], specifically the PIND register.
   */
  if (digitalRead(encoderPinA) == digitalRead(encoderPinB)) {
    encoderPos++;
  } else {
    encoderPos--;
  }

}

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

void motorRun(signed int pwm){

//Protection
  if (average > overLoad){
  motorStop();
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
