#include "protocol.h"


WiFiUDP priv_udp;
IPAddress priv_broadcastIp(192, 168, 0, 255);
#define UDP_PORT 1555

#define PDU_HDR_AND_CRC_LEN 14
void udp_setup(void){
     priv_udp.begin(1555);
}

void udp_print_packet (const uint8_t *buf, int len){
    char hex[5];
    for (int i = 0; i < len; i++) {
        snprintf (hex, sizeof (hex), "<%02X> ", buf[i]);
        Serial.print(hex);
    }
    Serial.println("");
}

void udp_send_packet (const uint8_t *buf, int len, boolean is_broadcast)
{
    if (is_broadcast) {
        priv_udp.beginPacket(priv_broadcastIp, UDP_PORT);
    }
    else {
        priv_udp.beginPacket(priv_udp.remoteIP(), priv_udp.remotePort());
    }
    priv_udp.write(buf, len);
    priv_udp.endPacket();
}
#if 0
int udp_parse_packet (const uint8_t *buf, int len)
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
#endif
