

#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(10);

  // GPIO14 as IR transp
  pinMode(14, OUTPUT);
  digitalWrite(14, 0);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Start up ");
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(14, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);              // wait for a second
  digitalWrite(14, LOW);    // turn the LED off by making the voltage LOW
  delay(100);              // wait for a second

  int buttonState = digitalRead(15);
  // print out the state of the button:
  Serial.println(buttonState);

}
