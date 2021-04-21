#include "intsvc.h"

#include <MSP430x41x.h>
#include <stddef.h>
#include <string.h>

#include "drive.h"
#include "flash.h"
#include "global.h"
#include "iodefine.h"
#include "lcd.h"
#include "main.h"

#define ALARM_VALID_10SEC2HOUR 360 //(3600 / 10) // one hour
#define ALARM_VALID_MIN2HOUR 60 // one hour

extern current_data_t current;
extern config_t config;
extern rxData_t rxData;
extern rxBuf_t rxBuf;
extern txBuf_t txBuf;
extern u_char RxReady;
extern lcdFlag_t lcdFlag;
extern flash_t flash;
volatile flag_t flag;

extern meter_config_t *pMeterInfo;

volatile u_char timerA_mode = TA0_IDLE_MODE;
u_int detectInterval = 0;

struct {
	u_int tempAmountLiter;
	u_int flashBackupAmountLiter;
	u_char flashBackupCounter : 7, flashBackupValue : 1;
} backup;

// 8 bits counter
struct {
	u_int fastMode;
	u_char stateForward;
	u_char rxWait;
	u_char tenMinute;
	u_int q4_rps;
	u_char preReverse;
} counter8;

// 16 bits counter
struct {
	int q4Alarm;
	int reverseTimeout;
	int leak;
	int noUse;
} counter16;

const struct {
	u_char currLevel;
	u_char oldLevel;
	u_char nextState;
	u_char rvsState;
} forward[4] = { { MRSENSOR_CASE0, MRSENSOR_CASE3, STATUS_1, STATUS_3 },
		 { MRSENSOR_CASE1, MRSENSOR_CASE0, STATUS_2, STATUS_0 },
		 { MRSENSOR_CASE2, MRSENSOR_CASE1, STATUS_3, STATUS_1 },
		 { MRSENSOR_CASE3, MRSENSOR_CASE2, STATUS_0, STATUS_2 } };

const struct {
	u_char currLevel;
	u_char oldLevel;
	u_char nextState;
	u_char rvsState;
} reverse[4] = { { MRSENSOR_CASE0, MRSENSOR_CASE1, STATUS_3, STATUS_1 },
		 { MRSENSOR_CASE1, MRSENSOR_CASE2, STATUS_0, STATUS_2 },
		 { MRSENSOR_CASE2, MRSENSOR_CASE3, STATUS_1, STATUS_3 },
		 { MRSENSOR_CASE3, MRSENSOR_CASE0, STATUS_2, STATUS_0 } };

#define MAGIC_NUMBER 0x12AB
__no_init __data16 u_int magicNumber;
__no_init __data16 u_int CC_modifyProtectTime;
__no_init __data16 u_int counter16_leak;
__no_init __data16 u_int counter16_noUse;
__no_init __data16 u_int nRxMessageStored;
extern int nRxMessage;

void saveAndPeriodicReset()
{
	if (current.meter_fault) {
		return;
	}

	STOP_WATCHDOG();
	IE2 &= ~BTIE; // Enable BT interrupt
	P1IE = 0;
	_BIC_SR(GIE); // disable interrupt

	// save data in flash memory
	saveMeterValue();

	// save parameter in uninitialized area
	magicNumber = MAGIC_NUMBER;
	CC_modifyProtectTime = current.CC_modifyProtectTime;
	counter16_leak = counter16.leak;
	counter16_noUse = counter16.noUse;
	nRxMessageStored = nRxMessage;

	// reset
	REBOOT_SYSTEM();
}

void restoreAfterPeriodicReset()
{
	if (magicNumber != MAGIC_NUMBER) {
		display_initialScreen();
		nRxMessageStored = nRxMessage = 0;
		return;
	}

	LCD_allOnOff(0);

	magicNumber = 0; // clear magic nnumber

	nRxMessage = nRxMessageStored;

	current.CC_modifyProtectTime = CC_modifyProtectTime;

	counter16.leak = counter16_leak;
	if (counter16.leak >= LEAK_LIMIT) {
		current.status.leak = TRUE;
		LCD_TURN_ON_LEAK();
	} else {
		current.status.leak = FALSE;
		LCD_TURN_OFF_LEAK();
	}

	counter16.noUse = counter16_noUse;
	if (counter16.noUse >= NO_USE_DAY) {
		flag.longWaiting = 1; // Long waiting flag
	} else {
		flag.longWaiting = 0; // Long waiting flag
	}
}

pulse_type_req_t pulseTypeReq;

int longWaiting()
{
	return flag.longWaiting;
}

