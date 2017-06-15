/*
 * 3piRB-3pi_fw.cpp
 *
 * Created: 7.7.2014 9:18:53
 *  Author: kubas
 */ 


#include "3piLibPack.h"
#include "../fw/avrlib/portd.hpp"
#include "../fw/avrlib/pin.hpp"
#include "../fw/avrlib/stopwatch.hpp"
#include "../fw/avrlib/command_parser.hpp"
#include "../fw/avrlib/math.hpp"

#include "time.hpp"

struct serial_t
{
	void write(char c) { rs232.sendCharacter(c); }
	bool empty() { return !rs232.peek(m_c); }
	char read() { return m_c; }
private:
	char m_c;
};
serial_t serial;

#include "../fw/3pi_commands.hpp"
#include "../fw/3pi_defines.hpp"
#include "../fw/led_impl.hpp"

template <typename Port, int Pin>
class led_t: public detail::led_impl_t
{
	typedef avrlib::pin<Port, Pin> m_pin;
public:
	void init() { m_pin::make_low(); }
	bool process() { m_pin::set_value(detail::led_impl_t::process()); return false; }
};
led_t<avrlib::portd, 7> led;

class advance_buzzer_t: public detail::led_impl_t
{
public:
	bool process()
	{
		if(detail::led_impl_t::process())
			buzzer.start();
		else
			buzzer.stop();
		return false;
	}
};
advance_buzzer_t advance_buzzer;

typedef avrlib::timed_command_parser<base_timer_type> cmd_parser_type;

void ack(cmd_parser_type& cmd_parser)
{
	if (cmd_parser.command() != CMD_PING)
		return;
	if (cmd_parser.state() == cmd_parser.simple_command)
	{
		serial.write(SRESP_ACK);
	}
	else
	{
		cmd_parser.write(cmd_parser.command());
		cmd_parser.send(serial, RESP_ACK);
	}
}

void unknown_subcommand(cmd_parser_type& cmd_parser)
{
	cmd_parser.write(RESP_ERROR_UNKNOWN_SUBCMD);
	cmd_parser.write(cmd_parser.command());
	cmd_parser.write(cmd_parser[0]);
	cmd_parser.send(serial, RESP_ERROR);
}

void missing_subcommand(cmd_parser_type& cmd_parser)
{
	cmd_parser.write(RESP_ERROR_MISSING_SUBCMD);
	cmd_parser.write(cmd_parser.command());
	cmd_parser.send(serial, RESP_ERROR);
}

void invalid_length(cmd_parser_type& cmd_parser, uint8_t expected)
{
	cmd_parser.write(RESP_ERROR_INVALID_LENGTH);
	cmd_parser.write(cmd_parser.command());
	cmd_parser.write(cmd_parser.size());
	cmd_parser.write(expected);
	cmd_parser.send(serial, RESP_ERROR);
}

bool led_remote_control(cmd_parser_type& cmd_parser, detail::led_impl_t* led)
{
	if (led == nullptr)
		return false;
	if (cmd_parser.size() == 0)
	{
		missing_subcommand(cmd_parser);
		return false;
	}
	switch(cmd_parser[0])
	{
	case CMD_LED_SET:
		if (cmd_parser.size() == 1)
			led->set();
		else
			led->set(cmd_parser[1]);
		break;
	case CMD_LED_CLEAR:
		led->clear();
		break;
	case CMD_LED_TOGGLE:
		led->toggle();
		break;
	case CMD_LED_ON:
		led->on();
		break;
	case CMD_LED_OFF:
		led->off();
		break;
	case CMD_LED_BLINK:
		if (cmd_parser.size() >= 5)
		{
			auto period = cmd_parser.read<detail::led_impl_t::time_type>(1); 
			detail::led_impl_t::diff_type diff = 0;
			detail::led_impl_t::count_type count = 0;
			if (cmd_parser.size() >= 9)
				diff = cmd_parser.read<decltype(diff)>(5);
			if (cmd_parser.size() == (9 + sizeof count))
				count = cmd_parser.read<decltype(count)>(9);
			led->blink(period, diff, count);
		}
		else
		{
			invalid_length(cmd_parser, 5);
			return false;
		}
		break;
	case CMD_LED_IS_ON:
		cmd_parser.write(led->get());
		cmd_parser.write(cmd_parser[0]);
		cmd_parser.send(serial, cmd_parser.command());
		return false;
	case CMD_LED_IS_BLINKING:
		cmd_parser.write(led->blinking());
		cmd_parser.write(cmd_parser[0]);
		cmd_parser.send(serial, cmd_parser.command());
		return false;
	default:
		unknown_subcommand(cmd_parser);
		return false;
	}
	return true;
}

