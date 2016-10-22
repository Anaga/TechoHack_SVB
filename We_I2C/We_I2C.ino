/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  


    This is a simple code to test BH1750FVI Light senosr
  communicate using I2C Protocol 
  this library enable 2 slave device address
  Main address  0x23 
  secondary address 0x5C 
  connect this sensor as following :
  VCC >>> 3.3V
  SDA >>> A4 
  SCL >>> A5
  addr >> A3
  Gnd >>>Gnd

  Written By : Mohannad Rawashdeh
*********/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <BH1750FVI.h>


MDNSResponder mdns;

BH1750FVI LightSensor;


// Replace with your network credentials
const char* ssid = "N-Router";
const char* password = "adminAdmin";

ESP8266WebServer server(80);

String webPage = "";

//int gpio0_pin = 0;
int gpio2_pin = 2;

#include <Servo.h>

Servo myservo;


void setup(void){
  webPage += "<h1>ESP8266 Web Server</h1><p>Socket #1 <a href=\"socket1On\"><button>ON</button></a>&nbsp;<a href=\"socket1Off\"><button>OFF</button></a></p>";
  webPage += "<p>Socket #2 <a href=\"socket2On\"><button>ON</button></a>&nbsp;<a href=\"socket2Off\"><button>OFF</button></a></p>";
  
  // preparing GPIOs
//  pinMode(gpio0_pin, OUTPUT);
//  digitalWrite(gpio0_pin, LOW);
  pinMode(gpio2_pin, OUTPUT);
  digitalWrite(gpio2_pin, LOW);
  LightSensor.begin();
  delay(1000);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  myservo.attach(4);

  LightSensor.SetAddress(Device_Address_H);//Address 0x5C
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  LightSensor.SetMode(Continuous_H_resolution_Mode);
  
  Serial.println("Running...");
  
  server.on("/", [](){
    server.send(200, "text/html", webPage);
  });
  server.on("/socket1On", [](){
    server.send(200, "text/html", webPage);
//    digitalWrite(gpio0_pin, HIGH);
    myservo.write(180);
    delay(100);
  });
  server.on("/socket1Off", [](){
    server.send(200, "text/html", webPage);
//    digitalWrite(gpio0_pin, LOW);
    myservo.write(0);
    delay(100); 
  });
  server.on("/socket2On", [](){
    server.send(200, "text/html", webPage);
    digitalWrite(gpio2_pin, HIGH);
    delay(100);
  });
  server.on("/socket2Off", [](){
    server.send(200, "text/html", webPage);
    digitalWrite(gpio2_pin, LOW);
    delay(100); 
  });
  server.begin();
  Serial.println("HTTP server started");
}
 
void loop(void){
  server.handleClient();
    uint16_t lux = LightSensor.GetLightIntensity();// Get Lux value
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lux");
    delay(1000);
  
} 
