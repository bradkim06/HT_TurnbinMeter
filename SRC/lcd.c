#include <MSP430x41x.h>
#include <string.h>

#include "drive.h"
#include "global.h"
#include "intsvc.h"
#include "iodefine.h"
#include "lcd.h"
#include "main.h"

#define LCD_DIGIT_NUM 11

extern current_data_t current;

void LCD_allOnOff(BYTE onOff)
{
	if (onOff == TRUE) {
		onOff = 0xFF;
	}

	LCDM1 = onOff;
	LCDM2 = onOff;
	LCDM3 = onOff;
	LCDM3 = onOff;
	LCDM4 = onOff;
	LCDM5 = onOff;
	LCDM6 = onOff;
	LCDM7 = onOff;
	LCDM8 = onOff;
	LCDM9 = onOff;
	LCDM10 = onOff;
	LCDM11 = onOff;
	LCDM12 = onOff;
}

void LCD_Flow_ICON()
{
	static u_char toggle = 0;
	toggle ^= 0x01;

	LCDM4 |= 0x01;

	LCDM1 &= 0x0F;
	if (toggle & 0x01) {
		LCDM1 |= 0x90;
	} else {
		LCDM1 |= 0x60;
	}
}

void LCD_Battery_ICON_Display(BYTE IData)
{
	LCDM1 &= ~0x0e; // Battery ICon - R,R1,R2,R3 ICON Off

	if (IData >= 3) {
		LCDM1 |= 0x0f; // Battery ICon - R,R1,R2,R3 ICON On
	} else if (IData == 2) {
		LCDM1 |= 0x07; // Battery ICon - R,R2,R3 ICON On - R1 ICON Off
	} else if (IData == 1) {
		LCDM1 |= 0x05; // Battery ICon - R,R3 ICON On - R1,R2 ICON Off
	} else {
		LCDM1 |= 0x01; // Battery ICon - R ICON On - R1,R2,R3 ICON Off
	}
}

void LCD_Pattern_Display(BYTE const *Pattern)
{
	for (int i = 0; i < LCD_DIGIT_NUM; i++) {
		LCD_Digit_Display(i, Pattern[i]);
	}
}

const struct {
	u_char *addr;
} DIGIT_ADDR[LCD_DIGIT_NUM] = { (u_char *)LCDM6_,  (u_char *)LCDM7_,  (u_char *)LCDM8_,
				(u_char *)LCDM9_,  (u_char *)LCDM10_, (u_char *)LCDM11_,
				(u_char *)LCDM12_, (u_char *)LCDM5_,  (u_char *)LCDM4_,
				(u_char *)LCDM3_,  (u_char *)LCDM2_ };

void lcd10CCField(u_char data)
{
	LCDM2 = (LCDM2 & 0x01) | (data & 0xfe);
}

void lcd100CCField(u_char data)
{
	LCDM3 = (LCDM3 & 0x01) | (data & 0xfe);
}

void lcd1LiterField(u_char data)
{
	LCDM4 = (LCDM4 & 0x01) | (data & 0xfe);
}

void lcd10LiterField(u_char data)
{
	LCDM5 = (LCDM5 & 0x01) | (data & 0xfe);
}

void lcd100LiterField(u_char data)
{
	LCDM12 = (LCDM12 & 0x10) | (data & 0xef);
}

void lcd1TonField(u_char data)
{
	LCDM11 = (LCDM11 & 0x10) | (data & 0xef);
}

void lcd10TonField(u_char data)
{
	LCDM10 = (LCDM10 & 0x10) | (data & 0xef);
}

void lcd100TonField(u_char data)
{
	LCDM9 = (LCDM9 & 0x10) | (data & 0xef);
}

void lcd1000TonField(u_char data)
{
	LCDM8 = (LCDM8 & 0x10) | (data & 0xef);
}

void lcd10000TonField(u_char data)
{
	LCDM7 = (LCDM7 & 0x10) | (data & 0xef);
}