union time_t
{
	uint32_t value;
	uint8_t byte[4];
};

void run(void)
{
    cmd_parser_type cmd_parser(base_timer, 16);
	
	timeout emergency_brake_timeout(1000);
	emergency_brake_timeout.force();

	timeout sensors_reader_timeout(400);
	sensors_reader_timeout.cancel();
	bool single = false;
	
	bool white_line = false;

	bool send_ack = true;

	char ch = 0;

	led.on();

	for(;;)
	{
		if(rs232.peek(ch))
		{
			led.toggle();
			send_ack = true;
			switch (cmd_parser.push_data(ch))
			{
			case CMD_NOP:
				emergency_brake_timeout.restart();
				break;

			case CMD_POWER_STEER: 
				if (cmd_parser.size() == 9)
				{
					int8_t fwd = cmd_parser[3];
					int8_t cross = cmd_parser[1];

					if (fwd < -8)
						fwd += 8;
					else if (fwd > 8)
						fwd -= 8;
					else
						fwd = 0;

					if (cross < -8)
						cross += 8;
					else if (cross > 8)
						cross -= 8;
					else
						cross = 0;

					cross /= 2;
						
					int16_t left_target  = avrlib::clamp((fwd + cross) * 2, -255, 255);
					int16_t right_target = avrlib::clamp((fwd - cross) * 2, -255, 255);
					
					setMotorPower(left_target, right_target);
					
					emergency_brake_timeout.restart();
				}
				else
				{
					invalid_length(cmd_parser, 9);
					send_ack = false;
				}
				break;

			case CMD_POWER_TANK:
				if (cmd_parser.size() == 4)
				{
					int16_t left_target  = avrlib::clamp<motor_power_type>(cmd_parser.read<motor_power_type>(0*sizeof(motor_power_type)), -255, 255);
					int16_t right_target = avrlib::clamp<motor_power_type>(cmd_parser.read<motor_power_type>(1*sizeof(motor_power_type)), -255, 255);

					setMotorPower(left_target, right_target);
					
					emergency_brake_timeout.restart();
				}
				else
				{
					invalid_length(cmd_parser, 2);
					send_ack = false;
				}
				break;

			case CMD_DISP_PRINT:
				for (uint8_t i = 0; i != cmd_parser.size(); ++i)
					display.send_data(cmd_parser[i]);
				break;

			case CMD_DISP_SET:
				if (cmd_parser.size() == 0)
				{
					missing_subcommand(cmd_parser);
					send_ack = false;
					break;
				}
				for (uint8_t i = 0; i < cmd_parser.size(); ++i)
				{
					switch(cmd_parser[i])
					{
					case CMD_DISP_SET_CLEAR:
						display.clear();
						break;
					case CMD_DISP_SET_CURSOR_POSITION:
						if (cmd_parser.size() >= (i + 2))
						{
							display.gotoXY(cmd_parser[i+1], cmd_parser[i+2]);
							i += 2;
						}
						else
						{
							invalid_length(cmd_parser, i + 2);
							send_ack = false;
						}
						break;
					case CMD_DISP_SET_CURSOR_HIDE:
						display.hideCursor();
						break;
					case CMD_DISP_SET_CURSOR_SHOW:
						display.showCursor(CURSOR_SOLID);
						break;
					case CMD_DISP_SET_CURSOR_BLINK:
						display.showCursor(CURSOR_BLINKING);
						break;
					case CMD_DISP_SET_CURSOR_MOVE:
						if (cmd_parser.size() > i)
						{
							auto n = cmd_parser.read<int8_t>(++i);
							display.moveCursor(n < 0 ? LCD_LEFT : LCD_RIGHT, avrlib::abs(n));
						}
						else
						{
							invalid_length(cmd_parser, i + 1);
							send_ack = false;
						}
						break;
					case CMD_DISP_SET_SCROLL:
						if (cmd_parser.size() >= (i + 3))
						{
							auto n = cmd_parser.read<int8_t>(++i);
							auto t = cmd_parser.read<uint16_t>(++i);
							i += 1;
							display.scroll(n < 0 ? LCD_LEFT : LCD_RIGHT, avrlib::abs(n), t);
						}
						else
						{
							invalid_length(cmd_parser, i + 4);
							send_ack = false;
						}
						break;
					case CMD_DISP_SET_CUSTOM_CHAR:
						if (cmd_parser.size() >= (i + 9))
						{
							display.loadCustomCharacter(reinterpret_cast<const char*>(cmd_parser.data() + i + 2), cmd_parser[i+1]);
							i += 9;
						}
						else
						{
							invalid_length(cmd_parser, i + 10);
							send_ack = false;
						}
						break;
					case CMD_DISP_SET_CUSTOM_CMD:
						if (cmd_parser.size() >= (i + 1))
						{
							display.send_cmd(cmd_parser[i+1]);
							i += 1;
						}
						else
						{
							invalid_length(cmd_parser, i + 2);
							send_ack = false;
						}
						break;
					}
				}
				break;
			
			case CMD_BUZZER:
				send_ack = led_remote_control(cmd_parser, &advance_buzzer);
				break;

			case CMD_LED:
				send_ack = led_remote_control(cmd_parser, &led);
				break;

			case CMD_CALIBRATION:
				if (cmd_parser.size() == 0)
				{
					missing_subcommand(cmd_parser);
					send_ack = false;
					break;
				}
				switch(cmd_parser[0])
				{
				case CMD_CALIBRATION_RESET:
					resetCalibration();
					break;
				case CMD_CALIBRATION_SENSORS:
					calibrate_sensors();
					break;
				case CMD_CALIBRATION_ROUND:
					cal_round();
					break;
				case CMD_CALIBRATION_STORE:
					if(cmd_parser.size() == 3)
					{
						store_sensor_cal(cmd_parser.read<uint16_t>(1));
					}
					else
					{
						invalid_length(cmd_parser, 3);
						send_ack = false;
					}
					break;
				case CMD_CALIBRATION_LOAD:
					if(cmd_parser.size() == 3)
					{
						load_sensor_cal(cmd_parser.read<uint16_t>(1));
					}
					else
					{
						invalid_length(cmd_parser, 3);
						send_ack = false;
					}
					break;
				}
				break;

			case CMD_EEPROM_LOAD:
				if (cmd_parser.size() == 4)
				{
					auto addr = cmd_parser.read<uint16_t>(0);
					auto len = cmd_parser.read<uint16_t>(2);
					uint8_t v = 0;
					for (uint16_t i = 0; i != len; ++i)
					{
						v = load_eeprom<uint8_t>(addr + i);
						if (!cmd_parser.write(v))
						{
							cmd_parser.send(serial, CMD_EEPROM_LOAD);
							--i;
						}
					}
					cmd_parser.send(serial, CMD_EEPROM_LOAD);
				}
				else
				{
					invalid_length(cmd_parser, 4);
					send_ack = false;
				}
				break;

			case CMD_EEPROM_STORE:
				if (cmd_parser.size() >= 2)
				{
					auto addr = cmd_parser.read<uint16_t>(0);
					for (uint8_t i = 2; i != cmd_parser.size(); ++i, ++addr)
						store_eeprom(addr, cmd_parser[i]);
				}
				else
				{
					invalid_length(cmd_parser, 2);
					send_ack = false;
				}
				break;

			case CMD_EEPROM_SIZE:
				cmd_parser.write(uint16_t(E2END));
				cmd_parser.send(serial, CMD_EEPROM_SIZE);
				break;

			case CMD_PING:
				break;

			case SCMD_STOP:
				setMotorPower(0, 0);
				cmd_parser.clear();
				break;

			case SCMD_STOP_ALL:
				emergency_brake_timeout.force();
				cmd_parser.clear();
				send_ack = false;
				break;

			case SCMD_NOP:
				emergency_brake_timeout.restart();
				cmd_parser.clear();
				break;

			case SCMD_MEAS_STOP:
				sensors_reader_timeout.cancel();
				cmd_parser.clear();
				break;

			case SCMD_MEAS_START:
				single = false;
				sensors_reader_timeout.restart();
				cmd_parser.clear();
				break;

			case SCMD_MEAS_SINGLE:
				single = true;
				sensors_reader_timeout.force();
				cmd_parser.clear();
				break;

			case ' ' ... '~':
				display.send_data(cmd_parser.command());
				cmd_parser.clear();
				break;

			case SCMD_DISP_CLEAR:
				display.clear();
				cmd_parser.clear();
				break;

			case SCMD_DISP_CURSOR_HIDE:
				display.hideCursor();
				cmd_parser.clear();
				break;

			case SCMD_DISP_CURSOR_SHOW:
				display.showCursor(CURSOR_SOLID);
				cmd_parser.clear();
				break;

			case SCMD_DISP_CURSOR_BLINK:
				display.showCursor(CURSOR_BLINKING);
				cmd_parser.clear();
				break;

			case SCMD_DISP_CURSOR_MOVE_LETF:
				display.moveCursor(LCD_LEFT, 1);
				cmd_parser.clear();
				break;

			case SCMD_DISP_CURSOR_MOVE_RIGHT:
				display.moveCursor(LCD_RIGHT, 1);
				cmd_parser.clear();
				break;

			case SCMD_DISP_SCROLL_LEFT:
				display.scroll(LCD_LEFT, 1, 0);
				cmd_parser.clear();
				break;

			case SCMD_DISP_SCROLL_RIGHT:
				display.scroll(LCD_RIGHT, 1, 0);
				cmd_parser.clear();
				break;

			case SCMD_LED_SET:
				led.set();
				cmd_parser.clear();
				break;

			case SCMD_LED_CLEAR:
				led.clear();
				cmd_parser.clear();
				break;

			case SCMD_LED_TOGGLE:
				led.toggle();
				cmd_parser.clear();
				break;

			case SCMD_LED_ON:
				led.on();
				cmd_parser.clear();
				break;

			case SCMD_LED_OFF:
				led.off();
				cmd_parser.clear();
				break;

			case SCMD_LED_IS_ON:
				serial.write(led.get() ? SRESP_LED_ON : SRESP_LED_OFF);
				cmd_parser.clear();
				break;

			case SCMD_LED_IS_BLINKING:
				serial.write(led.get() ? SRESP_LED_BLINKING : SRESP_LED_NOT_BLINKING);
				cmd_parser.clear();
				break;

			case SCMD_BUZZER_SET:
				advance_buzzer.set();
				cmd_parser.clear();
				break;

			case SCMD_BUZZER_CLEAR:
				advance_buzzer.clear();
				cmd_parser.clear();
				break;

			case SCMD_BUZZER_TOGGLE:
				advance_buzzer.toggle();
				cmd_parser.clear();
				break;

			case SCMD_BUZZER_ON:
				advance_buzzer.on();
				cmd_parser.clear();
				break;

			case SCMD_BUZZER_OFF:
				advance_buzzer.off();
				cmd_parser.clear();
				break;

			case SCMD_BUZZER_IS_ON:
				serial.write(advance_buzzer.get() ? SRESP_BUZZER_ON : SRESP_BUZZER_OFF);
				cmd_parser.clear();
				break;

			case SCMD_BUZZER_IS_BLINKING:
				serial.write(advance_buzzer.get() ? SRESP_BUZZER_BLINKING : SRESP_BUZZER_NOT_BLINKING);
				cmd_parser.clear();
				break;

			case 255: // valid byte received
				break;

			case 254: // error
				cmd_parser.clear();
				send_ack = false;
				break;

			default:
				if (cmd_parser.state() == cmd_parser.simple_command)
				{
					serial.write(SRESP_UNKNOWN_CMD);
					cmd_parser.clear();
				}
				else
				{
					cmd_parser.write(RESP_ERROR_UNKNOWN_CMD);
					cmd_parser.write(cmd_parser.command());
					cmd_parser.send(serial, RESP_ERROR);
				}
				send_ack = false;
				break;
			}
			if(send_ack)
				ack(cmd_parser);
		}

		if (emergency_brake_timeout)
		{
			setMotorPower(0, 0);
			advance_buzzer.off();
			led.off();
			emergency_brake_timeout.cancel();
			serial.write(SCMD_STOP_ALL);
		}

		led.process();
		advance_buzzer.process();

		if (sensors_reader_timeout)
		{
			if (single)
			{
				sensors_reader_timeout.cancel();
				single = false;
			}
			else
				sensors_reader_timeout.ack();
			time_t t;
			t.value = base_timer.value();
			uint16_t v = getSensorValue(0);
			uint16_t s = getSensorValue(1);
			v |= (s & 0x03F0) << 6;
			cmd_parser.write(v);
			v = getSensorValue(2) << 4;
			v |= s & 0x000F;
			s = getSensorValue(3);
			v |= (s & 0x0300) << 6;
			cmd_parser.write(v);
			v = s & 0x00FF;
			s = getSensorValue(4);
			v |= (s & 0x00FF) << 8;
			cmd_parser.write(v);
			v = getBatteryVoltage();
			v |= (s & 0x0300) << 2;
			s = getSensorValue(6);
			v |= (s & 0x03C0) << 6;
			cmd_parser.write(v);
			v = getLinePos(white_line);
			v |= isPressed(BUTTON_A) << 13;
			v |= isPressed(BUTTON_B) << 14;
			v |= isPressed(BUTTON_C) << 15;
			cmd_parser.write(v);
			t.byte[3] &= 0x03;
			t.byte[3] |= (s & 0x3F) << 2;
			cmd_parser.write(t);
			cmd_parser.send(serial, RESP_SENSORS);
		}
	}
}

