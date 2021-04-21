#ifndef __IO_DEFINE_H__
#define __IO_DEFINE_H__

// PORT1 Define

#define P_TXD BIT0 // I/O	//UART TXD port
#define P_MASTERREQ BIT1 // I		//Master request port
#define P_RXD BIT1 // I		//UART RXD port
#define P_BATSEL4 BIT2 // O		//BAT LEVEL4 SELECT
#define P_BATSEL3 BIT3 // O		//BAT LEVEL3 SELECT
#define P_BATSEL2 BIT4 // O		//BAT LEVEL2 SELECT
#define P_BATSEL1 BIT5 // O		//BAT LEVEL1 SELECT
#define P_BATLEVEL BIT6 // I		//BAT LEVEL INPUT
//#define 	NOT_USED	BIT7
#define MASTERREQ_TESTTEST P1IN &P_MRSENSOR1
#define PTXPORT_SET P1OUT |= P_TXD
#define PTXPORT_CLR P1OUT &= ~P_TXD
#define PBATSEL4_SET P1OUT |= P_BATSEL4
#define PBATSEL4_CLR P1OUT &= ~P_BATSEL4
#define PBATSEL3_SET P1OUT |= P_BATSEL3
#define PBATSEL3_CLR P1OUT &= ~P_BATSEL3
#define PBATSEL2_SET P1OUT |= P_BATSEL2
#define PBATSEL2_CLR P1OUT &= ~P_BATSEL2
#define PBATSEL1_SET P1OUT |= P_BATSEL1
#define PBATSEL1_CLR P1OUT &= ~P_BATSEL1

// PORT2 Define
//#define 	NOT_USED	BIT0
//#define 	NOT_USED	BIT1
//#define 	NOT_USED	BIT2		//O		//LCD SEG23
//#define 	NOT_USED	BIT3		//O		//LCD SEG22
//#define 	NOT_USED	BIT4		//O		//LCD SEG21
//#define 	NOT_USED	BIT5		//O		//LCD SEG20
//#define 	NOT_USED	BIT6		//O		//LCD SEG19
//#define 	NOT_USED	BIT7		//O		//LCD SEG18

// PORT3 Define
//#define 	NOT_USED	BIT0		//O		//LCD SEG17
//#define 	NOT_USED	BIT1		//O		//LCD SEG16
//#define 	NOT_USED	BIT2		//O		//LCD SEG15
//#define 	NOT_USED	BIT3		//O		//LCD SEG14
//#define 	NOT_USED	BIT4		//O		//LCD SEG13
//#define 	NOT_USED	BIT5		//O		//LCD SEG12
//#define 	NOT_USED	BIT6		//O		//LCD SEG11
//#define 	NOT_USED	BIT7		//O		//LCD SEG10

// PORT4 Define
//#define 	NOT_USED	BIT0		//O		//LCD SEG9
//#define 	NOT_USED	BIT1		//O		//LCD SEG8
//#define 	NOT_USED	BIT2		//O		//LCD SEG7
//#define 	NOT_USED	BIT3		//O		//LCD SEG6
//#define 	NOT_USED	BIT4		//O		//LCD SEG5
//#define 	NOT_USED	BIT5		//O		//LCD SEG4
//#define 	NOT_USED	BIT6		//O		//LCD SEG3
//#define 	NOT_USED	BIT7		//O		//LCD SEG2

// PORT5 Define
//#define 	NOT_USED	BIT0		//O		//LCD SEG1
//#define 	NOT_USED	BIT1		//O		//LCD SEG0
//#define 	NOT_USED	BIT2		//O		//LCD COM1
//#define 	NOT_USED	BIT3		//O		//LCD COM2
//#define 	NOT_USED	BIT4		//O		//LCD COM3
//#define 	NOT_USED	BIT5		//I		//LCD V2
//#define 	NOT_USED	BIT6		//I		//LCD V3
//#define 	NOT_USED	BIT7		//I		//LCD V4

// PORT6 Define
#define P_MRPOWER BIT0 // O		//MR Sensor Power port
#define P_MRSENSOR1 BIT1 // I		//MR Sensor Signal1 port
#define P_MRSENSOR2 BIT2 // I		//MR Sensor Signal2 port
//#define 	NOT_USED	BIT3
//#define 	NOT_USED	BIT4
#define P_OPTION1 BIT5 // I		//Option1 port
#define P_OPTION2 BIT6 // I		//Option2 port
//#define 	NOT_USED	BIT7
#define MRPOWER_SET P6OUT |= P_MRPOWER
#define MRPOWER_CLR P6OUT &= ~P_MRPOWER
#define MRSENSOR1_TEST P6IN &P_MRSENSOR1
#define MRSENSOR2_TEST P6IN &P_MRSENSOR2
#define OPTION1_TEST P6IN &P_OPTION1
#define OPTION2_TEST P6IN &P_OPTION2

#endif
