#include <MSP430x41x.h>

#include "drive.h"
#include "global.h"
#include "intsvc.h"
#include "iodefine.h"
#include "lcd.h"
#include "main.h"

/*************************************************************
// 	Interrupt Initial
//	-----------------
//	Input :
//	Output:
**************************************************************/
void Interrput_initial(void)
{
	volatile unsigned int i; // Use volatile to prevent removal by compiler optimizatio

	WDTCTL = WDTPW + WDTHOLD; // Stop WDT
	SCFI0 |= FN_2; // Set DCO operating range
	FLL_CTL0 |= XCAP10PF; // Configure load caps
	SCFQCTL = 60; //(60+1) x 32768 = 1.9988Mhz
	for (i = 0; i < 30000; i++)
		; // Delay for 32 kHz crystal to stabilize

	BTCTL = BT_fLCD_512 + BT_ADLY_32; // fACLK:64, 32ms
	IE2 |= BTIE; // Enable BT interrupt

	CACTL1 = CARSEL + CAREF_3; // CA1(-) Ref, Vt*
	CACTL2 = P2CA0; // CA0 connect, out not filter
	CAPD |= CAPD6; // Port pin buffer disable

	TACCTL0 = 0;
	TACTL = TASSEL_2 + MC_2 + ID_3; // SMCLK, cont. mode, divider /8 : 1200bps

	P1SEL &= ~TX_PIN;
	P1SEL &= ~RX_PIN;

	P1DIR |= TX_PIN; // set P1.0 as output
	P1DIR &= ~RX_PIN; // set P1.1 as input
	P1OUT &= ~TX_PIN; // set tx data bit low

	P1IFG = 0;
	P1IES |= RX_PIN; // P1.1 interrupt (high -> low transition)
	P1IE = 0;
}

/*************************************************************
// 	Input/Output Port Initial
//	-------------------------
//	Input :
//	Output:
**************************************************************/
void Port_initial(void)
{
	P1DIR = 0x00; // PORT P1 Direction
	P2DIR = 0xfc; // PORT P2 Direction
	P3DIR = 0xff; // PORT P3 Direction
	P4DIR = 0xff; // PORT P4 Direction
	P5DIR = 0x1f; // PORT P5 Direction
	P6DIR = 0x01; // PORT P6 Direction

	P1SEL = 0x40; // PORT P1 Selection
	P2SEL = 0xfc; // PORT P2 Selection
	P3SEL = 0xff; // PORT P3 Selection
	P4SEL = 0xff; // PORT P4 Selection
	P5SEL = 0xff; // PORT P5 Selection
	P6SEL = 0x00; // PORT P6 Selection

	P1OUT = 0xff; // PORT P1 Output
	P2OUT = 0xff; // PORT P2 Output
	P3OUT = 0xff; // PORT P3 Output
	P4OUT = 0xff; // PORT P4 Output
	P5OUT = 0xff; // PORT P5 Output
	P6OUT = 0xfe; // PORT P6 Output

	LCDCTL = LCDP1 + LCDP0 + LCD4MUX + LCDON; // 4-Mux LCD, segments S0-S23
}
