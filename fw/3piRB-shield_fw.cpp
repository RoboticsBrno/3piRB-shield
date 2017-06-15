/*
 * fw.cpp
 * 3piRB-shield fw
 * Created: 30.03.2017 0:58:50
 * Author : JarekParal
 */ 

#include "settings.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <util/delay.h>
#include <stddef.h>

#include "avrlib/assert.hpp"
#include "avrlib/async_usart.hpp"
#include "avrlib/uart_xmega.hpp"
#include "avrlib/command_parser.hpp"
#include "avrlib/string.hpp"
#include "avrlib/format.hpp"
#include "avrlib/math.hpp"
#include "avrlib/algorithm.hpp"

using avrlib::format;
using avrlib::format_spgm;
using avrlib::send;
using avrlib::send_spgm;
using avrlib::string;

#include "build_info.hpp"

typedef avrlib::async_usart<avrlib::uart_xmega, DEBUG_UART_RX_BUFF_SIZE, DEBUG_UART_TX_BUFF_SIZE> debug_t; // D0
debug_t debug;
ISR(USARTD0_RXC_vect) { debug.intr_rx(); }

typedef avrlib::async_usart<avrlib::uart_xmega, DATA_UART_RX_BUFF_SIZE, DATA_UART_TX_BUFF_SIZE> data_uart_t;

data_uart_t bt_uart; // F0
ISR(USARTF0_RXC_vect) { bt_uart.intr_rx(); }

data_uart_t r3pi_uart; // E0
ISR(USARTE0_RXC_vect) { r3pi_uart.intr_rx(); }

data_uart_t sink; // D1
ISR(USARTD1_DRE_vect)
{
	sink.tx_buffer().clear();
	sink.intr_tx();
}

// struct Sink {
// 	void write(char){}
// 	void flush(){}
// }; Sink sink;


void process();

#include "adc.hpp"
#include "mcu_gpio_defs.hpp"
#include "miscellaneous.hpp"
#include "serial_number.hpp"
#include "reset.hpp"
#include "eeprom.hpp"
#include "clock.hpp"
#include "time.hpp"
#include "baudrate.hpp"
#include "new.hpp"
#include "lambda_storage.hpp"
#include "led.hpp"
#include "spi.hpp"
#include "encoder.hpp"
#include "encoder_spi.hpp"
#include "regulator.hpp"
#include "button.hpp"
#include "ir_sensor.hpp"
#include "3pi.hpp"

const int16_t Vbat_destruction_low_voltage = 2140; // 2140 = cca. 3,2 V
const int16_t Vbat_critical_low_voltage = 2300; // 2300 = cca. 3,3 V
const int16_t Vbat_standard_voltage = 2530; // 2530 = 3,7 v
const int16_t Vbat_max_voltage = 2840; // 2840 = 4,2 v

const eeprom::addr_type IR_calibration_addr = 0;

template <typename Serial>
void print_device_info(Serial& serial, const bool& with_reset = true)
{
	format_spgm(serial, PSTR("\n3piRB-shield\n\tSN: % \n")) % const_cast<const char*>(serial_number_str);
	BUILD_INFO::print(serial, 1, '\t');
	if(with_reset)
	reset.print_source(serial, PSTR("\tLast reset: "), PSTR("\n"));
	serial.flush();
}

avrlib::timed_command_parser<base_timer_type> packet(base_timer, msec(100));

/*
timeout emergency_shutdown_timeout(msec(500));
volatile bool emergency_braked = false;

button_intr_t stop_btn(pin_t(pin_stop_btn::PORT(), pin_stop_btn::bp), 0,
	[](button_t::callback_arg_type v)
	{
		if(v && emergency_braked)
		emergency_shutdown_timeout.restart();
	},
	[](button_t::callback_arg_type v)
	{
		if(v)
		{
			stop_btn.disable_interrupt();
		}
	}
);

ISR(PORTD_INT0_vect)
{
	stop_btn.process_intr();
}
*/

