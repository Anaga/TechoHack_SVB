#include <ESP8266WiFi.h>
#include <Wire.h>
#include <BH1750FVI.h>
#include <WiFiUdp.h>
#include "CRC_16.h"

BH1750FVI LightSensor1;
BH1750FVI LightSensor2;


WiFiUDP udp;
IPAddress broadcastIp(192, 168, 0, 255);

typedef struct
{
  uint16_t transaction_id;
  uint8_t group_id;
  uint32_t source;
  uint32_t dest;
  uint8_t command;
  int data_len;
  const uint8_t *data;
} PDU;

#define PDU_HDR_AND_CRC_LEN 14

typedef enum {
  CMD_READ_SENSORS    = 1,
  CMD_WRITE_SETPOINT  = 2,
  CMD_READ_SETPOINT   = 3,
  CMD_READ_POSITION   = 4,
} CMDS;

typedef enum {
  CMD_ST_OK   = 0,
  CMD_ST_ERR  = 1,
} CMD_STATUS;

const uint8_t MY_GROUP_ID = 1;
uint32_t MY_DEVICE_ID = 0;


const char* ssid = "N-Router";
const char* password = "adminAdmin";

boolean isWifiConnect = false;

int hexdec (char hex)
{
  if (hex >= '0' && hex <= '9') {
    return hex - '0';
  } else if (hex >= 'A' && hex <= 'F') {
    return hex - 'A' + 10;
  } else if (hex >= 'a' && hex <= 'f') {
    return hex - 'a' + 10;
  } else {
    return 0;
  }
}

boolean tryToConnect(){
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("MAC address: ");
        Serial.println(WiFi.macAddress());
      
        char mac[18];
        WiFi.macAddress().toCharArray (mac, sizeof(mac));
        MY_DEVICE_ID = ((hexdec(mac[9]) << 20) | (hexdec(mac[10]) << 16) | (hexdec(mac[12]) << 12) | (hexdec(mac[13]) << 8) | (hexdec(mac[15]) << 4) | (hexdec(mac[16]) << 0));
      
        Serial.print("Device ID: ");
        Serial.println(MY_DEVICE_ID);

        udp.begin(1555);
        return true;
    }
    return false;
}

void setupLightSensors(void){
     LightSensor1.begin();
     LightSensor2.begin();
     
     Serial.println("Setup LightSensor ...");
     LightSensor1.SetAddress(Device_Address_L);//Address 0x23
     LightSensor1.SetMode(Continuous_H_resolution_Mode);

     LightSensor2.SetAddress(Device_Address_H);//Address 0x5C
     LightSensor2.SetMode(Continuous_H_resolution_Mode);

  
     // Make first mesurment
     getAndPrintLigth();
  
}


// ### DUMMIES ######################################
static uint16_t setpoint = 0;
static uint16_t current_pos = 0;
static uint16_t sensors_data[] = { 0xABCD, 0xCDEF };
static uint16_t priv_lux1 = 0;
static uint16_t priv_lux2 = 0;



