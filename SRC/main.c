#include <MSP430x41x.h>
#include <string.h>

#include "drive.h"
#include "flash.h"
#include "global.h"
#include "intsvc.h"
#include "iodefine.h"
#include "lcd.h"
#include "main.h"
#include "meter.h"

extern flag_t flag;

txBuf_t txBuf;
rxData_t rxData;
rxBuf_t rxBuf;

u_char RxReady = 0;

flash_t flash;
config_t config;
current_data_t current;
lcdFlag_t lcdFlag;
int nRxMessage = 0;

meter_config_t *pMeterInfo = NULL;
static int nomal_mode_count = 0;

extern volatile u_char timerA_mode;

extern pulse_type_req_t pulseTypeReq;

void delay_msec(int n)
{
	u_int i;
	do {
		i = 333;
		do {
			__no_operation();
			__no_operation();
		} while (--i);
	} while (--n);
}

void delay_100msec(int n)
{
	for (int i = 0; i < n; i++) {
		delay_msec(100);
		RESTART_WATCHDOG();
	}
}

u_char Cal_Checksum(u_char *p, u_char len)
{
	u_char checksum = 0;
	for (int i = 0; i < len; i++) {
		checksum += *p++;
	}
	return checksum;
}

void seperateDigit(u_int value, u_char *array)
{
	array[0] = value / 1000;
	value %= 1000;
	array[1] = value / 100;
	value %= 100;
	array[2] = value / 10;
	array[3] = value % 10;
}

u_char checkValid(u_int value)
{
	u_int cc = pMeterInfo->cc;

	if (value <= (cc * 3) / 2 && value >= cc / 2) {
		return 1;
	} else {
		return 0;
	}
}

void set_q3Value()
{
	// 설정된 값이 설계치의 0.5 ~ 1.5배 범위를 벗어나면 설정치를 무시하고 설계치를
	// 적용함.
	if (flash.cc < (pMeterInfo->cc / 2) || flash.cc > ((pMeterInfo->cc * 3) / 2)) {
		flash.cc = pMeterInfo->cc;
		writeFlash();
	}

	u_int value = flash.cc;

	seperateDigit(value, config.q3Value);

	value = flash.cc + flash.qtcc;
	seperateDigit(value, config.qtValue);

	value = flash.cc + flash.q2cc;
	seperateDigit(value, config.q2Value);

	value = flash.cc + flash.q1cc;
	seperateDigit(value, config.q1Value);
}

void initialization()
{
	memset(&current, 0, sizeof(current));
	memset(&config, 0, sizeof(config));

	if (readFlash() == 0 || flash.meterType < DN15 || flash.meterType > DN50) {
		current.meter_fault = 1;
		current.CC_modifyProtectTime = 0;
		lcdFlag.needUpdate = 1;
		lcdFlag.updateStartPos = 0;
		return;
	}

	memcpy(config.serial, flash.serial, 4);

	pMeterInfo = (meter_config_t *)&meter_config[flash.meterType - 1];

	set_q3Value();

	memcpy(current.value, flash.value, VALUE_DIGIT_LEN);
}

void send_shSmallResp(u_char batt, u_char status)
{
	SH_meter_data_resp_t *p = (SH_meter_data_resp_t *)txBuf.buf;

	p->irq_ack = IRQ_ACK;
	p->stx = STX;
	p->len = LEN_SMALL;
	memcpy(p->serial, config.serial, 4);

	int pos = 1;
	for (int i = 3; i >= 0; i--) {
		p->value[i] = (current.value[pos] << 4) + current.value[pos + 1];
		pos += 2;
	}

	p->batt = batt;
	p->status = status;

	p->etx = ETX;

	p->checksum = 0;
	u_char len = (u_char)(&p->checksum - &p->stx);
	u_char *pBody = &p->stx;
	for (int i = 0; i < len; i++) {
		p->checksum ^= *pBody++;
	}

	txBuf.len = sizeof(SH_meter_data_resp_t);

	startTx(TA0_TX600_MODE);
}