encoder_t encoder_left(TCE1, pin_enc_left_a , 0);
encoder_t encoder_right(TCD1, pin_enc_right_a, 2);

ISR(TCE1_OVF_vect) { encoder_left.process_intr(); }
ISR(TCD1_OVF_vect) { encoder_right.process_intr(); }

SPI::Interface spie(SPIE);
ISR(SPIE_INT_vect) { spie.process_intr(); }

encoder_spi_t encoder_ctrl_left(spie, pin_enc_left_cs);
encoder_spi_t encoder_ctrl_right(spie, pin_enc_right_cs);

R3pi r3pi(pin_3pi_power,
		  pin_3pi_reset,
		  pin_disp_backlight_en,
		  13,
		  r3pi_uart,
		  encoder_left,
		  encoder_right);

void process()
{
	static uint8_t led_index = 0;
	
	button_t::process_all();
	
	/*
	if(emergency_shutdown_timeout)
	{
		if(stepper_motor_mode)
		{
			stepper_motor.enable(false);
			for(auto p: pwms)
			p->off();
		}
		else
		{
			motor_1.off();
			motor_2.off();
		}
		emergency_shutdown_timeout.cancel();
		if(emergency_braked)
		send(debug, "emergency shutdown finished\n");
	}
	
	if(emergency_braked && !led_err.red.blinking())
	led_err.red.blink(msec(200));
	*/

	if(leds[led_index]->process())
	{
		if(++led_index == led_count)
		{
			led_index = 0;
		}
	}

	adc_t::process_all();
	
	//encoder_ctrl_left.process();
	//encoder_ctrl_right.process();
	
	r3pi.process();

	bt_uart.process_tx();
	r3pi_uart.process_tx();
	debug.process_tx();
}

template<typename Serial, typename T>
void dump_modul_settings(Serial& serial, const T& module, const char* msg = nullptr)
{
	if(msg != nullptr)
		send(serial, msg);
	const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&module);
	for(uint16_t i = 0; i != sizeof(T); ++i)
	{
		if(*ptr)
			format(serial, "%x2: %x2\n") % i % *ptr;
		++ptr;
	}
}

void shutdown()
{
	send(debug, "\tShutdown!\n");
	debug.flush();
	led_2.green.off();
	led_2.red.on();
	process();

	pin_SHDN.set_high();
	for(;;);
}

template <typename Stream>
void send_avakar_packet(Stream & s, uint8_t id, int16_t value) {
	packet.write(value);
	packet.send(s, id);
	s.flush();
}