void lcd100000TonField(u_char data)
{
	LCDM6 = (LCDM6 & 0x10) | (data & 0xef);
}

const u_char digitPattern_normal[11] =
	// bit 0가 반드시 0이어야 함.
	{ 0xFA, 0x0A, 0xD6, 0x9E, 0x2E, 0xBC, 0xFC, 0x3A, 0xFE, 0x3E, 0x04 };
//         '0'      '1'      '2'     '3'     '4'      '5'      '6'     '7' '8'
//         '9'

const u_char digitPattern_reverse[11] =
	// bit 4가 반드시 0이어야 함.
	{ 0xAF, 0xA0, 0x6D, 0xE9, 0xE2, 0xCB, 0xCF, 0xA3, 0xEF, 0xE3, 0x40 };
//         '0'      '1'      '2'     '3'     '4'      '5'      '6'     '7' '8'
//         '9'

void LCD_Digit_Display(int digit, BYTE NumData)
{
	u_char code = 0;

	if (NumData > 9) {
		return;
	}

	u_char *addr = DIGIT_ADDR[digit].addr;

	if (digit < 7) {
		code = digitPattern_reverse[NumData];
		*addr = (*addr & 0x10) | (code & 0xef);
	} else {
		code = digitPattern_normal[NumData];
		*addr = (*addr & 0x01) | (code & 0xfe);
	}
}

void LCD_Display_Minus()
{
	LCDM6 = (LCDM6 & 0x10) | digitPattern_reverse[10];
	LCDM7 = (LCDM7 & 0x10) | digitPattern_reverse[10];
	LCDM8 = (LCDM8 & 0x10) | digitPattern_reverse[10];
	LCDM9 = (LCDM9 & 0x10) | digitPattern_reverse[10];
	LCDM10 = (LCDM10 & 0x10) | digitPattern_reverse[10];
	LCDM11 = (LCDM11 & 0x10) | digitPattern_reverse[10];
	LCDM12 = (LCDM12 & 0x10) | digitPattern_reverse[10];
	LCDM5 = (LCDM5 & 0x01) | digitPattern_normal[10];
	LCDM4 = (LCDM4 & 0x01) | digitPattern_normal[10];
	LCDM3 = (LCDM3 & 0x01) | digitPattern_normal[10];
	LCDM2 = (LCDM2 & 0x01) | digitPattern_normal[10];
}

void DisplayMeterValue(u_char pos)
{
	// digit 0 ~ 6까지 7자리는 상하위 nibble이 뒤바뀐 digitPattern_reverse를
	// 사용하여야 함. 각 case 뒤에 break가 붙지 않아야 함.

	switch (pos) {
	case 0:
		LCDM6 = (LCDM6 & 0x10) | digitPattern_reverse[current.value[0]];
	case 1:
		LCDM7 = (LCDM7 & 0x10) | digitPattern_reverse[current.value[1]];
	case 2:
		LCDM8 = (LCDM8 & 0x10) | digitPattern_reverse[current.value[2]];
	case 3:
		LCDM9 = (LCDM9 & 0x10) | digitPattern_reverse[current.value[3]];
	case 4:
		LCDM10 = (LCDM10 & 0x10) | digitPattern_reverse[current.value[4]];
	case 5:
		LCDM11 = (LCDM11 & 0x10) | digitPattern_reverse[current.value[5]];
	case 6:
		LCDM12 = (LCDM12 & 0x10) | digitPattern_reverse[current.value[6]];
	case 7:
		LCDM5 = (LCDM5 & 0x01) | digitPattern_normal[current.value[7]];
	case 8:
		LCDM4 = (LCDM4 & 0x01) | digitPattern_normal[current.value[8]];
	case 9:
		LCDM3 = (LCDM3 & 0x01) | digitPattern_normal[current.value[9]];
	case 10:
		LCDM2 = (LCDM2 & 0x01) | digitPattern_normal[current.value[10]];
		break;
	}
}
