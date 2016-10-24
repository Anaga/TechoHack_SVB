#include <ESP8266WiFi.h>
#include <Wire.h>
#include <BH1750FVI.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <SVB_Driver.h>

#include "CRC_16.h"
#include "types.h"
#include "protocol.h"

BH1750FVI LightSensor1;
BH1750FVI LightSensor2;
Ticker LightSenseTicker;
Ticker MasterLightRequestTicker;
Ticker MasterSetpointTicker;
Ticker MasterSetpointRateLimitTicker;

// Light sense interval in seconds
#define LIGHT_SENSE_INTERVAL 1.0

// Light sense interval in seconds
#define MASTER_REQEST_INTERVAL 5.0
#define MASTER_SETPOINT_INTERVAL 1.0
#define MASTER_SETPOINT_RATELIMIT 0.2

#define IM_MASTER 1

WiFiUDP udp;
IPAddress broadcastIp(192, 168, 0, 255);
#define UDP_PORT 1555

#define PDU_HDR_AND_CRC_LEN 14

#define LIGHT_GEST 300

const uint8_t MY_GROUP_ID = 1;
uint32_t MY_DEVICE_ID = 0;

const char* ssid = "SVB";
const char* password = "12345678";
boolean doLightSense = false;
boolean doRequestLight = false;
boolean doBroadcastSetpoint = false;
boolean allowBroadcastSetpoint = true;
boolean isWifiConnect = false;

// ### Interenal Unit varaibles ################
static uint16_t priv_setpoint = 0;
static uint16_t priv_current_pos = 0;

static uint16_t priv_lux1 = 0;
static uint16_t priv_lux2 = 0;

static int priv_svb_step = 0;
// ### Interenal Unit values #####

void send_command_open_all();

void send_command_close_all();

int hexdec (char hex){
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
    getAndPrintLight();
}

uint16_t getAndPrintLight(void){
    priv_lux1 = LightSensor1.GetLightIntensity();// Get Lux value from sensor1
    priv_lux2 = LightSensor2.GetLightIntensity();// And from sensor2
    Serial.print("Light1: ");
    Serial.print(priv_lux1);
    Serial.print(" lux, Light2: ");
    Serial.print(priv_lux2);
    Serial.println(" lux");
    return (priv_lux1+priv_lux2)/2;
}

void triggerLightSense (void){
    doLightSense = true;
}

void triggerMasterLightRequest (void){
    doRequestLight = true;
}

void triggerBroadcastSetpoint (void){
    doBroadcastSetpoint = true;
}

void ratelimitBroadcastSetpoint (void){
    allowBroadcastSetpoint = true;
}

void setpoint_set (uint16_t sp){
    Serial.print ("New setpoint received: ");
    Serial.println (sp);

    if (sp > 10000) {
        priv_setpoint = 10000;
    }
    else {
        priv_setpoint = sp;
    }

    Serial.print ("New setpoint: ");
    Serial.println (priv_setpoint);
    setSVB_RelativeSetpoint (priv_setpoint / 100);

    Serial.print ("Mode: ");
    Serial.println (getSVB_Mode());
}

uint16_t setpoint_get (void){
    return getSVB_RelativeSetpoint () * 100;
}

uint16_t current_pos_get (void){
    return getSVB_RelativePosition () * 100;
}


// ### UDP ##########################################
static void udp_send_packet (const uint8_t *buf, int len, boolean is_broadcast);
static void udp_print_packet (const uint8_t *buf, int len);

/**
  * @brief Send protocol packet
  */
static void send_pdu (PDU *pdu, boolean is_broadcast)
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
    if (pdu->data) {
        memcpy (bufptr, pdu->data, pdu->data_len);
    }
    bufptr += pdu->data_len;

    // Add CRC
    uint16_t crc = CRC16_Calc (buf, bufptr - buf);
    *bufptr++ = (crc >> 8) & 0xFF;
    *bufptr++ = crc & 0xFF;

    Serial.print ("TX: ");
    udp_print_packet (buf, bufptr - buf);

    udp_send_packet (buf, bufptr - buf, is_broadcast);
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
            Serial.println ("Got CMD_READ_SENSORS response from other unit ");
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
            uint16_t sp;
            Serial.print ("CMD_WRITE_SETPOINT ");
            if (pdu->data_len == 2) {
                sp = (pdu->data[0] << 8 | pdu->data[1]);
                setpoint_set (sp);
                *dataptr++ = CMD_ST_OK;
            } else {
                *dataptr++ = CMD_ST_ERR;
                Serial.print ("CMD_WRITE_SETPOINT, data len not 2!");
            }
            Serial.println (sp);
            pdu->data_len = 1;
        }
            break;

        case CMD_READ_SETPOINT: {
            Serial.print ("CMD_READ_SETPOINT ");
            uint16_t sp = setpoint_get ();
            *dataptr++ = CMD_ST_OK;
            *dataptr++ = (sp >> 8) & 0xFF;
            *dataptr++ = (sp >> 0) & 0xFF;
            pdu->data_len = 3;
            Serial.println (sp);
        }
            break;

        case CMD_READ_POSITION: {
            Serial.print ("CMD_READ_POSITION ");
            uint16_t pos = current_pos_get ();
            *dataptr++ = CMD_ST_OK;
            *dataptr++ = (pos >> 8) & 0xFF;
            *dataptr++ = (pos >> 0) & 0xFF;
            Serial.println (pos);
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
        send_pdu (pdu, false);
    }
}