void memset_volatile(volatile void *s, char c, size_t n)
{
	volatile char *p = s;
	while (n-- > 0) {
		*p++ = c;
	}
}

void init_parameters()
{
	memset(&backup, 0, sizeof(backup));
	memset_volatile(&flag, 0, sizeof(flag));

	memset(&counter8, 0, sizeof(counter8));
	memset(&counter16, 0, sizeof(counter16));

	CLEAR_PULSE_REQ();
	flag.longWaiting = 1;
	counter16.noUse = NO_USE_DAY;
}

void controlBackupAndAlarm()
{
	// this function is called every 10 minute

	readBattery();

	// 톤은 여섯자리로 표시되나 하위 3자리만 확인해도 3톤을 넘는지 알 수 있음
	int tonUsed = current.value[TON_DIGIT_LEN - 3] * 100 +
		      current.value[TON_DIGIT_LEN - 2] * 10 + current.value[TON_DIGIT_LEN - 1];
	if (tonUsed >= 3) {
		// 사용량이 3톤이 넘은 후에 시간을 카운트하기 시작함.
		if (current.CC_modifyProtectTime <= CC_MODIFY_PROTECT_TIME) {
			current.CC_modifyProtectTime++;
		}
	}

	// check leak alarm
	if (backup.tempAmountLiter == 0) {
		counter16.leak = 0;
		current.status.leak = FALSE;
		LCD_TURN_OFF_LEAK();
	} else {
		if (++counter16.leak > LEAK_LIMIT) { // 10 min * 1008 = 7 days
			counter16.leak--;
			current.status.leak = TRUE;
			LCD_TURN_ON_LEAK();
		}
	}

	backup.flashBackupAmountLiter += backup.tempAmountLiter;
	backup.tempAmountLiter = 0;

	backup.flashBackupCounter++;
	if (backup.flashBackupCounter >= 90) { // 90 * 10 min = 15 hours
		if (tonUsed >= 3) {
			saveAndPeriodicReset();
		}
	} else {
		if (backup.flashBackupAmountLiter >= pMeterInfo->saveLimit) {
			lcdFlag.needUpdate = 1;
			lcdFlag.updateStartPos = 0;

			backup.flashBackupCounter = 0;
			backup.flashBackupAmountLiter = 0;
			saveMeterValue();
		}
	}
}

void getOneByte()
{
	rxData.bitCount = 9;

	TACCR0 = TAR; // Current state of TA counter
	TACCR0 += BITIME12_5;
	TACCTL0 |= CCIE;
}

void sendNextData()
{
	if (txBuf.bytePos >= txBuf.len) {
		TACCTL0 &= ~CCIE;
		timerA_mode = TA0_IDLE_MODE;
	} else {
		// 응답 메시지 송신을 원활하게 하기 위해 Basic Timer Interrupt를 중지시키면
		// 임펠러 회전 측정이 일시 중지되므로 정확도가 떨어지고, LCD의 회전판이
		// 멈칫거림.
		txBuf.txData = txBuf.buf[txBuf.bytePos++];
		txBuf.bitPos = 11; // start(1) + data(8) + stop(2)
		TACCR0 = TAR; // Current state of TA counter
		TACCR0 += BITIME12;
		TACCTL0 |= CCIE;
	}
}

void startTx(u_char mode)
{
	P1OUT |= TX_PIN;

	delay_msec(20);

	txBuf.bytePos = 0;
	sendNextData();
	timerA_mode = mode;
}

void readBattery()
{
	u_char battLevel = BATTERY_H;

	P1DIR |= 0x28; // BATSEL3 & BATSEL1 Output Direction
	P1OUT &= (~P_BATSEL1 & ~P_BATSEL3); // Detect level 3.45V setting
	CACTL1 = CACTL1 | CAON; // Comparator A on
	__no_operation();
	__no_operation();
	__no_operation();
	if (CACTL2 & CAOUT) {
		P1DIR &= ~0x38; // PORT P1 Direction
		CACTL1 = CACTL1 & ~CAON; // Comparator A off
		battLevel = BATTERY_H; // Battery level high set
	} else {
		P1DIR &= ~0x20; // PORT P1 Direction
		P1DIR |= P_BATSEL2; // BATSEL2 Output Direction
		P1OUT &= (~P_BATSEL2 & ~P_BATSEL3); // Detect level 3.3V setting
		__no_operation();
		if (CACTL2 & CAOUT) {
			P1DIR &= ~0x38; // PORT P1 Direction
			CACTL1 = CACTL1 & ~CAON; // Comparator A off
			battLevel = BATTERY_M; // Battery level middle set
		} else {
			P1DIR &= ~0x10; // PORT P1 Direction
			PBATSEL3_CLR; // Detect level 3.15V setting
			__no_operation();
			if (CACTL2 & CAOUT) {
				battLevel = BATTERY_L; // Battery level low set
			} else {
				battLevel = BATTERY_EMPTY; // Battery level empty set
			}
			P1DIR &= ~0x38; // PORT P1 Direction
			CACTL1 = CACTL1 & ~CAON; // Comparator A off
		}
	}
	if (battLevel >= BATTERY_M) {
		current.status.lowBatt = FALSE;
	} else {
		current.status.lowBatt = TRUE;
	}
	current.batt = battLevel;
	LCD_Battery_ICON_Display(battLevel); // Battery ICON Display
}

