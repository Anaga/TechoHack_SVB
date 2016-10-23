#ifndef protocol_h
#define protocol_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

void udp_setup(void);

void udp_print_packet (const uint8_t *buf, int len);
void udp_send_packet (const uint8_t *buf, int len, boolean is_broadcast);
//int udp_parse_packet (const uint8_t *buf, int len);
#endif