void sendSHMeterDataResp()
{
	u_char batt = current.batt;
	if (current.batt >= 2) {
		batt = 2; // shinhan batt 2 is full
	}

	u_char status = 0xC5; // reserved bit all 1
	if (current.meter_fault == 1) {
		status = 0xFF; // meter fail을 의미
	} else {
		if (current.status.q4Alarm) {
			status = status | MAX_WARNING;
		}
		if (current.status.leak) {
			status = status | LEAK_WARNING;
		}
		if (current.status.reverseFlow) {
			status = status | REVERSE_WARNING;
		}
		if (longWaiting()) {
			status = status | LONGWAIT_WARNING;
		}
	}

	send_shSmallResp(batt, status);
}

void sendMeterDataResp()
{
	meter_data_resp_t *p = (meter_data_resp_t *)txBuf.buf;

	p->start1 = p->start2 = RESP_START;
	p->l1_field = p->l2_field = RESP_LFIELD;
	p->c_field = METER_DATA_RESP;
	p->a_field = current.a_field;
	p->ci_field = CI_FIXED_VALUE;
	p->data.mdh = MDH_FIXED_VALUE;
	memcpy(p->data.serial, config.serial, 4);

	if (current.meter_fault == 1) {
		p->data.status = 0xFF; // meter fault 시 0xFF 전송
	} else {
		memcpy(&p->data.status, &current.status, 1);
	}
	p->data.meterType = flash.meterType;
	p->data.dataSize = BCD_8_DIGITS;
	p->data.dataUnit = UNIT_M3;
	if (p->data.meterType == DN50) {
		p->data.pointPos = POINT_POS_2;
	} else {
		p->data.pointPos = POINT_POS_3;
	}

	int pos = 0;
	if (p->data.meterType != DN50) {
		pos = 1;
	}

	for (int i = 3; i >= 0; i--) {
		p->data.data[i] = (current.value[pos] << 4) + current.value[pos + 1];
		pos += 2;
	}

	p->checksum = Cal_Checksum(&p->c_field, (u_char)(&p->checksum - &p->c_field));

	p->stop = RESP_STOP;

	txBuf.len = sizeof(meter_data_resp_t);

	startTx(TA0_TX1200_MODE);
}

void sendMeterStatusResp(u_char result)
{
	meter_status_resp_t *p = (meter_status_resp_t *)txBuf.buf;

	p->start1 = p->start2 = RESP_START;
	p->l1_field = p->l2_field = RESP_LFIELD;
	p->c_field = METER_STATUS_RESP;
	p->meterType = flash.meterType;
	memcpy(p->serial, flash.serial, 4);

	memcpy(p->cc, &flash.cc, 2);

	u_int value = flash.cc;

	value = flash.cc + flash.qtcc;
	memcpy(p->qtcc, &value, 2);

	value = flash.cc + flash.q2cc;
	memcpy(p->q2cc, &value, 2);

	value = flash.cc + flash.q1cc;
	memcpy(p->q1cc, &value, 2);

	// p->qtcc[0] = p->qtcc[1] = 0;

	p->maker = MAKER_NUM;
	p->ver = VERSION_NUMBER;
	p->result = result;

	p->checksum = Cal_Checksum(&p->c_field, (u_char)(&p->checksum - &p->c_field));

	p->stop = RESP_STOP;

	txBuf.len = sizeof(meter_status_resp_t);

	startTx(TA0_TX1200_MODE);
}

