#ifndef __FLASH_H__
#define __FLASH_H__

#include "global.h"

#define FLASH_VALID 0xAC

typedef struct {
	u_char value[VALUE_DIGIT_LEN]; // ton, liter, cc
	u_char serial[4];
	u_char meterType;
	u_char sleepStatus;
	u_int cc;
	int qtcc;
	int q2cc;
	int q1cc;
	u_char checksum;
} flash_t;

void writeFlash();
u_char readFlash();
void saveMeterValue();

#endif //__FLASH_H__
