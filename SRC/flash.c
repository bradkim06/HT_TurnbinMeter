#include <MSP430x41x.h>
#include <string.h>

#include "drive.h"
#include "flash.h"
#include "global.h"
#include "intsvc.h"
#include "iodefine.h"
#include "lcd.h"
#include "main.h"

extern config_t config;
extern current_data_t current;
extern lcdFlag_t lcdFlag;
extern flash_t flash;

const char *FLASH_A_ADDR = (char *)0x1080;
const char *FLASH_B_ADDR = (char *)0x1000;

void erase_sector(char *pDevice)
{
	FCTL2 = FWKEY + FSSEL1 + FN0; // for information flash memory

	FCTL3 = FWKEY; // Clear Lock bit
	FCTL1 = FWKEY + ERASE; // Set Erase bit

	*pDevice = 0; // Dummy write to erase Flash seg
}

void write_sector(char *pDevice)
{
	STOP_WATCHDOG();
	_BIC_SR(GIE); // disable interrupt

	erase_sector(pDevice);

	while (FCTL3 & BUSY)
		;

	FCTL1 = FWKEY + WRT; // Set WRT bit for write operation

	u_char *p = (u_char *)&flash;
	for (int i = 0; i < sizeof(flash_t); i++) {
		*pDevice++ = *p++; // Write value to flash
	}

	FCTL3 = FWKEY + LOCK;

	_BIS_SR(GIE); // enable interrupt
	START_WATCHDOG();
}

u_char read_sector(char *pDevice)
{
	char *pDes = (char *)&flash;

	for (int i = 0; i < sizeof(flash_t); i++) {
		*pDes++ = *pDevice++;
	}

	u_char checksum = 0;
	pDes = (char *)&flash;

	for (int i = 0; i < sizeof(flash_t) - 2; i++) {
		checksum += *pDes++;
	}

	if (checksum == flash.checksum /* && pFlash->flag == FLASH_VALID*/) {
		return 1;
	}
	return 0;
}

void writeFlash()
{
	int len = sizeof(flash_t);
	u_char *p = (u_char *)&flash;

	flash.checksum = 0;
	for (int i = 0; i < len - 2; i++) {
		flash.checksum += *p++;
	}

	write_sector((char *)FLASH_A_ADDR);
	write_sector((char *)FLASH_B_ADDR);
}

u_char readFlash()
{
	if (read_sector((char *)FLASH_A_ADDR)) {
		write_sector((char *)FLASH_B_ADDR);
		return 1;
	} else if (read_sector((char *)FLASH_B_ADDR)) {
		write_sector((char *)FLASH_A_ADDR);
		return 1;
	}
	return 0;
}

void saveMeterValue()
{
	readFlash();

	memcpy(flash.value, current.value, VALUE_DIGIT_LEN);

	writeFlash();
}