void sendMeterStatusDataResp(u_char result)
{
	meter_status_data_resp_t *p = (meter_status_data_resp_t *)txBuf.buf;

	p->start1 = p->start2 = RESP_START;
	p->l1_field = p->l2_field = RESP_LFIELD;
	p->c_field = METER_STATUS_DATA_RESP;
	p->meterType = flash.meterType;
	memcpy(p->serial, flash.serial, 4);

	memcpy(p->cc, &flash.cc, 2);

	u_int value = flash.cc;

	value = flash.cc + flash.qtcc;
	memcpy(p->qtcc, &value, 2);

	value = flash.cc + flash.q2cc;
	memcpy(p->q2cc, &value, 2);

	value = flash.cc + flash.q1cc;
	memcpy(p->q1cc, &value, 2);

	p->maker = MAKER_NUM;
	p->firmware_ver = VERSION_NUMBER;

	for (int i = 5; i >= 0; i--) {
		p->meter_value[i] =
			(current.value[(5 - i) * 2 + 0] << 4) + current.value[(5 - i) * 2 + 1];
	}

	p->checksum = Cal_Checksum(&p->c_field, (u_char)(&p->checksum - &p->c_field));

	p->stop = RESP_STOP;

	txBuf.len = sizeof(meter_status_data_resp_t);

	startTx(TA0_TX1200_MODE);
}

void display_initialScreen()
{
	LCD_allOnOff(0);
	delay_100msec(10); // delay 1.0 sec

	if (current.meter_fault) {
		return;
	}

	BYTE string[11];
	memset(string, 0xff, 11);

	u_char caliber = 0;
	switch (flash.meterType) {
	case DN15:
		caliber = 15;
		break;
	case DN20:
		caliber = 20;
		break;
	case DN25:
		caliber = 25;
		break;
	case DN32:
		caliber = 32;
		break;
	case DN40:
		caliber = 40;
		break;
	case DN50:
		caliber = 50;
		break;
	}

	string[0] = caliber / 10;
	string[1] = caliber % 10;

	int nonZeroFound = 0;
	for (int i = 0; i < 4; i++) {
		if (nonZeroFound == 0) {
			if (config.q3Value[i]) {
				nonZeroFound = 1;
			}
		}

		if (config.q3Value[i] || nonZeroFound) {
			string[3 + i] = config.q3Value[i];
		}
	}

	string[9] = MAKER_NUM / 0x10;
	string[10] = MAKER_NUM % 0x10;

	LCD_allOnOff(0);
	LCD_TURN_ON_POINT();
	LCD_Pattern_Display(string);

	delay_100msec(10); // delay 1.5 sec

	LCD_TURN_ON_FWD_ARROW();
	LCD_Flow_ICON();
	readBattery();
}

