#ifndef _3PI_HPP_
#define _3PI_HPP_

#include "encoder.hpp"
#include "regulator.hpp"

namespace detail
{
namespace R3PI
{
#include "3pi_commands.hpp"
#include "3pi_defines.hpp"
} // namespace R3PI

template<typename Stream, typename Parser>
void print_packet_data(Stream& stream, Parser& parser, uint8_t offset = 0, bool new_line = true)
{
	for (; offset < parser.size(); ++offset)
		format_spgm(stream, PSTR(" 0x%2x")) % parser[offset];
	if (new_line)
		stream.write('\n');
}

} // namespace detail

class R3pi
{
public:
	typedef avrlib::timed_command_parser<base_timer_type> cmd_parser_type;
	typedef data_uart_t link_type;
	typedef detail::R3PI::motor_power_type motor_power_type;
	typedef detail::R3PI::motor_speed_type motor_speed_type;

private:
	class led_t
	{
		friend class R3pi;

		typedef detail::R3PI::cmd_type cmd_type;
	public:
		led_t(link_type& link, cmd_type cmd, cmd_type scmd)
			:m_link(link), m_cmd(cmd), m_scmd(scmd & 0xF0), m_updated(false)
		{}

		void set(bool v = true)
		{
			if (v)
				m_link.write(m_scmd | (detail::R3PI::SCMD_LED_SET));
			else
				m_link.write(m_scmd | (detail::R3PI::SCMD_LED_CLEAR));
		}
		void clear() { m_link.write(m_scmd | (detail::R3PI::SCMD_LED_CLEAR)); }
		void toggle() { m_link.write(m_scmd | (detail::R3PI::SCMD_LED_TOGGLE)); }
		void on() { format(debug, "on: 0x%x2\n") % (m_scmd | (detail::R3PI::SCMD_LED_ON)); m_link.write(m_scmd | (detail::R3PI::SCMD_LED_ON)); }
		void off() { m_link.write(m_scmd | (detail::R3PI::SCMD_LED_OFF)); }

		bool updated() const { return m_updated; }

		bool get()
		{
			m_link.write(m_scmd | (detail::R3PI::SCMD_LED_IS_ON));
			wait_for_update();
			return m_on;
		}

		bool blinking()
		{
			m_link.write(m_scmd | (detail::R3PI::SCMD_LED_IS_BLINKING));
			wait_for_update();
			return m_blinking;
		}

	private:
		void wait_for_update()
		{
			m_updated = false;
			wait(msec(10), [&]()->bool{ ::process(); return m_updated; }, 0);
		}

		void set_on(bool v)
		{
			m_on = v;
			m_updated = true;
		}

		void set_blinking(bool v)
		{
			m_blinking = v;
			m_updated = true;
		}

		link_type& m_link;
		const cmd_type m_cmd;
		const cmd_type m_scmd;
		bool m_updated;
		bool m_on;
		bool m_blinking;
	};

public:
	

	R3pi(pin_t pin_power,
		 pin_t pin_reset,
		 pin_t pin_disp_backlight,
		 adc_t::adc_pin_type adc_vboost,
		 link_type& link,
		 encoder_t& left_enc,
		 encoder_t& right_enc)
		: m_pin_power(pin_power),
		  m_pin_reset(pin_reset),
		  m_pin_disp_backlight(pin_disp_backlight),
		  m_vboost(adc_vboost),
		  m_link(link),
		  m_enc_left(left_enc),
		  m_enc_right(right_enc),
		  m_reg_left(detail::R3PI::MOTOR_POWER_MAX, 1, 0, 0),
		  m_reg_right(detail::R3PI::MOTOR_POWER_MAX, 1, 0, 0),
		  m_reg_timout(msec(10)),
		  m_cmd_parser(base_timer, msec(16)),
		  m_keep_connection_timeout(msec(250)),
		  m_bridge(nullptr),
		  m_led(m_link, detail::R3PI::CMD_LED, detail::R3PI::SCMD_LED_SET),
		  m_buzzer(m_link, detail::R3PI::CMD_BUZZER, detail::R3PI::SCMD_BUZZER_SET)
		{
			m_reg_timout.cancel();
		}

