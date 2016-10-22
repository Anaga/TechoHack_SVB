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

  if (pdu->command & 0x80) {

    switch (pdu->command ^ 0x80) {

      case CMD_READ_SENSORS: {
        uint16_t sensor1;
        uint16_t sensor2;
        if (pdu->data_len == CMD_ST_OK) {
          if (pdu->data_len >= 3) {
            sensor1 = (pdu->data[1] << 8) | pdu->data[2];
          }
          if (pdu->data_len >= 4) {
            sensor2 = (pdu->data[3] << 8) | pdu->data[4];
          }
        }
      }
      break;

      case CMD_READ_SETPOINT: {
        uint16_t sp;
        if (pdu->data_len == CMD_ST_OK) {
          if (pdu->data_len >= 3) {
            sp = (pdu->data[1] << 8) | pdu->data[2];
          }
        }
      }
      break;

      case CMD_READ_POSITION: {
        uint16_t pos;
        if (pdu->data_len == CMD_ST_OK) {
          if (pdu->data_len >= 3) {
            pos = (pdu->data[1] << 8) | pdu->data[2];
          }
        }
      }
      break;
    }

  } else {

    // This is command

    Serial.print ("Command ");
    Serial.print (pdu->command);
    Serial.print (" received (data_len:");
    Serial.print (pdu->data_len);
    Serial.println (")");

    switch (pdu->command) {

      case CMD_READ_SENSORS: {
        *dataptr++ = CMD_ST_OK;
        *dataptr++ = (sensors_data[0] >> 8) & 0xFF;
        *dataptr++ = (sensors_data[0] >> 0) & 0xFF;
        *dataptr++ = (sensors_data[1] >> 8) & 0xFF;
        *dataptr++ = (sensors_data[1] >> 0) & 0xFF;
        pdu->data_len = sizeof (sensors_data);
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
          pdu->data_len = 2;
      }
      break;

      case CMD_READ_POSITION: {
        uint16_t pos = current_pos_get ();
        *dataptr++ = CMD_ST_OK;
        *dataptr++ = (pos >> 8) & 0xFF;
        *dataptr++ = (pos >> 0) & 0xFF;
        pdu->data_len = 2;
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
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  char mac[18];
  WiFi.macAddress().toCharArray (mac, sizeof(mac));
  MY_DEVICE_ID = ((hexdec(mac[9]) << 20) | (hexdec(mac[10]) << 16) | (hexdec(mac[12]) << 12) | (hexdec(mac[13]) << 8) | (hexdec(mac[15]) << 4) | (hexdec(mac[16]) << 0));

  Serial.print("Device ID: ");
  Serial.println(MY_DEVICE_ID);

  udp.begin(1555);
}

void loop()
{
  led_manage();
  udp_manage();
}
