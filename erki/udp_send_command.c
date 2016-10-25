#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>

#define UDP_PORT 1555

static int sockfd = -1;

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

static int16_t transactionID = 0;

#define MY_DEVICE_ID ((0x14 << 16) | (0x5A << 8) | (0x50 << 0))

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

int udp_send (const uint8_t *buf, int len)
{
    int res;
    struct sockaddr_in s;

    memset(&s, '\0', sizeof(struct sockaddr_in));
    s.sin_family = AF_INET;
    s.sin_port = htons(UDP_PORT);
    s.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    res = sendto(sockfd, buf, len, 0, (struct sockaddr *)&s, sizeof(struct sockaddr_in));
    if (res == -1) {
        perror("sendto");
        return -1;
    }
}

int cmd_read_sensors (uint8_t group, int dest_device_id)
{
    uint8_t buf[32];
    uint8_t *bufptr = buf;

    transactionID++;

    *bufptr++ = (transactionID >> 8) & 0xFF;
    *bufptr++ = (transactionID >> 0) & 0xFF;
    *bufptr++ = group;
    *bufptr++ = (MY_DEVICE_ID >> 24) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >> 16) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >>  8) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >>  0) & 0xFF;
    *bufptr++ = (dest_device_id >> 24) & 0xFF;
    *bufptr++ = (dest_device_id >> 16) & 0xFF;
    *bufptr++ = (dest_device_id >>  8) & 0xFF;
    *bufptr++ = (dest_device_id >>  0) & 0xFF;
    *bufptr++ = CMD_READ_SENSORS;
 
    uint16_t crc = CRC16_Calc (buf, bufptr - buf);
    *bufptr++ = (crc >> 8) & 0xFF;
    *bufptr++ = (crc >> 0) & 0xFF;
    
    return udp_send (buf, bufptr - buf);
}

int cmd_write_setpoint (uint8_t group, int dest_device_id, int16_t setpoint)
{
    uint8_t buf[32];
    uint8_t *bufptr = buf;

    transactionID++;

    *bufptr++ = (transactionID >> 8) & 0xFF;
    *bufptr++ = (transactionID >> 0) & 0xFF;
    *bufptr++ = group;
    *bufptr++ = (MY_DEVICE_ID >> 24) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >> 16) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >>  8) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >>  0) & 0xFF;
    *bufptr++ = (dest_device_id >> 24) & 0xFF;
    *bufptr++ = (dest_device_id >> 16) & 0xFF;
    *bufptr++ = (dest_device_id >>  8) & 0xFF;
    *bufptr++ = (dest_device_id >>  0) & 0xFF;
    *bufptr++ = CMD_WRITE_SETPOINT;
    *bufptr++ = (setpoint >> 8) & 0xFF;
    *bufptr++ = (setpoint >> 0) & 0xFF;;
 
    uint16_t crc = CRC16_Calc (buf, bufptr - buf);
    *bufptr++ = (crc >> 8) & 0xFF;
    *bufptr++ = (crc >> 0) & 0xFF;
    
    return udp_send (buf, bufptr - buf);
}

int cmd_read_setpoint (uint8_t group, int dest_device_id)
{
    uint8_t buf[32];
    uint8_t *bufptr = buf;

    transactionID++;

    *bufptr++ = (transactionID >> 8) & 0xFF;
    *bufptr++ = (transactionID >> 0) & 0xFF;
    *bufptr++ = group;
    *bufptr++ = (MY_DEVICE_ID >> 24) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >> 16) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >>  8) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >>  0) & 0xFF;
    *bufptr++ = (dest_device_id >> 24) & 0xFF;
    *bufptr++ = (dest_device_id >> 16) & 0xFF;
    *bufptr++ = (dest_device_id >>  8) & 0xFF;
    *bufptr++ = (dest_device_id >>  0) & 0xFF;
    *bufptr++ = CMD_READ_SETPOINT;
 
    uint16_t crc = CRC16_Calc (buf, bufptr - buf);
    *bufptr++ = (crc >> 8) & 0xFF;
    *bufptr++ = (crc >> 0) & 0xFF;
    
    return udp_send (buf, bufptr - buf);
}

int cmd_read_position (uint8_t group, int dest_device_id)
{
    uint8_t buf[32];
    uint8_t *bufptr = buf;

    transactionID++;

    *bufptr++ = (transactionID >> 8) & 0xFF;
    *bufptr++ = (transactionID >> 0) & 0xFF;
    *bufptr++ = group;
    *bufptr++ = (MY_DEVICE_ID >> 24) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >> 16) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >>  8) & 0xFF;
    *bufptr++ = (MY_DEVICE_ID >>  0) & 0xFF;
    *bufptr++ = (dest_device_id >> 24) & 0xFF;
    *bufptr++ = (dest_device_id >> 16) & 0xFF;
    *bufptr++ = (dest_device_id >>  8) & 0xFF;
    *bufptr++ = (dest_device_id >>  0) & 0xFF;
    *bufptr++ = CMD_READ_POSITION;
 
    uint16_t crc = CRC16_Calc (buf, bufptr - buf);
    *bufptr++ = (crc >> 8) & 0xFF;
    *bufptr++ = (crc >> 0) & 0xFF;
    
    return udp_send (buf, bufptr - buf);
}