	void init()
	{
		on();
		wait(msec(100), ::process);
		m_link.write(detail::R3PI::SCMD_MEAS_STOP);
	}

	void on() { m_pin_power.set_high(); }

	void off() { m_pin_power.set_low(); }

	bool is_on() { return m_pin_power.read(); }

	void reset()
	{
		m_pin_reset.set_high();
		wait(msec(10), ::process);
		m_pin_reset.set_low();
	}

	void set_motors_power(motor_power_type left, motor_power_type right)
	{
		if (m_bridge != nullptr)
			return;
		if (m_reg_timout.running()) {
			m_reg_timout.cancel();
		}
		m_cmd_parser.write(left);
		m_cmd_parser.write(right);
		m_cmd_parser.send(m_link, detail::R3PI::CMD_POWER_TANK);
	}

	void set_motor_speed(motor_speed_type left, motor_speed_type right)
	{
		if(!m_reg_timout.running()) {
			m_reg_timout.start();
			m_reg_left.clear();
			m_reg_right.clear();
		}
		m_reg_left(left);
		m_reg_right(right);
	}

	void stop()
	{
		if (m_bridge != nullptr)
			return;
		m_link.write(detail::R3PI::SCMD_STOP);
	}

	led_t& led() { return m_led; }	
	led_t& buzzer() { return m_buzzer; }

	link_type* set_bridge(link_type* bridge)
	{
		m_bridge = bridge;
		if(m_bridge == nullptr)
			m_keep_connection_timeout.restart();
		else
			m_keep_connection_timeout.cancel();
		return &m_link;
	}