/**
  * @brief Send UDP packet
  */
static void udp_send_packet (const uint8_t *buf, int len, boolean is_broadcast)
{
    if (is_broadcast) {
        udp.beginPacket(broadcastIp, UDP_PORT);
    }
    else {
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
    }
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

    LightSenseTicker.attach(LIGHT_SENSE_INTERVAL, triggerLightSense);

    MasterLightRequestTicker.attach(MASTER_REQEST_INTERVAL, triggerMasterLightRequest);
    MasterSetpointTicker.attach(MASTER_SETPOINT_INTERVAL, triggerBroadcastSetpoint);

    // Set up motor control
    SVBsetup(!IM_MASTER);

    // Make first measurment
    getAndPrintLight();
    Serial.println("Setup Complite ...");
    light_logic();
}

void light_logic(void){
  static int is_open = 0;
  static int is_close = 0;
  
    /*
Light1: 162 lux, Light2: 71 lux
Light1: 162 lux, Light2: 72 lux
Light1: 162 lux, Light2: 72 lux
Light1: 162 lux, Light2: 72 lux
Light1: 162 lux, Light2: 70 lux
Light1: 162 lux, Light2: 67 lux
Light1: 162 lux, Light2: 61 lux
Light1: 162 lux, Light2: 65 lux
Light1: 169 lux, Light2: 850 lux
Light1: 169 lux, Light2: 2045 lux
Light1: 169 lux, Light2: 1954 lux
Light1: 160 lux, Light2: 53 lux
Light1: 161 lux, Light2: 68 lux
Light1: 161 lux, Light2: 72 lux
Light1: 161 lux, Light2: 72 lux
    */

    uint16_t in_sen_val = priv_lux1;
    uint16_t out_sen_val = priv_lux2;
    int16_t delta = out_sen_val - in_sen_val;

    if (delta > LIGHT_GEST) {
        if (!is_open) {
          is_open = 1;
          is_close = 0;
          send_command_open_all();    
        }
    }
    if (delta < -LIGHT_GEST) {
        if (!is_close) {
          is_open = 0;
          is_close = 1;
          send_command_close_all();    
        }
    }

}
void handle_master_logic(void){
    static int oldPosition = -1;
    int currentPosition;
    
    if (doRequestLight){
        send_light_request_to_all();
        doRequestLight = false;
    }

    currentPosition = getSVB_RelativePosition ();
    if (oldPosition >= 0) {
      if (currentPosition != oldPosition) {
        triggerBroadcastSetpoint ();
      }
    }
    oldPosition = getSVB_RelativePosition ();

    if (doBroadcastSetpoint && allowBroadcastSetpoint) {

        // Don't allow more setpoints before ratelimit timeout has exceeded
        allowBroadcastSetpoint = false;
        doBroadcastSetpoint = false;

        // Setpoint 'em all
        send_setpoint_to_all();

        // Start ratelimit timer
        MasterSetpointRateLimitTicker.once(MASTER_SETPOINT_RATELIMIT, ratelimitBroadcastSetpoint);

        // Reset the interval timer in case setpoint was not triggered by ticker callback
        MasterSetpointTicker.attach(MASTER_SETPOINT_INTERVAL, triggerBroadcastSetpoint);
    }
    light_logic();
}

void send_command_open_all(void){
    Serial.println("send_command_open_all ...");
    setSVB_RelativeSetpoint(50);
}

void send_command_close_all(void){
    Serial.println("send_command_close_all ...");
    setSVB_RelativeSetpoint(0);
}

void send_light_request_to_all(void){
    PDU pdu;

    pdu.transaction_id = 0x1234;
    pdu.group_id = MY_GROUP_ID;
    pdu.source = MY_DEVICE_ID;
    pdu.dest = -1;
    pdu.command = CMD_READ_SENSORS;
    pdu.data = NULL;
    pdu.data_len = 0;

    send_pdu (&pdu,true);
}

void send_setpoint_to_all (void) {
    uint8_t databuf[2];
    PDU pdu;

    int currentPosition = getSVB_RelativePosition () * 100;

    if (currentPosition > 10000) {
      currentPosition = 10000;
    } else if (currentPosition < 0) {
      currentPosition = 0;
    }

    uint16_t curPos16 = (uint16_t)currentPosition;
    databuf[0] = (curPos16 >> 8) & 0xFF;
    databuf[1] = (curPos16 >> 0) & 0xFF;

    pdu.transaction_id = 0x1234;
    pdu.group_id = MY_GROUP_ID;
    pdu.source = MY_DEVICE_ID;
    pdu.dest = -1;
    pdu.command = CMD_WRITE_SETPOINT;
    pdu.data = databuf;
    pdu.data_len = 2;

    send_pdu (&pdu,true);
}

void loop(void){
    if (isWifiConnect){
        udp_manage();
        if (IM_MASTER){
            handle_master_logic();
        }
    }
    else {
        isWifiConnect = tryToConnect();
    }

    if (doLightSense) {
        priv_current_pos = getAndPrintLight();
        doLightSense = false;
    }

  SVBdrive(!IM_MASTER); 
  delay(25);
}
