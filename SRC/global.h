#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define ENABLE_BIT_DEFINITIONS

//****************************************************************
//  계량기 하부 제조업체 선택: 다음 중 하나만 1로 정의되어야 함
//  기능 버전 번호 확인 후 작업
//****************************************************************

#define TURN_COUNT 0x01

#define MAKER_MARK "HT" //"1"
#define MAKER_NUM 1 //"1"
#define VERSION_NUMBER 209 // X 100

#define TON_DIGIT_LEN 6
#define LITER_DIGIT_LEN 3
#define CC_DIGIT_LEN 3
#define SUB_CC_DIGIT_LEN 1
#define VALUE_DIGIT_LEN (TON_DIGIT_LEN + LITER_DIGIT_LEN + CC_DIGIT_LEN + SUB_CC_DIGIT_LEN)

#define DISPLAY_POS_TON 0
#define DISPLAY_POS_LITER 6
#define DISPLAY_POS_CC 9

// DN15, 20, 25, 32, 40, 50 등 6가지만 이 코드로 지원함
#define NUM_METER_TYPE 6
// meter type
#define DN15 0x01
#define DN20 0x02
#define DN25 0x03
#define DN32 0x04
#define DN40 0x05
#define DN50 0x06

// LCD Display Command
#define LCD_DISPLAY_ALL 1

// Error code for Calibration Response
#define ERR_NONE 0
#define ERR_CALIBER 1
#define ERR_MODIFY_PROTECTED 2
#define SET_ERR_MORE_THAN_50P 3
#define NO_MATCH_MAKER 4

typedef unsigned char BYTE;
typedef unsigned char u_char;
typedef signed char s_char;
typedef unsigned int INT16;
typedef unsigned int u_int;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long u_long;

#define TX_PIN 0x01
#define RX_PIN 0x02

#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0
#define HIGH 1
#define LOW 0

typedef struct {
	u_char bitCount;
	u_char data;
} rxData_t;

#define LEN_RX_MSG 21 // rx max - 19
#define LEN_TX_MSG 30 // tx max - 28

typedef struct {
	u_char pos;
	u_char buf[LEN_RX_MSG];
} rxBuf_t;

typedef struct {
	u_char len;
	u_char bytePos;
	u_char bitPos;
	u_char txData;
	u_char buf[LEN_TX_MSG];
} txBuf_t;

#define TA0_IDLE_MODE 0
#define TA0_RX_MODE 1
#define TA0_TX600_MODE 2
#define TA0_TX1200_MODE 3

#define REDRAW_LEVEL_NONE 0
#define REDRAW_LEVEL_CC 1
#define REDRAW_LEVEL_LITER 2
#define REDRAW_LEVEL_TON 3
#define REDRAW_LEVEL_ALL 4

typedef struct {
	u_char reserved1 : 1, frozen : 1, lowBatt : 1, reserved2 : 1, reserved3 : 1, leak : 1,
		reverseFlow : 1, q4Alarm : 1;
} status_t;

// message type
#define METER_DATA_REQ 0x5B // terminal --> meter
#define METER_DATA_RESP 0x08 // meter --> terminal

#define SET_SERIAL_NUMBER 0xA0 // terminal --> meter
#define METER_DATA_SET 0xA1 // terminal --> meter
#define METER_STATUS_REQ 0xA2 // terminal --> meter
#define METER_STATUS_DATA_REQ 0xA3 // terminal --> meter
#define METER_LCD_TEST 0xA6 // terminal --> meter
#define METER_LCD_SET 0xA7 // terminal --> meter

#define METER_STATUS_RESP 0xB2 // meter --> terminal
#define METER_STATUS_DATA_RESP 0xB3 // meter --> terminal

// frame delimiter (terminal --> meter)
#define REQ_START 0x10
#define REQ_STOP 0x16

// frame delimiter (meter --> terminal)
#define RESP_START 0x68
#define RESP_STOP 0x16
#define RESP_LFIELD 0x0F

// shinhan meter(meter --> terminal)
#define IRQ_ACK 0x06
#define STX 0x02
#define ETX 0x03
#define LEN_SMALL 0x0B

// shinhan status
#define MAX_WARNING 0x02
#define LEAK_WARNING 0x08
#define REVERSE_WARNING 0x10
#define LONGWAIT_WARNING 0x20

#define CI_FIXED_VALUE 0x78
#define MDH_FIXED_VALUE 0x0F
#define BCD_8_DIGITS 0x0C
#define UNIT_M3 1
#define POINT_POS_6 6
#define POINT_POS_3 3
#define POINT_POS_2 2

#define CC_MODIFY_PROTECT_TIME (u_int)(12960) // 12960 * 10minutes = 90 days
#define NO_USE_DAY (u_int)(5400) // 5400 minutes = 90h
#define LEAK_LIMIT (u_int)(1000) // 10 min * 1000 ~= 7 days

// rx - 5
typedef struct {
	u_char start;
	u_char c_field;
	u_char a_field;
	u_char checksum;
	u_char stop;
} meter_data_req_t;

// rx - 7
typedef struct {
	u_char start;
	u_char c_field;
	u_char a_field;
	u_char data;
	u_char time;
	u_char checksum;
	u_char stop;
} meter_lcd_req_t;

// rx - 11
typedef struct {
	u_char start;
	u_char c_field;
	u_char a_field;
	u_char value[6];
	u_char checksum;
	u_char stop;
} meter_data_set_t;

