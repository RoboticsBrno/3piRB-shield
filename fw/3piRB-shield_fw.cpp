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

void process();

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
#include "encoder.hpp"
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

//avrlib::timed_command_parser<base_timer_type> debug_data(base_timer, msec(100));

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

int main(void)
{
	init_mcu_gpio();
	init_clock();
	
	init_serial_number();
	
	debug.usart().open(USARTD0, calc_baudrate(DEBUG_UART_BAUDRATE, F_CPU), true);
	bt_uart.usart().open(USARTF0, calc_baudrate(BT_UART_BAUDRATE, F_CPU), true);
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

	for(auto l: leds)
	{
		l->red.off();
		l->green.off();
	}
	led_2.green.blink(msec(500));
	
	// interrupt stop btn
	//stop_btn.pin().port.INTFLAGS = PORT_INT0IF_bm | PORT_INT1IF_bm;
	//stop_btn.pin().port.INTCTRL = PORT_INT0LVL_HI_gc;
	
	char ch = 0;

	for(;;)
	{
		if(!debug.empty())
		{
			ch = debug.read();
			switch(ch)
			{
			case '\r':
				debug.write('\n');
				break;
			}
		}

		process();
	}
}