void decideQ1Q2()
{
	flag.qtDetected = 0;
	flag.q2Detected = 0;
	flag.q1Detected = 0;

	if (detectInterval > pMeterInfo->q1.speedLimit_L) {
		flag.q1Detected = 1;
	} else if (detectInterval > pMeterInfo->q2.speedLimit_L) {
		flag.q2Detected = 1;
	} else if (detectInterval > pMeterInfo->qt.speedLimit_L) {
		flag.qtDetected = 1;
	}

	detectInterval = 0;
}

void incMeterValue()
{
	u_char *value = config.q3Value;

	if (flag.qtDetected) {
		value = config.qtValue;
	} else if (flag.q2Detected) {
		value = config.q2Value;
	} else if (flag.q1Detected) {
		value = config.q1Value;
	}

	u_char overflow = 0;

	lcdFlag.needUpdate = 1;
	if (lcdFlag.updateStartPos > DISPLAY_POS_CC) {
		lcdFlag.updateStartPos = DISPLAY_POS_CC;
	}

	// subCC
	current.value[12] += value[3];
	if (current.value[12] >= 10) {
		current.value[12] -= 10;
		overflow = 1;
	} else {
		overflow = 0;
	}

	// 1cc
	current.value[11] += (value[2] + overflow);
	if (current.value[11] >= 10) {
		current.value[11] -= 10;
		overflow = 1;
	} else {
		overflow = 0;
	}

	// 10cc
	current.value[10] += (value[1] + overflow);
	if (current.value[10] >= 10) {
		current.value[10] -= 10;
		overflow = 1;
	} else {
		overflow = 0;
	}

	// 100cc
	current.value[9] += (value[0] + overflow);
	if (current.value[9] >= 10) {
		current.value[9] -= 10;
		overflow = 1;
	} else {
		return;
	}

	// Liter increase
	backup.tempAmountLiter++;
	if (lcdFlag.updateStartPos > DISPLAY_POS_LITER) {
		lcdFlag.updateStartPos = DISPLAY_POS_LITER;
	}

	if (++current.value[8] < 10)
		return; // 1 Liter

	current.value[8] = 0;
	if (++current.value[7] < 10)
		return; // 10 Liter

	current.value[7] = 0;
	if (++current.value[6] < 10)
		return; // 100 Liter

	lcdFlag.updateStartPos = DISPLAY_POS_TON;

	current.value[6] = 0;
	if (++current.value[5] < 10)
		return; // 1 ton

	current.value[5] = 0;
	if (++current.value[4] < 10)
		return; // 10 ton

	current.value[4] = 0;
	if (++current.value[3] < 10)
		return; // 100 ton

	current.value[3] = 0;
	if (++current.value[2] < 10)
		return; // 1000 ton

	current.value[2] = 0;
	if (++current.value[1] < 10)
		return; // 10000 ton

	current.value[1] = 0;
	if (++current.value[0] >= 10) { // 100000 ton
		current.value[0] = 0;
	}
}