// part - 12
typedef struct {
	u_char mdh; // fixed value - 0x0f
	u_char serial[4]; // meter serial no(8 digits BCD)
	u_char status;
	u_char dataSize : 4, // 8 digits BCD - 0x0c
		meterType : 4; // 1 ~ 0x0c
	u_char pointPos : 4,
		dataUnit : 4; // m3 = 1
	u_char data[4];
} user_data_t;

// tx - 21
typedef struct {
	u_char start1;
	u_char l1_field;
	u_char l2_field;
	u_char start2;
	u_char c_field;
	u_char a_field;
	u_char ci_field;
	user_data_t data;
	u_char checksum;
	u_char stop;
} meter_data_resp_t;

// tx - 15
typedef struct {
	u_char irq_ack;
	u_char stx;
	u_char len;
	u_char serial[4];
	u_char value[4];
	u_char batt;
	u_char status;
	u_char etx;
	u_char checksum;
} SH_meter_data_resp_t;

// tx - 23
typedef struct {
	u_char start1;
	u_char l1_field;
	u_char l2_field;
	u_char start2;
	u_char c_field;
	u_char meterType;
	u_char serial[4];
	u_char cc[2];
	u_char qtcc[2];
	u_char q2cc[2];
	u_char q1cc[2];
	u_char maker;
	u_char ver;
	u_char result;
	u_char checksum;
	u_char stop;
} meter_status_resp_t;

// tx - 28
typedef struct {
	u_char start1;
	u_char l1_field;
	u_char l2_field;
	u_char start2;
	u_char c_field;
	u_char meterType;
	u_char serial[4];
	u_char cc[2];
	u_char qtcc[2];
	u_char q2cc[2];
	u_char q1cc[2];
	u_char maker; //제조사 추가
	u_char firmware_ver;
	u_char meter_value[6];
	u_char checksum;
	u_char stop;
} meter_status_data_resp_t;

// rx - 7
typedef struct {
	u_char start;
	u_char c_field;
	u_char a_field;
	u_char on_Hmark;
	u_char checksum;
	u_char stop;
} meter_lcd_set_req_t;

// rx - 19
typedef struct {
	u_char start;
	u_char c_field;
	u_char a_field;
	u_char meterType;
	u_char serial[4];
	u_char cc[2];
	u_char qtcc[2];
	u_char q2cc[2];
	u_char q1cc[2];
	u_char maker; //제조사 추가
	u_char checksum;
	u_char stop;
} set_serial_number_t;

typedef struct {
	u_char serial[4];
	u_char q3Value[4]; // CC: 3, SubCC: 1
	u_char qtValue[4];
	u_char q2Value[4];
	u_char q1Value[4];
} config_t;

typedef struct {
	status_t status;
	u_char a_field;
	u_char value[VALUE_DIGIT_LEN]; // ton, liter, cc
	u_char batt;
	u_int CC_modifyProtectTime;
	u_char meter_fault;
} current_data_t;

typedef struct {
	u_char halfTurn;
	u_char fastMode : 1, rotating : 1, oneMinuteElapsed : 1, longWaiting : 1, reverseFlow : 1,
		dataSending : 1, reserved1 : 2;
	u_char q1Detected : 1, q2Detected : 1, qtDetected : 1, reserved2 : 2, reserved3 : 3;
} flag_t;

typedef struct {
	u_char needUpdate : 1, updateStartPos : 7;
} lcdFlag_t;

typedef struct {
	int cc;
	u_int speedLimit_L;
} adjust_t;

typedef struct {
	u_int cc;
	u_int saveLimit;
	u_int q4_rps;
	adjust_t qt;
	adjust_t q2;
	adjust_t q1;
} meter_config_t;

#define MIN_PULSE_WIDTH (250 - 50)
#define MAX_PULSE_WIDTH (250 + 50)

typedef struct {
	volatile u_int msec;
	volatile u_char txResp : 1, flag : 1, step : 4;
} pulse_type_req_t;

// watchdog timer related definition
#define STOP_WATCHDOG()                                                                            \
	{                                                                                          \
		WDTCTL = WDTPW + WDTHOLD;                                                          \
	}
#define CLEAR_WATCHDOG()                                                                           \
	{                                                                                          \
		WDTCTL = WDTPW + WDTCNTCL;                                                         \
	}
#define START_WATCHDOG()                                                                           \
	{                                                                                          \
		WDTCTL = WDT_ARST_1000;                                                            \
	}
#define RESTART_WATCHDOG()                                                                         \
	{                                                                                          \
		WDTCTL = WDTPW + WDTCNTCL;                                                         \
		WDTCTL = WDT_ARST_1000;                                                            \
	}
#define REBOOT_SYSTEM()                                                                            \
	{                                                                                          \
		WDTCTL = ~WDTPW;                                                                   \
	}

#define ENABLE_RX_INTERRUPT()                                                                      \
	do {                                                                                       \
		P1IFG &= ~RX_PIN;                                                                  \
		P1IE |= RX_PIN;                                                                    \
	} while (0)

extern pulse_type_req_t pulseTypeReq;
#define CLEAR_PULSE_REQ()                                                                          \
	do {                                                                                       \
		memset(&pulseTypeReq, 0, sizeof(pulseTypeReq));                                    \
	} while (0)

#endif