	void process()
	{
		using namespace detail::R3PI;
		if(!m_link.empty())
		{
			if (m_bridge != nullptr)
			{
				m_bridge->write(m_link.read());
			}
			else
			{
				switch(m_cmd_parser.push_data(m_link.read()))
				{
				case RESP_ACK:
					break;

				case RESP_SENSORS:
					if (m_cmd_parser.size() != 14)
					{
						format_spgm(debug, PSTR("R3pi Error: received too short sensor packet: only %2 bytes.\n")) % m_cmd_parser.size();
					}
					else
					{
						m_sensors[0] =   m_cmd_parser[ 0]               | ((m_cmd_parser[ 1] & 0x03) << 8);
						m_sensors[1] =  (m_cmd_parser[ 2] & 0x0F)       | ((m_cmd_parser[ 1] & 0xFC) << 2);
						m_sensors[2] = ((m_cmd_parser[ 2] & 0xF0) >> 4) | ((m_cmd_parser[ 3] & 0x3F) >> 4);
						m_sensors[3] =   m_cmd_parser[ 4]               | ((m_cmd_parser[ 3] & 0xC0) >> 6);
						m_sensors[4] =   m_cmd_parser[ 5]               | ((m_cmd_parser[ 7] & 0x3F) >> 4);
						m_vbat       =   m_cmd_parser[ 6]               | ((m_cmd_parser[ 7] & 0x03) << 8);
						m_pot        = ((m_cmd_parser[13] & 0xFC) >> 2) | ((m_cmd_parser[ 7] & 0xF0) >> 6);
						m_line_pos   =   m_cmd_parser[ 8]               | ((m_cmd_parser[ 9] & 0x1F) << 8);
						m_btn_a      =  (m_cmd_parser[ 9] & 0x20);
						m_btn_b      =  (m_cmd_parser[ 9] & 0x40);
						m_btn_c      =  (m_cmd_parser[ 9] & 0x80);
						m_time       =   m_cmd_parser.read<decltype(m_time)>(10) & 0x03FFFFFF;
					}
					break;

				case RESP_ERROR:
					if (m_cmd_parser.size() == 0)
					{
						format_spgm(debug, PSTR("R3pi Error: received empty ERROR packet (without error code).\n"));
					}
					else
					{
						switch(m_cmd_parser[0])
						{
						case RESP_ERROR_UNKNOWN_CMD:
							if (m_cmd_parser.size() < 2)
							{
								format_spgm(debug, PSTR("R3pi Error: received UNKNOWN_CMD ERROR without cmd code.\n"));
							}
							else if (m_cmd_parser.size() > 2)
							{
								format_spgm(debug, PSTR("R3pi Error: received UNKNOWN_CMD ERROR packet with more data than expected: cmd code = 0x%x2 , additional data:")) % m_cmd_parser[1];
								detail::print_packet_data(debug, m_cmd_parser, 2);
							}
							else
							{
								format_spgm(debug, PSTR("R3pi Error: received UNKNOWN_CMD ERROR: cmd code = 0x%x2.\n")) % m_cmd_parser[1];
							}
							break;
						case RESP_ERROR_UNKNOWN_SUBCMD:
							if (m_cmd_parser.size() < 3)
							{
								format_spgm(debug, PSTR("R3pi Error: received UNKNOWN_SUBCMD ERROR without subcmd code, cmd code = 0x%2x.\n")) % m_cmd_parser[1];
							}
							else if (m_cmd_parser.size() < 2)
							{
								format_spgm(debug, PSTR("R3pi Error: received UNKNOWN_SUBCMD ERROR without cmd code.\n"));
							}
							else if (m_cmd_parser.size() > 3)
							{
								format_spgm(debug, PSTR("R3pi Error: received UNKNOWN_SUBCMD ERROR packet with more data than expected: cmd code = 0x%x2, subcmd code = 0x%x2, additional data:")) % m_cmd_parser[1] % m_cmd_parser[2];
								detail::print_packet_data(debug, m_cmd_parser, 3);
							}
							else
							{
								format_spgm(debug, PSTR("R3pi Error: received UNKNOWN_SUBCMD ERROR: cmd code = 0x%x2, subcmd code = 0x%x2.\n")) % m_cmd_parser[1] % m_cmd_parser[2];
							}
							break;
						case RESP_ERROR_MISSING_SUBCMD:
							if (m_cmd_parser.size() < 2)
							{
								format_spgm(debug, PSTR("R3pi Error: received MISSING_SUBCMD ERROR without cmd code.\n"));
							}
							else if (m_cmd_parser.size() > 2)
							{
								format_spgm(debug, PSTR("R3pi Error: received MISSING_SUBCMD ERROR packet with more data than expected: cmd code = 0x%x2, additional data:")) % m_cmd_parser[1];
								detail::print_packet_data(debug, m_cmd_parser, 2);
							}
							else
							{
								format_spgm(debug, PSTR("R3pi Error: received MISSING_SUBCMD ERROR: cmd code = 0x%x2.\n")) % m_cmd_parser[1];
							}
							break;
						case RESP_ERROR_INVALID_LENGTH:
							if (m_cmd_parser.size() < 4)
							{
								format_spgm(debug, PSTR("R3pi Error: received INVALID_LENGTH ERROR without expected length, cmd code = 0x%2x, received length = %2.\n")) % m_cmd_parser[1] % m_cmd_parser[2];
							}
							else if (m_cmd_parser.size() < 3)
							{
								format_spgm(debug, PSTR("R3pi Error: received INVALID_LENGTH ERROR without received length, cmd code = 0x%2x.\n")) % m_cmd_parser[1];
							}
							else if (m_cmd_parser.size() < 2)
							{
								format_spgm(debug, PSTR("R3pi Error: received INVALID_LENGTH ERROR without cmd code.\n"));
							}
							else if (m_cmd_parser.size() > 4)
							{
								format_spgm(debug, PSTR("R3pi Error: received INVALID_LENGTH ERROR packet with more data than expected: cmd code = 0x%x2, received length = %2, expected length = %2, additional data:")) % m_cmd_parser[1] % m_cmd_parser[2] % m_cmd_parser[3];
								detail::print_packet_data(debug, m_cmd_parser, 3);
							}
							else
							{
								format_spgm(debug, PSTR("R3pi Error: received INVALID_LENGTH ERROR: cmd code = 0x%x2, received length = %2, expected length = %2.\n")) % m_cmd_parser[1] % m_cmd_parser[2] % m_cmd_parser[3];
							}
							break;
						default:
							format_spgm(debug, PSTR("R3pi Error: received unknown ERROR code: 0x%2x")) % m_cmd_parser[0];
							if(m_cmd_parser.size() > 1)
							{
								format_spgm(debug, PSTR(", additional data:"));
								detail::print_packet_data(debug, m_cmd_parser, 1);
							}
							else
							{
								format_spgm(debug, PSTR(".\n"));
							}
							break;
						}
					}
					break;

				case SCMD_STOP_ALL:
					send(debug, "STOP_ALL\n");
					break;

				case SRESP_ACK: // ACK
					break;

				case SRESP_LED_ON:
					m_led.set_on(true);
					break;

				case SRESP_LED_OFF:
					m_led.set_on(false);
					break;

				case SRESP_LED_BLINKING:
					m_led.set_blinking(true);
					break;

				case SRESP_LED_NOT_BLINKING:
					m_led.set_blinking(false);
					break;

				case SRESP_BUZZER_ON:
					m_buzzer.set_on(true);
					break;

				case SRESP_BUZZER_OFF:
					m_buzzer.set_on(false);
					break;

				case SRESP_BUZZER_BLINKING:
					m_buzzer.set_blinking(true);
					break;

				case SRESP_BUZZER_NOT_BLINKING:
					m_buzzer.set_blinking(false);
					break;

				case SRESP_UNKNOWN_CMD:
					format_spgm(debug, PSTR("R3pi Error: received responce to unknown simple packet.\n"));
					break;

				case 255: // valid byte received
					break;

				case 254: // error
					m_cmd_parser.clear();
					format_spgm(debug, PSTR("R3pi Error: packet error (error count: % ).\n")) % m_cmd_parser.error_cnt();
					break;

				default:
					if (m_cmd_parser.state() == m_cmd_parser.simple_command)
					{
						format_spgm(debug, PSTR("R3pi Error: received unknown simple packet: 0x%x2.\n")) % m_cmd_parser.command();
						m_cmd_parser.clear();
					}
					else
					{
						format_spgm(debug, PSTR("R3pi Error: received packet with unknown cmd code: 0x%x2")) % m_cmd_parser.command();
						if(m_cmd_parser.size() > 0)
						{
							format_spgm(debug, PSTR(", with data:"));
							detail::print_packet_data(debug, m_cmd_parser);
						}
						else
						{
							format_spgm(debug, PSTR(".\n"));
						}
					}
					break;
				}
			}
		}
		if (m_keep_connection_timeout)
		{
			m_keep_connection_timeout.ack();
			m_link.write(SCMD_NOP);
		}
		if (m_reg_timout) {
			m_reg_timout.ack();
			set_motor_speed(m_reg_left.process(m_enc_left.value()), 
							m_reg_right.process(m_enc_right.value()));
		}
	}

	static const uint8_t c_sensors = 5;

private:

	pin_t m_pin_power;
	pin_t m_pin_reset;
	pin_t m_pin_disp_backlight;
	adc_t m_vboost;
	link_type& m_link;
	
	encoder_t& m_enc_left;
	encoder_t& m_enc_right;

	regulator_t<int32_t> m_reg_left;
	regulator_t<int32_t> m_reg_right;
	timeout m_reg_timout;

	cmd_parser_type m_cmd_parser;
	timeout m_keep_connection_timeout;
	link_type* m_bridge;

	led_t m_led;
	led_t m_buzzer;
		
	uint16_t m_sensors[c_sensors];
	uint16_t m_vbat;
	uint16_t m_pot;
	uint16_t m_line_pos;
	uint32_t m_time;
	bool m_btn_a;
	bool m_btn_b;
	bool m_btn_c;
	uint16_t m_eeprom_size;
};

#endif