void checkMessage()
{
	u_char checksum = 0;
	meter_data_req_t *p = (meter_data_req_t *)rxBuf.buf;

	if (p->c_field == (METER_DATA_REQ)) { // 5B
		if (p->checksum != (p->c_field + p->a_field)) {
			return;
		}
		if (current.meter_fault != 0) {
			return;
		}

		current.a_field = p->a_field;
		sendMeterDataResp();
	} else if (p->c_field == METER_DATA_SET) { // A1 Current Value set
		if (current.CC_modifyProtectTime > CC_MODIFY_PROTECT_TIME) {
			return;
		}

		meter_data_set_t *pm = (meter_data_set_t *)rxBuf.buf;
		checksum = Cal_Checksum(&pm->c_field, (&pm->checksum - &pm->c_field));
		if (checksum != pm->checksum) {
			return;
		}

		for (int i = 0; i < 6; i++) {
			current.value[2 * i] = pm->value[5 - i] >> 4;
			current.value[2 * i + 1] = pm->value[5 - i] & 0x0f;
			current.value[12] = 0;
		}
		saveMeterValue();
		sendMeterDataResp();

		lcdFlag.needUpdate = 1;
		lcdFlag.updateStartPos = 0;
		current.CC_modifyProtectTime = 0;

	} else if (p->c_field == METER_STATUS_REQ) { // A2    Status
		if (p->checksum != (p->c_field + p->a_field)) {
			return;
		}

		sendMeterStatusResp(0);
	} else if (p->c_field == METER_STATUS_DATA_REQ) { // A3
		if (p->checksum != (p->c_field + p->a_field)) {
			return;
		}

		sendMeterStatusDataResp(0);
	} else if (p->c_field == METER_LCD_TEST) { // A6 LCD TEST
		meter_lcd_req_t *pl = (meter_lcd_req_t *)rxBuf.buf;
		if (pl->stop != REQ_STOP) {
			return;
		}
		checksum = Cal_Checksum(&pl->c_field, (&pl->checksum - &pl->c_field));
		if (checksum != pl->checksum) {
			return;
		}

		if (pl->data == LCD_DISPLAY_ALL) {
			if (pl->time > 20) {
				pl->time = 20;
			}
			if (pl->time == 0) {
				pl->time = 2;
			}

			pl->time *= 10;

			_BIC_SR(GIE);
			LCD_allOnOff(1);
			delay_100msec(pl->time); // delay lcd timer

			LCD_allOnOff(0);
			LCD_TURN_ON_FWD_ARROW();
			LCD_Flow_ICON();
			LCD_TURN_ON_M3();
			LCD_TURN_OFF_LEAK();
			LCD_TURN_OFF_NO_USE();
			LCD_TURN_ON_POINT();
			readBattery();

			lcdFlag.needUpdate = 1;
			lcdFlag.updateStartPos = 0;
			_BIS_SR(GIE);
		}
	} else if (p->c_field == METER_LCD_SET) { // A7 Set Lcd
		meter_lcd_set_req_t *pls = (meter_lcd_set_req_t *)rxBuf.buf;
		if (pls->checksum == (pls->c_field + pls->a_field + pls->data)) {
			flash.sleepStatus = pls->data;
			if (pls->data == 0) {
				LCD_TURN_OFF_H();
			} else if (pls->data == 1) {
				LCD_TURN_ON_H();
			}
		}
	} else if (p->c_field == SET_SERIAL_NUMBER) { // A0 Set Serial
		set_serial_number_t *ps = (set_serial_number_t *)&rxBuf.buf;

		if (ps->stop != REQ_STOP) {
			return;
		}
		checksum = Cal_Checksum(&ps->c_field, (&ps->checksum - &ps->c_field));
		if (checksum != ps->checksum) {
			return;
		}
		int errCode = ERR_NONE;

		do {
			if (current.CC_modifyProtectTime > CC_MODIFY_PROTECT_TIME) {
				errCode = ERR_MODIFY_PROTECTED;
				break;
			}

			if ((ps->maker != MAKER_NUM) && (ps->maker != 0)) {
				errCode = NO_MATCH_MAKER;
				break;
			}

			if (ps->meterType == 0) {
				ps->meterType = flash.meterType;
			}

			if (ps->meterType < DN15 || ps->meterType > DN50) {
				errCode = ERR_CALIBER;
				break;
			}

			if (flash.meterType != ps->meterType) {
				flash.meterType = ps->meterType;
				pMeterInfo = (meter_config_t *)&meter_config[ps->meterType - 1];
				flash.cc = pMeterInfo->cc;
				flash.qtcc = pMeterInfo->qt.cc;
				flash.q2cc = pMeterInfo->q2.cc;
				flash.q1cc = pMeterInfo->q1.cc;
				memset(current.value, 0, VALUE_DIGIT_LEN);
				memset(flash.value, 0, VALUE_DIGIT_LEN);
			}

			u_int value = 0;
			u_int q3 = flash.cc;
			u_char cc_change = 0;

			memcpy(&value, ps->cc, 2);
			if (value) {
				if (checkValid(value)) {
					flash.cc = value;
					q3 = value;
					cc_change = 1;
				} else {
					errCode = SET_ERR_MORE_THAN_50P;
					break;
				}
			}

			memcpy(&value, ps->qtcc, 2);
			if (value) {
				if (checkValid(value)) {
					flash.qtcc = value - q3;
					cc_change = 1;
				} else {
					errCode = SET_ERR_MORE_THAN_50P;
					break;
				}
			}

			memcpy(&value, ps->q2cc, 2);
			if (value) {
				if (checkValid(value)) {
					flash.q2cc = value - q3;
					cc_change = 1;
				} else {
					errCode = SET_ERR_MORE_THAN_50P;
					break;
				}
			}

			memcpy(&value, ps->q1cc, 2);
			if (value) {
				if (checkValid(value)) {
					flash.q1cc = value - q3;
					cc_change = 1;
				} else {
					errCode = SET_ERR_MORE_THAN_50P;
					break;
				}
			}

			if (cc_change) {
				memset(current.value, 0, VALUE_DIGIT_LEN);
				memset(flash.value, 0, VALUE_DIGIT_LEN);
			}

			if (ps->serial[0] + ps->serial[1] + ps->serial[2] + ps->serial[3]) {
				memcpy(flash.serial, ps->serial, 4);
				memcpy(flash.value, current.value, VALUE_DIGIT_LEN);
			}

			if (current.meter_fault == 1) {
				flash.sleepStatus = 0;
				current.meter_fault = 0;
			}

			writeFlash();
		} while (0);

		sendMeterStatusResp(errCode);
		delay_100msec(5);

		if (errCode == ERR_NONE) {
			REBOOT_SYSTEM();
		}
	}
}

