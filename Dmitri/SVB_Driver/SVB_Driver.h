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
    int getSVB_WantedPosition(void);
    void setSVB_WantedPosition(int value);
    void averageSettleDown(void);
    int getSVB_EncoderPosition(void);
    void setSVB_RelativeSetpoint (uint16_t sp);
    uint16_t getSVB_RelativeSetpoint (void);
    uint16_t getSVB_RelativePosition (void);
  #endif



