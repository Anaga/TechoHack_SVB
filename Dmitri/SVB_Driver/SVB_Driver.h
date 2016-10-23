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
  #endif



