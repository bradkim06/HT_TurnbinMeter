#ifndef __MAIN_H__
#define __MAIN_H__

//************** DEFINITIONS *********************************

#define VLD_SVSOFF 0x00
#define VLD1_9V 0x10
#define VLD2_1V 0x20
#define VLD2_2V 0x30
#define VLD2_3V 0x40
#define VLD2_4V 0x50
#define VLD2_5V 0x60
#define VLD2_65V 0x70
#define VLD2_8V 0x80
#define VLD2_9V 0x90
#define VLD3_05V 0xa0
#define VLD3_2V 0xb0
#define VLD3_35V 0xc0
#define VLD3_5V 0xd0
#define VLD3_7V 0xe0

#define BATTERY_H 3
#define BATTERY_M 2
#define BATTERY_L 1
#define BATTERY_EMPTY 0

#define BATTERY_LVLSET 0
#define BATTERY_OUTCHK1 1
#define BATTERY_OUTCHK2 2
#define BATTERY_OUTCHK3 3

#define BATTERY_CHECK_INTERVAL 60 // sec

//#define MASTER_DATA_REQUEST		0x02
//#define MASTER_SERIALNO_RECEIVE	0x01

#define MASTER_DATA_REQUEST 0x03
#define MASTER_SERIALNO_RECEIVE 0x00

#define FORWARD 0
#define REVERSE 1

//************** TYPE ****************************************

//************** USER NORMAL PARAMETERS **********************

//************** EXTERN **************************************

extern BYTE WaterFlowTone[5]; // Water flow tone data
extern BYTE WaterFlowCC[6]; // Water flow CC data
extern BYTE SerialNumber[10]; // Serial Number

extern BYTE BatteryLevel; // Battery level
extern INT16 BatteryCheckTime; // Battery level check time

extern BYTE LongWaiting; // Long waiting flag

extern BYTE LCDTestMode; // LCD Test mode

//************** FUNCTION PROTOTYPES *************************
void delay_100msec(int n);
void delay_msec(int n);
void display_initialScreen();

#endif
