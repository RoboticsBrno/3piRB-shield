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

typedef avrlib::async_usart<avrlib::uart_xmega, BT_UART_RX_BUFF_SIZE, BT_UART_TX_BUFF_SIZE> bt_uart_t; // F0
bt_uart_t bt_uart;
ISR(USARTF0_RXC_vect) { bt_uart.intr_rx(); }

const int16_t Vbat_destruction_low_voltage = 2140; // 2140 = cca. 3,2 V
const int16_t Vbat_critical_low_voltage = 2300; // 2300 = cca. 3,3 V
const int16_t Vbat_standard_voltage = 2530; // 2530 = 3,7 v
const int16_t Vbat_max_voltage = 2840; // 2840 = 4,2 v
void process();

#include "adc.hpp"
#include "mcu_gpio_defs.hpp"
#include "miscellaneous.hpp"
#include "serial_number.hpp"
#include "reset.hpp"
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
	
	bt_uart.process_tx();
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
	_delay_ms(1);

	print_device_info(debug);
	
	// mapping eeprom
	NVM.CTRLB = NVM_EEMAPEN_bm;
	while((NVM.STATUS & NVM_NVMBUSY_bm) != 0){};
	
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
	leds[sensor_to_led(9)]->green.on();

	// adc init
	adc_t::init();
	
	adc_t adc_IR1(1);
	adc_t adc_IR2(2);
	adc_t adc_IR3(3);
	adc_t adc_IR4(4);
	adc_t adc_IR5(5);
	adc_t adc_IR6(6);
	adc_t adc_IR7(7);
	adc_t adc_IR8(8);
	adc_t adc_IR9(9);

	const uint8_t ADC_IR_COUNT = 9;
	adc_t * adc_IR[ADC_IR_COUNT] = {
		&adc_IR1,
		&adc_IR2,
		&adc_IR3,
		&adc_IR4,
		&adc_IR5,
		&adc_IR6,
		&adc_IR7,
		&adc_IR8,
		&adc_IR9
	};

	adc_t::adc_value_type adc_last_values[ADC_IR_COUNT] = {0};
	adc_t::adc_value_type adc_IR_threshold = 30;

	adc_t adc_I(11);
	adc_t adc_Iref(12);
	adc_t adc_Vbat(13);
	adc_t adc_3pi_Vbst(14);
	adc_t adc_Bat_charg_stat(15);
	
	format(debug, "adc_Vbat.pin(): %  - max_index: % \n") % adc_Vbat.pin() % adc_Vbat.max_index();
	
	pin_AREF_EN.set_high();
	pin_IR_front.set_high();
	pin_IR_back_left_right.set_high();
	
	// interrupt stop btn
	//stop_btn.pin().port.INTFLAGS = PORT_INT0IF_bm | PORT_INT1IF_bm;
	//stop_btn.pin().port.INTCTRL = PORT_INT0LVL_HI_gc;
	
	char ch = 0;
	uint16_t time_cnt = 0;

	timeout debug_sender(msec(200));
	//debug_sender.cancel();

	while(pin_btn_pwr.read()) {
		process();
	}

	bool Vbat_critical_value_activate = false;

	timeout tt1(msec( 500));
	timeout tt2(msec(1000));
	timeout tt3(msec(333));

	for(;;)
	{
// 		if(tt1)
// 		{
// 			tt1.ack();
// 			leds[5]->green.toggle();
// 		}
// 		if(tt2)
// 		{
// 			tt2.ack();
// 			leds[6]->green.toggle();
// 		}
// 		if(tt3)
// 		{
// 			tt3.ack();
// 			leds[7]->green.toggle();
// 		}
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
			default:
				debug.write(ch);
			}
		}

		if(!bt_uart.empty())
		{
			ch = bt_uart.read();
			switch(ch)
			{
			case '\r':
				bt_uart.write('\n');
				break;
			case 's':
			case 'S':
				shutdown();
				break;
			default:
				bt_uart.write(ch);
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

		if(debug_sender) 
 		{
			// IR sensors debug
			format(debug, "%5 - ") % time_cnt;
			for(int i = 0; i < ADC_IR_COUNT; i++)
			{
				format(debug, "% : %4 \t") % i % adc_IR[i]->value();

				// set period of leds by value from adc - doesn't work
				if(abs(adc_last_values[i] - adc_IR[i]->value()) > adc_IR_threshold)
				{
					adc_last_values[i] = adc_IR[i]->value();
					leds[sensor_to_led(i)]->green.blink(msec(adc_IR[i]->value()/128));
				}
				send_avakar_packet(bt_uart, i, adc_IR[i]->value());
				
			}
			format(debug, "I: % \t Iref: % \t Vbat: % ")
				% adc_I.value()
				% adc_Iref.value()
				% adc_Vbat.value();

			format(debug, "encL: %9; encR: %9")
				% encoder_left.value()
				% encoder_right.value();

 			send(debug, "\n");

			send_avakar_packet(bt_uart, 10, adc_Vbat.value());
			
			packet.write(encoder_left.value());
			packet.write(encoder_right.value());
			packet.send(bt_uart, 11);
			
			// leds test - rounding light - doesn't work
// 			leds[get_led_near_sensor(led_round)]->green.on();
// 			leds[get_led_near_sensor(led_round-1)]->green.off();
// 			led_round++;
// 			led_round = led_round % 8;
// 			format(debug, "%  => %  (% )\t\t")
// 			% led_round
// 			% get_led_near_sensor(led_round)
// 			% get_led_near_sensor(led_round-1);

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
				//shutdown(); // TODO
			}
		} else {
			if (Vbat_critical_value_activate == true) {
// 				for (auto l: leds) {
// 					l->green.off();
// 					l->red.off();
// 				}
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

