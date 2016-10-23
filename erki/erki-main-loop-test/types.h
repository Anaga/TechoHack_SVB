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
