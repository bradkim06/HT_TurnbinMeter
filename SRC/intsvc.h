#ifndef __INTSVC_H__
#define __INTSVC_H__

#include "global.h"

//************** DEFINITIONS *********************************
#define STATUS_0 0
#define STATUS_1 1
#define STATUS_2 2
#define STATUS_3 3

#define MRSENSOR_CASE0 0x00
#define MRSENSOR_CASE1 0x02
#define MRSENSOR_CASE2 0x06
#define MRSENSOR_CASE3 0x04

#define MRSENSOR_STATUS_H_LEVEL 0x02
#define MRSENSOR_STATUS_L_LEVEL 0x00

#define BITIME12_5 0x68 // 0x6c                 // ~ 0.5 bit length + small adjustment
#define BITIME12 0xD1 // 0xD8                 // ~ 1200 baud

void init_parameters();
void startTx(u_char mode);
int longWaiting();
void readBattery();
void saveAndPeriodicReset();
void restoreAfterPeriodicReset();

#endif
