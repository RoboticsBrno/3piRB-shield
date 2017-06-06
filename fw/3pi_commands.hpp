#ifndef _3PI_COMMANDS_HPP_
#define _3PI_COMMANDS_HPP_

typedef uint8_t cmd_type;

// commands
const cmd_type CMD_NOP							=  0;
const cmd_type CMD_POWER_STEER					=  1; // for compatibility with radio control
const cmd_type CMD_POWER_TANK					=  2;
const cmd_type CMD_DISP_PRINT					=  3;
const cmd_type CMD_DISP_SET						=  4;
const cmd_type CMD_BUZZER						=  5;
const cmd_type CMD_LED							=  6;
const cmd_type CMD_CALIBRATION					=  7;
const cmd_type CMD_EEPROM_LOAD					=  8;
const cmd_type CMD_EEPROM_STORE					=  9;
const cmd_type CMD_EEPROM_SIZE					= 10;
const cmd_type CMD_PING							= 15;

const cmd_type CMD_DISP_SET_CLEAR				=  0;
const cmd_type CMD_DISP_SET_CURSOR_POSITION		=  1;
const cmd_type CMD_DISP_SET_CURSOR_HIDE			=  2;
const cmd_type CMD_DISP_SET_CURSOR_SHOW			=  3;
const cmd_type CMD_DISP_SET_CURSOR_BLINK		=  4;
const cmd_type CMD_DISP_SET_CURSOR_MOVE			=  5;
const cmd_type CMD_DISP_SET_SCROLL				=  6;
const cmd_type CMD_DISP_SET_CUSTOM_CHAR			=  7;
const cmd_type CMD_DISP_SET_CUSTOM_CMD			=  8;

// used for both LED and BUZZER
const cmd_type CMD_LED_SET						=  0;
const cmd_type CMD_LED_CLEAR					=  1;
const cmd_type CMD_LED_TOGGLE					=  2;
const cmd_type CMD_LED_ON						=  3;
const cmd_type CMD_LED_OFF						=  4;
const cmd_type CMD_LED_BLINK					=  5;
const cmd_type CMD_LED_IS_ON					=  6;
const cmd_type CMD_LED_IS_BLINKING				=  7;

const cmd_type CMD_CALIBRATION_RESET			=  0;
const cmd_type CMD_CALIBRATION_SENSORS			=  1;
const cmd_type CMD_CALIBRATION_ROUND			=  2;
const cmd_type CMD_CALIBRATION_STORE			=  3;
const cmd_type CMD_CALIBRATION_LOAD				=  4;


// single byte commands
const cmd_type SCMD_STOP						= 0x81;
const cmd_type SCMD_STOP_ALL					= 0x87;

const cmd_type SCMD_NOP							= 0x88;

const cmd_type SCMD_MEAS_STOP					= 0x8A;
const cmd_type SCMD_MEAS_START					= 0x8B;
const cmd_type SCMD_MEAS_SINGLE					= 0x8C;

const cmd_type SCMD_DISP_CLEAR					= 0x90;
const cmd_type SCMD_DISP_CURSOR_HIDE			= 0x91;
const cmd_type SCMD_DISP_CURSOR_SHOW			= 0x92;
const cmd_type SCMD_DISP_CURSOR_BLINK			= 0x93;
const cmd_type SCMD_DISP_CURSOR_MOVE_LETF		= 0x94;
const cmd_type SCMD_DISP_CURSOR_MOVE_RIGHT		= 0x95;
const cmd_type SCMD_DISP_SCROLL_LEFT			= 0x96;
const cmd_type SCMD_DISP_SCROLL_RIGHT			= 0x97;

const cmd_type SCMD_LED_SET						= 0xA0;
const cmd_type SCMD_LED_CLEAR					= 0xA1;
const cmd_type SCMD_LED_TOGGLE					= 0xA2;
const cmd_type SCMD_LED_ON						= 0xA3;
const cmd_type SCMD_LED_OFF						= 0xA4;
const cmd_type SCMD_LED_IS_ON					= 0xA5;
const cmd_type SCMD_LED_IS_BLINKING				= 0xA6;

const cmd_type SCMD_BUZZER_SET					= 0xB0;
const cmd_type SCMD_BUZZER_CLEAR				= 0xB1;
const cmd_type SCMD_BUZZER_TOGGLE				= 0xB2;
const cmd_type SCMD_BUZZER_ON					= 0xB3;
const cmd_type SCMD_BUZZER_OFF					= 0xB4;
const cmd_type SCMD_BUZZER_IS_ON				= 0xB5;
const cmd_type SCMD_BUZZER_IS_BLINKING			= 0xB6;


// responses
const cmd_type RESP_ACK							=  0;
const cmd_type RESP_ERROR						=  1;
const cmd_type RESP_SENSORS						=  2;

const cmd_type RESP_LEDS						=  8; // reserved for compatibility with radio control


const cmd_type RESP_ERROR_OK					=  0;
const cmd_type RESP_ERROR_UNKNOWN_CMD			=  1;
const cmd_type RESP_ERROR_UNKNOWN_SUBCMD		=  2;
const cmd_type RESP_ERROR_MISSING_SUBCMD		=  3;
const cmd_type RESP_ERROR_INVALID_LENGTH		=  4;


// single byte responses
const cmd_type SRESP_ACK						= 'A';
const cmd_type SRESP_UNKNOWN_CMD				= '?';

const cmd_type SRESP_LED_ON						= 'l';
const cmd_type SRESP_LED_OFF					= 'd';
const cmd_type SRESP_LED_BLINKING				= 'f';
const cmd_type SRESP_LED_NOT_BLINKING			= 'c';

const cmd_type SRESP_BUZZER_ON					= 'b';
const cmd_type SRESP_BUZZER_OFF					= 's';
const cmd_type SRESP_BUZZER_BLINKING			= 'i';
const cmd_type SRESP_BUZZER_NOT_BLINKING		= 'p';

#endif
