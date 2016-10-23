#include "protocol.h"

void udp_print_packet (const uint8_t *buf, int len){
    char hex[5];
    for (int i = 0; i < len; i++) {
        snprintf (hex, sizeof (hex), "<%02X> ", buf[i]);
        Serial.print(hex);
    }
    Serial.println("");
}
#endif
