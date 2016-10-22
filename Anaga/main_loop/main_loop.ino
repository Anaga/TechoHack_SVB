#include <ESP8266WiFi.h>
#include <Wire.h>
#include <BH1750FVI.h>
#include <WiFiUdp.h>
#include "CRC_16.h"

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

static void udp_print_packet (const uint8_t *buf, int len)
{
  char hex[5];
  for (int i = 0; i < len; i++) {
    snprintf (hex, sizeof (hex), "<%02X> ", buf[i]);
    Serial.print(hex);
  }
  Serial.println("");
}

static int udp_parse_packet (const uint8_t *buf, int len)
{
  uint16_t crc;

  if (len < 6) {
    Serial.println("ERR: UDP packet too small");
    return -1;
  }
  crc = buf[len - 2] << 8 | buf[len - 1];
  
  if (crc != CRC16_Calc (buf, len - 2)) {
    Serial.println("ERR: CRC mismatch");
    return -1;   }

  return 0;
}

static void udp_manage (void)
{
  // Buffer to hold incoming packet
  static uint8_t packetBuffer[255];
  int packetSize;

  // If there's data available, read a packet
  packetSize = udp.parsePacket();

  if (packetSize) {

    IPAddress remoteIp = udp.remoteIP();
    int remotePort = udp.remotePort();
    Serial.print(remoteIp);
    Serial.print(":");
    Serial.print(remotePort);
    Serial.print(" - ");

    // read the packet into packetBuffer
    int len = udp.read(packetBuffer, 255);
    if (len >= 0) {
      udp_print_packet (packetBuffer, len);
      udp_parse_packet (packetBuffer, len);
      packetBuffer[len] = 0;
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