uint16_t getAndPrintLigth(void){    
     priv_lux1 = LightSensor1.GetLightIntensity();// Get Lux value from sensor1
     priv_lux2 = LightSensor2.GetLightIntensity();// And from sensor2
     Serial.print("Light1: ");
     Serial.print(priv_lux1);
     Serial.print(" lux, Light2: ");
     Serial.print(priv_lux2);
     Serial.println(" lux");
     return (priv_lux1+priv_lux2)/2;
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


// ### UDP ##########################################
static void udp_send_packet (const uint8_t *buf, int len);
static void udp_print_packet (const uint8_t *buf, int len);

/**
  * @brief Send protocol packet
  */
static void send_pdu (PDU *pdu)
{
  uint8_t buf[255];
  uint8_t *bufptr = buf;
  
  if ((PDU_HDR_AND_CRC_LEN + pdu->data_len) > sizeof (buf)) {
    Serial.println ("ERR: PDU to big to send");
    return;
  }

  *bufptr++ = (pdu->transaction_id >> 8) & 0xFF;
  *bufptr++ = (pdu->transaction_id >> 0) & 0xFF;
  *bufptr++ = (pdu->group_id);
  *bufptr++ = (pdu->source >> 24) & 0xFF;
  *bufptr++ = (pdu->source >> 16) & 0xFF;
  *bufptr++ = (pdu->source >> 8) & 0xFF;
  *bufptr++ = (pdu->source >> 0) & 0xFF;
  *bufptr++ = (pdu->dest >> 24) & 0xFF;
  *bufptr++ = (pdu->dest >> 16) & 0xFF;
  *bufptr++ = (pdu->dest >> 8) & 0xFF;
  *bufptr++ = (pdu->dest >> 0) & 0xFF;
  *bufptr++ = (pdu->command);
  memcpy (bufptr, pdu->data, pdu->data_len);
  bufptr += pdu->data_len;

  // Add CRC
  uint16_t crc = CRC16_Calc (buf, bufptr - buf);
  *bufptr++ = (crc >> 8) & 0xFF;
  *bufptr++ = crc & 0xFF;

  Serial.print ("TX: ");  
  udp_print_packet (buf, bufptr - buf);

  udp_send_packet (buf, bufptr - buf);
}


/**
  * @brief  Handle protocol packet
  */
static void handle_pdu (PDU *pdu)
{
  uint8_t databuf[32];
  uint8_t *dataptr = databuf;

  // Is this for my group?
  if (pdu->group_id != -1 && pdu->group_id != MY_GROUP_ID) {
    return;
  }

  // Is this for me?
  if (pdu->dest != -1 && pdu->dest != MY_DEVICE_ID) {
    return;
  }

  // Source must be defined
  if (pdu->source == -1) {
    return;
  }

  char logbuf[128];
  snprintf (logbuf, sizeof (logbuf), "TrID: %d, GID: %d, %d => %d, 0x%02X, data_len: %d", pdu->transaction_id, pdu->group_id, pdu->source, pdu->dest, pdu->command, pdu->data_len);
  Serial.println (logbuf);

  if (pdu->command & 0x80) {

    // This is a response

    if (pdu->data_len < 1) {
      return;
    }
    if (pdu->data[0] != CMD_ST_OK) {
      return;
    }

    switch (pdu->command ^ 0x80) {

      case CMD_READ_SENSORS: {
        Serial.println ("Got CMD_READ_SENSORS responce from other unit ");
        if (pdu->data_len % 2) {
          for (int i = 0; i < (pdu->data_len - 1) / 2; i++) {
            uint16_t val = pdu->data[i * 2 + 1] << 8 | pdu->data[i * 2 + 2];
            Serial.print ("Got value ");
            Serial.println (val);
          }
        }
      }
      break;

      case CMD_READ_SETPOINT: {
        uint16_t sp;
        if (pdu->data_len == 3) {
          sp = (pdu->data[1] << 8) | pdu->data[2];
        }
      }
      break;

      case CMD_READ_POSITION: {
        uint16_t pos;
        if (pdu->data_len == 3) {
          pos = (pdu->data[1] << 8) | pdu->data[2];
        }
      }
      break;
    }
    
  } else {

    // This is a command

    Serial.print ("Command ");
    Serial.print (pdu->command);
    Serial.print (" received (data_len:");
    Serial.print (pdu->data_len);
    Serial.println (")");

    switch (pdu->command) {
      case CMD_READ_SENSORS: {
        *dataptr++ = CMD_ST_OK;

        Serial.print ("CMD_READ_SENSORS, ligth1 value is ");
        Serial.print (priv_lux1);
        Serial.print (" CMD_READ_SENSORS, ligt21 value is ");
        Serial.println (priv_lux2);

        *dataptr++ = (priv_lux1 >> 8) & 0xFF;
        *dataptr++ = (priv_lux1 >> 0) & 0xFF;
        *dataptr++ = (priv_lux2 >> 8) & 0xFF;
        *dataptr++ = (priv_lux2 >> 0) & 0xFF;
        pdu->data_len = 4 + 1;
      }
      break;

      case CMD_WRITE_SETPOINT: {
        if (pdu->data_len == 2) {
          uint16_t sp = (pdu->data[0] << 8 | pdu->data[1]);
          setpoint_set (sp);
          *dataptr++ = CMD_ST_OK;
        } else {
          *dataptr++ = CMD_ST_ERR;
        }
        pdu->data_len = 1;
      }
      break;

      case CMD_READ_SETPOINT: {
          uint16_t sp = setpoint_get ();
          *dataptr++ = CMD_ST_OK;
          *dataptr++ = (sp >> 8) & 0xFF;
          *dataptr++ = (sp >> 0) & 0xFF;
          pdu->data_len = 3;
      }
      break;

      case CMD_READ_POSITION: {
        uint16_t pos = current_pos_get ();
        *dataptr++ = CMD_ST_OK;
        *dataptr++ = (pos >> 8) & 0xFF;
        *dataptr++ = (pos >> 0) & 0xFF;
        pdu->data_len = 3;
      }
      break;

      default:
        break;
    }

    // Switch data buffer
    pdu->data = databuf;

    // Add response flag
    pdu->command |= 0x80;

    // Swap source and destination
    pdu->dest = pdu->source;
    pdu->source = MY_DEVICE_ID;

    // And out we go...
    send_pdu (pdu);
  }
}

/**
  * @brief Send UDP packet
  */
static void udp_send_packet (const uint8_t *buf, int len)
{
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write(buf, len);
  udp.endPacket();
}

/**
  * @brief  Print out packet for debugging
  */
static void udp_print_packet (const uint8_t *buf, int len)
{
  char hex[5];
  for (int i = 0; i < len; i++) {
    snprintf (hex, sizeof (hex), "<%02X> ", buf[i]);
    Serial.print(hex);
  }
  Serial.println("");
}

/**
  * @brief Validate and handle incoming packet
  */
static int udp_parse_packet (const uint8_t *buf, int len)
{
  uint16_t crc;
  PDU pdu;

  if (len < 6) {
    Serial.println("ERR: UDP packet too small");
    return -1;
  }

  crc = buf[len - 2] << 8 | buf[len - 1];

  if (crc != CRC16_Calc (buf, len - 2)) {
    Serial.println("ERR: CRC mismatch");
    return -1;
  }

  pdu.transaction_id = ((buf[0] << 8) | buf[1]);
  pdu.group_id = buf[2];
  pdu.source = (buf[3] << 24 | buf[4] << 16 | buf[5] << 8 | buf[6]);
  pdu.dest = (buf[7] << 24 | buf[8] << 16 | buf[9] << 8 | buf[10]);
  pdu.command = buf[11];
  pdu.data = &buf[12];
  pdu.data_len = len - PDU_HDR_AND_CRC_LEN;

  handle_pdu (&pdu);

  return 0;
}

/**
  * @brief  Check for incoming UDP packet
  */
static void udp_manage (void)
{
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
