#include <ESP8266WiFi.h>
#include <Wire.h>
#include <BH1750FVI.h>
#include <WiFiUdp.h>
#include "CRC_16.h"

#include "Protocol.h"

static uint16_t setpoint = 0;
static uint16_t current_pos = 0;
static uint16_t sensors_data[] = { 0xABCD, 0xCDEF };

const uint8_t MY_GROUP_ID = 1;
uint32_t MY_DEVICE_ID = 0;


BH1750FVI LightSensor1;
BH1750FVI LightSensor2;

WiFiUDP udp;
IPAddress broadcastIp(192, 168, 0, 255);

const char* ssid = "N-Router";
const char* password = "adminAdmin";

boolean isWifiConnect = false;

boolean tryToConnect(){
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        udp.begin(1555);
        return true;
    }
    return false;
}

void setupLightSensors(void){
     LightSensor1.begin();
     LightSensor2.begin();
     
     Serial.println("Setup LightSensor ...");
     LightSensor2.SetAddress(Device_Address_L);//Address 0x23
     LightSensor1.SetMode(Continuous_H_resolution_Mode);
     LightSensor1.SetAddress(Device_Address_H);//Address 0x5C
     LightSensor2.SetMode(Continuous_H_resolution_Mode);

  
     // Make first mesurment
     getAndPrintLigth();
  
}

uint16_t getAndPrintLigth(void){    
     uint16_t lux1 = LightSensor1.GetLightIntensity();// Get Lux value from sensor1
     uint16_t lux2 = LightSensor2.GetLightIntensity();// And from sensor2
     Serial.print("Light1: ");
     Serial.print(lux1);
     Serial.print(" lux, Light2: ");
     Serial.print(lux2);
     Serial.println(" lux");
     return (lux1+lux2)/2;
    }


void setpoint_set (uint16_t setpoint)
{
  if (setpoint > 10000) {
    setpoint = 10000;
  }

  Serial.print ("New setpoint received: ");
  Serial.println (setpoint);
}

uint16_t setpoint_get (void)
{
  return setpoint;
}

uint16_t current_pos_get (void)
{
  return current_pos;
}


static void udp_manage (void){
  // Buffer to hold incoming packet
  static uint8_t packet[255];
  int packetSize;

  // If there's data available, read a packet
  packetSize = udp.parsePacket();

  if (packetSize) {
    // read the packet into packetBuffer
    int len = udp.read(packet, 255);
    if (len >= 0) {
      Serial.print ("RX: ");
      udp_print_packet (packet, len);
      udp_parse_packet (packet, len);
    } else {
      Serial.println("ERR: UDP read failed");
    }
  }
}

void setup(void){
     Serial.begin(115200);
     Serial.println("");
     Serial.println("Setup WiFi ...");
     WiFi.begin(ssid, password);
     Serial.println("");

     setupLightSensors();
  
     // Make first mesurment
     getAndPrintLigth();
     Serial.println("Setup Complite ...");
}



void loop(void){
     if (isWifiConnect){
         udp_manage();
     }
     else {
        isWifiConnect = tryToConnect();
     }
     
     getAndPrintLigth();
     
     delay(1000);
}
