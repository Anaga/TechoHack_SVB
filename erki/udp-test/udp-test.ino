
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

WiFiUDP udp;
IPAddress broadcastIp(192, 168, 0, 255);

// Replace with your network credentials
const char* ssid = "N-Router";
const char* password = "adminAdmin";


// ### LED ##########################################

#define PIN_LED 2

static int LedCounter = 0;
static int LedOn = 0;

static void led_manage (void)
{  
  if (LedCounter > 0) {
    LedCounter--;
    if ((LedCounter % 200000) > 100000) {
      if (!LedOn) {
        digitalWrite(PIN_LED, LOW);
        LedOn = 1;
      }
    } else {
      if (LedOn) {
        digitalWrite(PIN_LED, HIGH);
        LedOn = 0;
      }
    }
  }
}

static void led_touch (void)
{
  if (LedCounter < 100000) {
    LedCounter += 200000;
  }
}


// ### UDP ##########################################

static void udp_manage (void)
{
  // Buffer to hold incoming packet
  static char packetBuffer[255];
  int packetSize;

  // If there's data available, read a packet
  packetSize = udp.parsePacket();

  if (packetSize) {

    led_touch();

    IPAddress remoteIp = udp.remoteIP();
    int remotePort = udp.remotePort();
    Serial.print(remoteIp);
    Serial.print(":");
    Serial.print(remotePort);
    Serial.print(" - ");

    // read the packet into packetBuffer
    int len = udp.read(packetBuffer, 255);
    if (len >= 0) {
      packetBuffer[len] = 0;
    } else {
      packetBuffer[0] = 0;
    }

    Serial.print(packetBuffer);

    if (packetBuffer[len - 1] != '\n') {
      Serial.println("");
    }

/*
    udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    udp.write(ReplyBuffer);
    udp.endPacket();
*/
  }

}

// ### Arduino ######################################

void setup() 
{
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  
  delay(1000);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  int toggle = 1;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    if (toggle) {
      digitalWrite(PIN_LED, LOW);
    } else {
      digitalWrite(PIN_LED, HIGH);
    }
    toggle = !toggle;
    Serial.print(".");
  }
  digitalWrite(PIN_LED, HIGH);

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  udp.begin(1555);
}

void loop() 
{
  led_manage();
  udp_manage();
}