int handle_packet (const uint8_t *buf, int len)
{
    uint16_t crc;
    PDU pdu;

    if (len < 6) {
        printf ("ERR: UDP packet too small");
        return -1;
    }

    crc = (buf[len - 2] << 8) | buf[len - 1];

    if (crc != CRC16_Calc (buf, len - 2)) {
        printf ("ERR: CRC mismatch\n");
        return -1;
    }

    pdu.transaction_id = ((buf[0] << 8) | buf[1]);
    pdu.group_id = buf[2];
    pdu.source = (buf[3] << 24 | buf[4] << 16 | buf[5] << 8 | buf[6]);
    pdu.dest = (buf[7] << 24 | buf[8] << 16 | buf[9] << 8 | buf[10]);
    pdu.command = buf[11];
    pdu.data = &buf[12];
    pdu.data_len = len - PDU_HDR_AND_CRC_LEN;
  
    // Ignore our own echoes...
    if (pdu.source == MY_DEVICE_ID) {
        return 0;
    }
    
    printf ("TrID: %d, GID: %d, %d => %d, 0x%02X, data_len: %d\n", pdu.transaction_id, pdu.group_id, pdu.source, pdu.dest, pdu.command, pdu.data_len);
    if (pdu.command & 0x80) {
        if (pdu.data_len >= 1) {
            if (pdu.data[0] == CMD_ST_OK) {
                printf ("  - CMD_ST_OK\n");
                switch (pdu.command ^ 0x80) {
                    case CMD_READ_SENSORS: {
                        if (pdu.data_len % 2) {
                            for (int i = 0; i < (pdu.data_len - 1) / 2; i++) {
                                uint16_t val = pdu.data[i * 2 + 1] << 8 | pdu.data[i * 2 + 2];
                                printf ("  - Sensor %d: %d\n", i + 1, val);
                            }
                        } else {
                            printf ("  - Invalid data length\n");
                        }
                    }
                    break;
                    case CMD_READ_SETPOINT: {
                        if (pdu.data_len == 3) {
                            uint16_t sp = pdu.data[1] << 8 | pdu.data[2];
                            printf ("  - Setpoint: %d\n", sp);
                        } else {
                            printf ("  - Invalid data length\n");
                        }
                    }
                    break;
                    case CMD_READ_POSITION: {
                        if (pdu.data_len == 3) {
                            uint16_t pos = pdu.data[1] << 8 | pdu.data[2];
                            printf ("  - Position: %d\n", pos);
                        } else {
                            printf ("  - Invalid data length\n");
                        }
                    }
                    break;
                    default: {
                    }
                    break;
                }
            } else {
            }
        } else {
            printf ("  - Invalid data length\n");
        }
    } else {
        switch (pdu.command) {
            case CMD_WRITE_SETPOINT: {
                if (pdu.data_len == 2) {
                    uint16_t sp = pdu.data[0] << 8 | pdu.data[1];
                    printf ("  - Setpoint: %d\n", sp);
                } else {
                    printf ("  - Invalid data length\n");
                }
            }
            break;
            default: {
            }
            break;
        }
    }
}


int main (int argc, char **argv)
{
    struct sockaddr_in servaddr;
    int broadcast = 1;
    int res, pfds;
    uint8_t buf[256];
    time_t start, now;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);
   
    // Enable broadcast
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof (broadcast)) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    memset(&servaddr, '\0', sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr ("0.0.0.0");
    servaddr.sin_port = htons (UDP_PORT);

    if (bind (sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        exit(1);
    }

    struct sockaddr_storage src_addr;

    struct iovec iov[1];
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);

    struct msghdr message;
    message.msg_name = &src_addr;
    message.msg_namelen = sizeof(src_addr);
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = 0;
    message.msg_controllen = 0;

    fd_set rfds;
    struct timeval tv;
    int sw = 0;

/*
    res = cmd_write_setpoint (1, -1, 10000);
    if (res == -1) {
        perror("sendto");
        exit(1);
    }
*/

    while (1) {

        sw++;
        
        if (sw == 1) {
            res = cmd_read_sensors (1, -1);
        } else if (sw == 2) {
            res = cmd_read_setpoint (1, -1);
        } else {
            res = cmd_read_position (1, -1);
            sw = 0;
        }
        
        if (res == -1) {
            perror("sendto");
            exit(1);
        }
        printf ("Sent %d bytes\n", res);
        
        start = time (NULL);
        
        while (1) {

            FD_ZERO(&rfds);
            FD_SET(0, &rfds);

            tv.tv_sec = 0;
            tv.tv_usec = 10000;

            // Doesn't work with UDP?
            pfds = select (1, &rfds, NULL, NULL, &tv);

            while (1) {
                res = recvmsg (sockfd, &message, MSG_PEEK);
    
                if (res > 0) {
                    res = recvmsg (sockfd, &message, 0);
                
                    if (res == -1) {
                        perror ("recvmsg");
                    } else if (message.msg_flags & MSG_TRUNC) {
                        printf ("Packet truncated\n");
                    } else {
                        handle_packet (buf, res);
                    }
                } else {
                    break;
                }
            }
            
            now = time (NULL);
            if (now - start > 1) {
                break;
            }

        }
                
    }

    close(sockfd);
    
    return 0;
}