/*************************************************************
//     Basic Timer interrupt
//    ---------------------
//    Sensor Normal : 32msec
//    Sensor Rotate : 4.0msec
**************************************************************/
#pragma vector = BASICTIMER_VECTOR
__interrupt void BASICTIMER_interrupt(void)
{
	static volatile short eighth_turn = 0;
	static volatile short reverse_halfturn = 0;
	static volatile u_char state_forward = 0;
	static volatile u_char state_reverse = 0;
	static volatile u_char store_sensor_old = 0;
	static volatile u_char time_sec = 0;
	static volatile u_int reverse_volume = 0;
	static volatile u_int msec = 0;

	u_char sensor = 0;
	u_char sensor_old = 0;
	u_char msecStep = 32;

	IFG2 &= 0x7F;
	RESTART_WATCHDOG();
	MRPOWER_SET;
	__no_operation();
	__no_operation();
	__no_operation();

	sensor = P6IN; // MR Sensor Input Data
	MRPOWER_CLR;
	sensor &= 0x06; // MR Sensor Input Data
	sensor_old = store_sensor_old;

	if (sensor != sensor_old) {
		flag.fastMode = 1;
		counter8.fastMode = 0; // Basic Timer fast mode end count

		// check forward direction
		if (sensor == forward[state_forward].currLevel &&
		    sensor_old == forward[state_forward].oldLevel) {
			eighth_turn++;

			state_forward = forward[state_forward].nextState;
			state_reverse = forward[state_forward].rvsState;
		} else if (sensor == forward[(state_forward + 1) & 0x03].currLevel &&
			   sensor_old == forward[state_forward].oldLevel) {
			eighth_turn += 2;

			state_forward = forward[(state_forward + 1) & 0x03].nextState;
			state_reverse = forward[(state_forward + 1) & 0x03].rvsState;
		}
		// check reverse direction
		else if (sensor == reverse[state_reverse].currLevel &&
			 sensor_old == reverse[state_reverse].oldLevel) {
			if (++counter8.preReverse >= 4) {
				counter8.preReverse = 0;
				reverse_halfturn++;
			}

			state_forward = reverse[state_reverse].rvsState;
			state_reverse = reverse[state_reverse].nextState;
		}

		store_sensor_old = sensor;

		if (eighth_turn > 3) {
			counter8.preReverse = 0;
			counter8.stateForward = TRUE;
			reverse_volume = 0;
			flag.halfTurn++;
			eighth_turn -= 4;
			LCD_TURN_ON_FWD_ARROW();
		}
	}

	if (flag.fastMode) {
		msecStep = 4;

		if (++counter8.fastMode > 30) { // 4 * 30 = 120 msec
			counter8.fastMode = 0;
			flag.fastMode = 0;
			BTCTL = BT_fLCD_512 + BT_ADLY_32;
		} else {
			BTCTL = BT_fLCD_512 + BT_ADLY_4;
		}
	} else {
		msecStep = 32;
		BTCTL = BT_fLCD_512 + BT_ADLY_32;
	}

	msec += msecStep;

	if ((detectInterval & 0x8000) == 0) {
		detectInterval += msecStep;
	}

	if (pulseTypeReq.flag) {
		pulseTypeReq.msec += msecStep;
	}

	volatile static char count = 0;
	if (flag.halfTurn >= TURN_COUNT) {
		count += TURN_COUNT;

		if ((count & 1) == 0) //짝수
		{
			decideQ1Q2();
		}

		if (count >= 4) {
			LCD_Flow_ICON();
			count = 0;
		}

		flag.halfTurn -= TURN_COUNT;
		counter8.q4_rps++;
		flag.longWaiting = 0;
		counter16.noUse = 0;
		incMeterValue();

	} else if (reverse_halfturn >= TURN_COUNT) {
		reverse_halfturn -= TURN_COUNT;
		reverse_volume += flash.cc;
		if (reverse_volume > 10000) {
			reverse_volume = 0;
			current.status.reverseFlow = TRUE;
			counter8.stateForward = FALSE;
			LCD_TURN_ON_RVS_ARROW();
		}
	}

	if (msec >= 10000) { // 10 sec
		msec -= 10000;
		if (counter8.q4_rps >= pMeterInfo->q4_rps) { // start q4 Alarm
			counter16.q4Alarm = 0;
			current.status.q4Alarm = TRUE;
		} else {
			if (++counter16.q4Alarm > ALARM_VALID_10SEC2HOUR) { // 1 hour
				counter16.q4Alarm--;
				current.status.q4Alarm = FALSE;
			}
		}
		counter8.q4_rps = 0;

		if (flag.longWaiting) {
			LCD_TURN_ON_NO_USE();
		} else {
			LCD_TURN_OFF_NO_USE();
		}

		time_sec += 10;
		if (time_sec >= 60) {
			flag.oneMinuteElapsed++;
			time_sec = 0;
		}
	}

	if (flag.oneMinuteElapsed) {
		flag.oneMinuteElapsed = 0;
		if (current.status.reverseFlow) { // reverse flow alarm status
			if (++counter16.reverseTimeout >= ALARM_VALID_MIN2HOUR) {
				if (counter8.stateForward == TRUE) {
					counter16.reverseTimeout = 0;
					current.status.reverseFlow = FALSE;
				}
			}
		}

		if (++counter8.tenMinute >= 10) {
			counter8.tenMinute = 0;
			controlBackupAndAlarm();
		}

		if (counter16.noUse <= NO_USE_DAY) {
			counter16.noUse++;
		} else {
			flag.longWaiting = 1; // Long waiting flag
		}
	}

	if (lcdFlag.needUpdate) {
		static u_char turn = 0;
		if (++turn > 10) {
			turn = 0;
			if (current.meter_fault == 1) {
				LCD_Display_Minus();
			} else {
				DisplayMeterValue(lcdFlag.updateStartPos);
			}
			lcdFlag.needUpdate = 0;
			lcdFlag.updateStartPos = VALUE_DIGIT_LEN;
		}
	}
}

