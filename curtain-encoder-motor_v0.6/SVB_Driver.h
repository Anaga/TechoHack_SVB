#ifndef SVB_Driver_h
#define SVB_Driver_h

#include "Arduino.h"

// Declaration of the functions

void doEncoder(void);
    void calcAverage(int lastRead);
    void motorRun(signed int pwm);
    void motorStop(void);
    void motorBreak(void);
    void doMoveIt(void);
    void doCalibrate(void);
    void SVBdrive(void);
    void SVBsetup(void);
    long getSVB_WantedPosition(void);
    void setSVB_WantedPosition(long value);
    void averageSettleDown(void);
    long getSVB_EncoderPosition(void);
  #endif



