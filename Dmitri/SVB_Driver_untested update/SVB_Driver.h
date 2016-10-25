#ifndef SVB_Driver_h
#define SVB_Driver_h

#include "Arduino.h"

// Declaration of the functions

void doEncoder(void);
void calcAverage(int lastRead);
void motorRun(signed int pwm, int motorNumber);
void motorStop(void);
void motorBreak(void);
void doMoveIt(int motorNumber);
void doCalibrate(int motorNumber);
void SVBdrive(int motorNumber);
void SVBsetup(int motorNumber);
int getSVB_WantedPosition(void);
void setSVB_WantedPosition(int value);
void averageSettleDown(void);
int getSVB_EncoderPosition(void);
void setSVB_RelativeSetpoint (uint16_t sp);
uint16_t getSVB_RelativeSetpoint (void);
uint16_t getSVB_RelativePosition (void);
int getSVB_Mode (void);
void getDelta(void);
void getDirection(void);
void getEncoderAndPos(void);
void getRightLeftPos(void);
#endif