int main(void)
{
	init_mcu_gpio();
	init_clock();
	
	init_serial_number();
	
	debug.usart().open(USARTD0, calc_baudrate(DEBUG_UART_BAUDRATE, F_CPU), avrlib::uart_intr_med);
	bt_uart.usart().open(USARTF0, calc_baudrate(BT_UART_BAUDRATE, F_CPU), avrlib::uart_intr_med);
	r3pi_uart.usart().open(USARTE0, calc_baudrate(R3PI_UART_BAUDRATE, F_CPU), avrlib::uart_intr_med);
	_delay_ms(1);

	sink.usart().open(USARTD1, 0, false);
	sink.usart().close();
	sink.async_tx(true);

	print_device_info(debug);
	
	eeprom::init();
	
	init_time();
	
	// enable interrupts
	PMIC.CTRL = PMIC_RREN_bm | PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
	sei();
	
	run_leds();

	spie.open();

	encoder_left.mode_quadrature();
	encoder_right.mode_quadrature();
	encoder_right.make_inverted();

	debug.flush();

	_delay_ms(1000);
	
	for(auto l: leds)
	{
		l->red.off();
		l->green.off();
	}

	//leds[get_led_near_sensor(9)]->green.blink(msec(500));
	//leds[sensor_to_led(9)]->green.on();

	pin_AREF_EN.set_high();
	ir_sensor_t::on();

	// adc init
	adc_t::init();

	adc_t adc_I(11);
	adc_t adc_Iref(12);
	adc_t adc_Vbat(13);
	adc_t adc_3pi_Vbst(14);
	adc_t adc_Bat_charg_stat(15);
	
	ir_sensor_t IR1(1);
	ir_sensor_t IR2(2);
	ir_sensor_t IR3(3);
	ir_sensor_t IR4(4);
	ir_sensor_t IR5(9);
	ir_sensor_t IR6(8);
	ir_sensor_t IR7(7);
	ir_sensor_t IR8(5);
	ir_sensor_t IR9(6);

	const uint8_t ADC_IR_COUNT = 9;
	ir_sensor_t * IR[ADC_IR_COUNT] = {
		&IR1,
		&IR2,
		&IR3,
		&IR4,
		&IR5,
		&IR6,
		&IR7,
		&IR8,
		&IR9
	};
	
	eeprom::addr_type ir_calibration_addr = IR_calibration_addr;
	for (auto s: IR)
	{
		s->load_calibration(ir_calibration_addr);
		ir_calibration_addr += s->calibration_size();
	}

	// interrupt stop btn
	//stop_btn.pin().port.INTFLAGS = PORT_INT0IF_bm | PORT_INT1IF_bm;
	//stop_btn.pin().port.INTCTRL = PORT_INT0LVL_HI_gc;
	
	r3pi.init();

	char ch = 0;
	uint16_t time_cnt = 0;

	timeout debug_sender(msec(200));

	timeout blink_timeout(msec(1000));
	const auto blink_period = (blink_timeout.get_timeout() * 14) >> 4;
	//debug_sender.cancel();

	while(pin_btn_pwr.read()) {
		process();
	}

	while(!adc_Vbat.new_value()) {
		process();
	}

	send(debug, "main loop\n");

	bool Vbat_critical_value_activate = false;

	data_uart_t* bt_bridge = nullptr;

    // Activate BT BRIDGE (BT -> 3pi)
    bt_bridge = r3pi.set_bridge(&bt_uart);
    
	timeout square_timeout(msec(1000));
	uint8_t square_state = 0;

	for(;;)
	{
		if(square_timeout)
		{
			square_timeout.ack();
			if (square_state == 0)
			{
				r3pi.set_motors_power(-20, 20);
				square_timeout.set_timeout(msec(350));
				square_state = 1;
			}
			else
			{
				r3pi.set_motors_power(20, 20);
				square_timeout.set_timeout(msec(2000));
				square_state = 0;
			}
		}
		if(IR2.value() > 250 && square_timeout.running())
		{
			r3pi.stop();
			square_timeout.cancel();
		}
		if(IR2.value() < 230 && !square_timeout.running())
		{
			square_timeout.restart();
		}
		
		if(!debug.empty())
		{
			ch = debug.read();
			switch(ch)
			{
			case '\r':
				debug.write('\n');
				break;
			case 's':
			case 'S':
				shutdown();
				break;
			case 'b':
				bt_bridge = r3pi.set_bridge(&bt_uart);
				break;
			case 'n':
				r3pi.set_bridge(nullptr);
				bt_bridge = nullptr;
				break;
			case 'p':
				r3pi.buzzer().on();
				break;
			case 'l':
				r3pi_uart.write(0xB3);
				break;
			default:
				debug.write(ch);
			}
		}

		if(!bt_uart.empty())
		{
			ch = bt_uart.read();
			if (bt_bridge != nullptr)
			{
				bt_bridge->write(ch);
			}
			else
			{
				switch(ch)
				{
				case '\r':
					bt_uart.write('\n');
					break;
				case 's':
				case 'S':
					shutdown();
					break;
				case 'c':
					for(auto l: leds)
					{
						l->red.off();
						l->green.off();
					}
					for (auto s: IR)
						s->calibrate(process);
					break;
				case 'C':
					ir_calibration_addr = IR_calibration_addr;
					for (auto s: IR)
					{
						s->save_calibration(ir_calibration_addr);
						ir_calibration_addr += s->calibration_size();
					}
					eeprom::flush();
					break;
				case 'p':
					for (auto s: IR)
						s->print_calibration(bt_uart);
					break;
				default:
					bt_uart.write(ch);
				}
			}
		}

		if (encoder_ctrl_right.is_updated())
		{
			format_spgm(debug, PSTR("l: a %4 m %4 g %3 l %1 h %1 o %1 e %1 p %1 c %1 f %1 i %9\tr: a %4 m %4 g %3 l %1 h %1 o %1 e %1 p %1 c %1 f %1 i %9\n"))
				% encoder_ctrl_left.angle()
				% encoder_ctrl_left.magnitude()
				% encoder_ctrl_left.agc()
				% encoder_ctrl_left.alarm_lo()
				% encoder_ctrl_left.alarm_hi()
				% encoder_ctrl_left.cordic_overflow()
				% encoder_ctrl_left.error()
				% encoder_ctrl_left.parity_error()
				% encoder_ctrl_left.command_error()
				% encoder_ctrl_left.framing_error()
				% encoder_left.value()
				% encoder_ctrl_right.angle()
				% encoder_ctrl_right.magnitude()
				% encoder_ctrl_right.agc()
				% encoder_ctrl_right.alarm_lo()
				% encoder_ctrl_right.alarm_hi()
				% encoder_ctrl_right.cordic_overflow()
				% encoder_ctrl_right.error()
				% encoder_ctrl_right.parity_error()
				% encoder_ctrl_right.command_error()
				% encoder_ctrl_right.framing_error()
				% encoder_right.value();
		}

		if (blink_timeout)
		{
			blink_timeout.ack();
			for(int i = 0; i < ADC_IR_COUNT; i++)
				leds[sensor_to_led(i)]->green.blink(blink_period, msec(-to_msec(blink_period) + (IR[i]->value() << 1)), 1);
		}

		if(debug_sender) 
 		{
			auto& text_out = debug;
			auto& bin_out = bt_bridge == nullptr ? bt_uart : sink;
			// IR sensors debug
			format(text_out, "%5 - ") % time_cnt;
			time_cnt = 0;
			for(int i = 0; i < ADC_IR_COUNT; i++)
			{	
				send_avakar_packet(bin_out, i+1, IR[i]->value());
				format(text_out, "% : %4 \t") % (i+1) % IR[i]->value();
				
			}
			format(text_out, "I: %4 \t Iref: %4 \t Vbat: %4 ")
				% adc_I.value()
				% adc_Iref.value()
				% adc_Vbat.value();

			format(text_out, "\t encL: %9; encR: %9")
				% encoder_left.value()
				% encoder_right.value();

 			send(text_out, "\n");

			send_avakar_packet(bin_out, 10, adc_Vbat.value());
			
			packet.write(encoder_left.value());
			packet.write(encoder_right.value());
			packet.send(bin_out, 11);

			debug_sender.ack();
		}

		// check battery voltage
		if(adc_Vbat.value() < Vbat_critical_low_voltage) {
			if(Vbat_critical_value_activate == false) {
				for (auto l: leds) {
					l->green.on();
					l->red.on();
				}
				Vbat_critical_value_activate = true;
			}
			if (adc_Vbat.value() < Vbat_destruction_low_voltage)
			{
				shutdown();
			}
		} else {
			if (Vbat_critical_value_activate == true) {
				for (auto l: leds) {
					l->green.off();
					l->red.off();
				}
				Vbat_critical_value_activate = false;
			}
		}

		if (pin_btn_pwr.read())
		{
			wait(msec(50));
			if (pin_btn_pwr.read())
			{
				shutdown();
			}
		}

		time_cnt++;
		process();
	}
}