void endComm(void)
{
	nomal_mode_count = 0;
	_BIC_SR(GIE); // disable interrupt
	memset(&rxBuf, 0, sizeof(rxBuf));
	memset(&rxData, 0, sizeof(rxData));
	memset(&txBuf, 0, sizeof(txBuf));
	RxReady = 0;
	ENABLE_RX_INTERRUPT();
	CLEAR_PULSE_REQ(); // 20161025 Cho
	timerA_mode = TA0_IDLE_MODE;
	_BIS_SR(LPM3_bits + GIE);
}

/*************************************************************
//        Main Function Service
//        ----------------------
//
**************************************************************/
void main(void)
{
	STOP_WATCHDOG();
	_BIC_SR(GIE); // disable interrupt

	Port_initial(); // IO Port Initial svc
	Interrput_initial(); // Interrupt Initial svc
	RESTART_WATCHDOG();

	initialization();
	init_parameters();

	RESTART_WATCHDOG();

	restoreAfterPeriodicReset();

	if (current.meter_fault == 0) {
		if (flash.sleepStatus) {
			LCD_TURN_ON_H();
		}
	}
	LCD_TURN_ON_M3();
	LCD_TURN_ON_POINT();
	LCD_TURN_ON_FWD_ARROW();
	LCD_Flow_ICON();

	memset(&txBuf, 0, sizeof(txBuf));
	memset(&rxBuf, 0, sizeof(rxBuf));
	memset(&rxData, 0, sizeof(rxData));

	readBattery();

	lcdFlag.needUpdate = 1;
	lcdFlag.updateStartPos = 0;

	RxReady = 0;

	RESTART_WATCHDOG();

	ENABLE_RX_INTERRUPT();
	CLEAR_PULSE_REQ(); // 20161025 Cho

	_BIS_SR(LPM3_bits + GIE); // Enter LPM3, enable interrupts

	while (1) {
		// RX Port 인터럽트 감지시 LPM3가 종료되면서 진입

		// 1초간 STD Start Byte 혹은 SH 검침 요청이 들어오지 않으면 LPM3 진입.
		if (pulseTypeReq.txResp == 0 && RxReady == 0) {
			delay_msec(20);
			if (++nomal_mode_count > 50) {
				endComm();
			}
			continue;
		}

		// STD 수신 종료 대기
		if (RxReady) {
			u_char prevPos = rxBuf.pos;
			delay_msec(20);
			while ((prevPos != rxBuf.pos) && (rxBuf.pos < LEN_RX_MSG)) {
				prevPos = rxBuf.pos;
				delay_msec(20);
			}
		}

		P1IE &= ~RX_PIN; // disable rx interrupt

		if (pulseTypeReq.txResp == 1) {
			delay_100msec(5);
			sendSHMeterDataResp();
		} else {
			checkMessage();
		}

		for (int i = 0; i < 500; i++) {
			delay_msec(2);
			if (timerA_mode == TA0_IDLE_MODE) {
				break;
			}
		}

		P1OUT &= ~TX_PIN;

		endComm();
	}
}
