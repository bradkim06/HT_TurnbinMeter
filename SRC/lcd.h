#ifndef __LCD_H__
#define __LCD_H__

#define LCD_TURN_ON_POINT()                                                                        \
	{                                                                                          \
		LCDM11 |= 0x10;                                                                    \
	}
#define LCD_TURN_OFF_POINT()                                                                       \
	{                                                                                          \
		LCDM11 &= ~0x10;                                                                   \
	}

#define LCD_TURN_ON_LEAK()                                                                         \
	{                                                                                          \
		LCDM8 |= 0x10;                                                                     \
		LCDM9 |= 0x10;                                                                     \
	}
#define LCD_TURN_OFF_LEAK()                                                                        \
	{                                                                                          \
		LCDM8 &= ~0x10;                                                                    \
		LCDM9 &= ~0x10;                                                                    \
	}

#define LCD_TURN_ON_M3()                                                                           \
	{                                                                                          \
		LCDM10 |= 0x10;                                                                    \
	}
#define LCD_TURN_OFF_M3()                                                                          \
	{                                                                                          \
		LCDM10 &= ~0x10;                                                                   \
	}

#define LCD_TURN_ON_RVS_ARROW()                                                                    \
	{                                                                                          \
		LCDM6 |= 0x10;                                                                     \
		LCDM7 &= ~0x10;                                                                    \
	}
#define LCD_TURN_ON_FWD_ARROW()                                                                    \
	{                                                                                          \
		LCDM6 &= ~0x10;                                                                    \
		LCDM7 |= 0x10;                                                                     \
	}
#define LCD_TURN_OFF_ARROWS()                                                                      \
	{                                                                                          \
		LCDM6 &= ~0x10;                                                                    \
		LCDM7 &= ~0x10;                                                                    \
	}

#define LCD_TURN_ON_NO_USE()                                                                       \
	{                                                                                          \
		LCDM12 |= 0x10;                                                                    \
	}
#define LCD_TURN_OFF_NO_USE()                                                                      \
	{                                                                                          \
		LCDM12 &= ~0x10;                                                                   \
	}

#define LCD_TURN_ON_H()                                                                            \
	{                                                                                          \
		LCDM5 |= 0x01;                                                                     \
	}
#define LCD_TURN_OFF_H()                                                                           \
	{                                                                                          \
		LCDM5 &= ~0x01;                                                                    \
	}

void LCD_allOnOff(BYTE onOff);
void LCD_Battery_ICON_Display(BYTE IData);
void LCD_Flow_ICON(void);
void LCD_Pattern_Display(BYTE const *Pattern);
void LCD_Digit_Display(int digit, BYTE NumData);
void DisplayMeterValue(u_char pos);
void LCD_Display_All();
void LCD_Display_Minus();

void LCD_Flow_ON();
void LCD_Flow_OFF();

#endif
