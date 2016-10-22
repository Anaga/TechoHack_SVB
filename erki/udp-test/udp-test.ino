
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

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

typedef enum {
  CMD_READ_ALL        = 1,
  CMD_WRITE_SETPOINT  = 2
};

static uint16_t sensors_data[] = { 0xABCD, 0xCDEF };

const uint8_t MY_GROUP_ID = 1;
uint32_t MY_DEVICE_ID = 0;

// Replace with your network credentials
const char* ssid = "N-Router";
const char* password = "adminAdmin";


// ### LED ##########################################

#define PIN_LED 2
#define LED_PERIOD 200000
static int LedCounter = 0;
static int LedOn = 0;

static void led_manage (void)
{  
  if (LedCounter > 0) {
    LedCounter--;
    if ((LedCounter % LED_PERIOD) > (LED_PERIOD / 2)) {
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
  if (LedCounter < (LED_PERIOD / 2)) {
    LedCounter += LED_PERIOD;
  }
}


// ### CRC ##########################################

/** IBM CRC Lookup table. */
static const uint16_t IBMCRCTable[256]  = {
  0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
  0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
  0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
  0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
  0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
  0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
  0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
  0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
  0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
  0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
  0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
  0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
  0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
  0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
  0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
  0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
  0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
  0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
  0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
  0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
  0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
  0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
  0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
  0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
  0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
  0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
  0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
  0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
  0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
  0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
  0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
  0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};


/**
  * @brief  This function calculates CRC16-IBM on a given memory array.
  * @param  data  Pointer to data buffer.
  * @param  len   Data length.
  * @retval uint16_t CRC value.
  */
uint16_t CRC16_Calc (const uint8_t *data, int len) 
{
  /* Seed for CRC16-IBM is 0xFFFF. */
  uint16_t crc16 = 0xFFFF;
  
  for (int i = 0; i < len; i++) {
    uint8_t Index = data[i] ^ (uint8_t)crc16;
    crc16 >>= 8;
    crc16 ^= IBMCRCTable[Index];
  }
  return crc16;
}


// ### UDP ##########################################
static void udp_send_packet (const uint8_t *buf, int len);

static void send_pdu (PDU *pdu)
{
  uint8_t buf[255];

  if (pdu->data_len + 14 > sizeof (buf)) {
    Serial.println ("ERR: PDU to big to send");
    return;
  }

  buf[0]  = (pdu->transaction_id >> 8) & 0xFF;
  buf[1]  = (pdu->transaction_id >> 0) & 0xFF;
  buf[2]  = (pdu->group_id);
  buf[3]  = (pdu->source >> 24) & 0xFF;
  buf[4]  = (pdu->source >> 16) & 0xFF;
  buf[5]  = (pdu->source >> 8) & 0xFF;
  buf[6]  = (pdu->source >> 0) & 0xFF;
  buf[7]  = (pdu->dest >> 24) & 0xFF;
  buf[8]  = (pdu->dest >> 16) & 0xFF;
  buf[9]  = (pdu->dest >> 8) & 0xFF;
  buf[10] = (pdu->dest >> 0) & 0xFF;
  buf[11] = (pdu->command);
  memcpy (&buf[12], pdu->data, pdu->data_len);

  // Add CRC
  uint16_t crc = CRC16_Calc (buf, 12 + pdu->data_len);
  buf[12 + pdu->data_len] = (crc >> 8) & 0xFF;
  buf[12 + pdu->data_len + 1] = crc & 0xFF;

  Serial.print ("TX: ");  
  udp_print_packet (buf, 14 + pdu->data_len);

  udp_send_packet (buf, 14 + pdu->data_len);
}

static void handle_pdu (PDU *pdu)
{
  // Is this for my group?
  if (pdu->group_id && pdu->group_id != MY_GROUP_ID) {
    return;
  }

  // Is this for me?
  if (pdu->dest && pdu->dest != MY_DEVICE_ID) {
    return;
  }

  if (pdu->command & 0x80) {

    // This is response...

  } else {

    Serial.print ("Command ");
    Serial.print (pdu->command);
    Serial.print (" received (data_len:");
    Serial.print (pdu->data_len);
    Serial.println (")");

    // This is command - create response
    pdu->dest = pdu->source;
    pdu->source = MY_DEVICE_ID;
  
    switch (pdu->command) {
      case CMD_READ_ALL:
        pdu->data = (uint8_t *)sensors_data; // Not good :)
        pdu->data_len = sizeof (sensors_data);
        break;
      case CMD_WRITE_SETPOINT:
        break;
      default:
        break;
    }
    
    // Add response flag
    pdu->command |= 0x80;

    send_pdu (pdu);
  }
}

static void udp_send_packet (const uint8_t *buf, int len)
{
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write(buf, len);
  udp.endPacket();
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
  pdu.data_len = len - 14;

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

    led_touch();
/*
    IPAddress remoteIp = udp.remoteIP();
    int remotePort = udp.remotePort();
    Serial.print(remoteIp);
    Serial.print(":");
    Serial.print(remotePort);
    Serial.print(" - ");
*/
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