/* sent data bits position
s.x : sensor
b.x : battery voltage in mV
p.x : trimmer value
l.x : line position
t.x : timestamp
bX  : button value

byte  bit  15                                                       8       7                                                       0      byte
    1    s1.9    s1.8    s1.7    s1.6    s1.5    s1.4    s0.9    s0.8    s0.7    s0.6    s0.5    s0.4    s0.3    s0.2    s0.1    s0.0     0
    3    s3.9    s3.8    s2.9    s2.8    s2.7    s2.6    s2.5    s2.4    s2.3    s2.2    s2.1    s2.0    s1.3    s1.2    s1.1    s1.0     2
    5    s4.7    s4.6    s4.5    s4.4    s4.3    s4.3    s4.1    s4.0    s3.7    s3.6    s3.5    s3.4    s3.3    s3.2    s3.1    s3.0     4
    7    p.9     p.8     p.7     p.6     s4.9    s4.8    b.9     b.8     b.7     b.6     b.5     b.4     b.3     b.2     b.1     b.0      6
    9    bC      bB      bA      l.12    l.11    l.10    l.9     l.8     l.7     l.6     l.5     l.4     l.3     l.2     l.1     l.0      8
    B    t.15    t.14    t.13    t.12    t.11    t.10    t.9     t.8     t.7     t.6     t.5     t.4     t.3     t.2     t.1     t.0      A
    D    p.5     p.4     p.3     p.2     p.1     p.0     t.25    t.24    t.23    t.22    t.21    t.20    t.19    t.18    t.17    t.16     C

*/