#pragma vector = PORT2_VECTOR
__interrupt void PORT2_interrupt(void)
{
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_interrupt(void)
{
	if (P1IFG & RX_PIN) {
		P1IFG &= ~RX_PIN; // clear interrupt flag

		if (timerA_mode < TA0_TX600_MODE) {
			timerA_mode = TA0_RX_MODE;
			P1IE &= ~RX_PIN; // disable interrupt
			_BIC_SR_IRQ(LPM3_bits); // Clear LPM3 bits from 0(SR)
			getOneByte();
		}
	}
}

#pragma vector = TIMERA1_VECTOR
__interrupt void TIMERA1_interrupt(void)
{
}

/*************************************************************
//     Timer A CC0 interrupt
//    ----------------------
**************************************************************/
#pragma vector = TIMERA0_VECTOR
__interrupt void TIMERA0_interrupt(void)
{
	volatile static u_char toggle = 0;
	TACCR0 += BITIME12;

	if (timerA_mode == TA0_TX600_MODE) {
		if (++toggle & 0x01) {
			return;
		}
	}

	if (timerA_mode == TA0_RX_MODE) {
		if (rxData.bitCount) {
			rxData.bitCount--;
			if (rxData.bitCount != 8) {
				rxData.data >>= 1;
				if (P1IN & RX_PIN) {
					rxData.data |= 0x80;
				} else {
					rxData.data &= ~0x80;
				}
			}

			if (rxData.bitCount) {
				TACCTL0 |= CCIE;
			} else {
				if (rxBuf.pos > 0) {
					if (rxBuf.pos < LEN_RX_MSG) {
						rxBuf.buf[rxBuf.pos++] = rxData.data;
					} else {
						timerA_mode = TA0_IDLE_MODE;
					}
				} else if (rxBuf.pos == 0) {
					if (rxData.data == REQ_START) {
						rxBuf.buf[rxBuf.pos++] = rxData.data;
						RxReady = 1;
					} else if (rxData.data == 0x00) {
						if (pulseTypeReq.flag == 0) {
							memset(&pulseTypeReq, 0,
							       sizeof(pulseTypeReq));
							pulseTypeReq.flag = 1;
							pulseTypeReq.step =
								1; // step to measure width of low pulse
						} else if (pulseTypeReq.flag == 1) {
							if ((pulseTypeReq.msec > MIN_PULSE_WIDTH) &&
							    (pulseTypeReq.msec < MAX_PULSE_WIDTH)) {
								pulseTypeReq.txResp = 1;
							} else {
								memset(&pulseTypeReq, 0,
								       sizeof(pulseTypeReq));
								timerA_mode = TA0_IDLE_MODE;
							}
						}
					}
				}
				TACCTL0 &= ~CCIE; // All bits RXed, disable interrupt
				ENABLE_RX_INTERRUPT();
			}
		}
	} else if (timerA_mode == TA0_TX600_MODE || timerA_mode == TA0_TX1200_MODE) {
		if (txBuf.bitPos >= 11) { // start bit
			P1OUT &= ~TX_PIN;
		} else if (txBuf.bitPos < 3) { // stop bit
			P1OUT |= TX_PIN;
		} else {
			if (txBuf.txData & 0x01) {
				P1OUT |= TX_PIN;
			} else {
				P1OUT &= ~TX_PIN;
			}
			txBuf.txData >>= 1;
		}

		if (--txBuf.bitPos > 0) {
			TACCTL0 |= CCIE;
		} else {
			sendNextData();
		}
	}
}

#pragma vector = WDT_VECTOR
__interrupt void WDT_interrupt(void)
{
}

#pragma vector = COMPARATORA_VECTOR
__interrupt void COMPARATORA_interrupt(void)
{
}

#pragma vector = NMI_VECTOR
__interrupt void NMI_interrupt(void)
{
}
