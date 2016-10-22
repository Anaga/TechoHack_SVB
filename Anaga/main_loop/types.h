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
