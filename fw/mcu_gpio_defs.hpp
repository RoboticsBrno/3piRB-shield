/*
 *  mcu_gpio_defs.hpp
 *
 * Created: 16.5.2016 9:16:14
 *  Author: kubas
 */ 

#ifndef MCU_GPIO_DEFS_HPP_
#define MCU_GPIO_DEFS_HPP_


#include "avrlib/xmega_pin.hpp"

using avrlib::pin_t;

// DEBUG
pin_t pin_debug_rx(PORTD, 2);
pin_t pin_debug_tx(PORTD, 3);

// UART_BT_XMEGA_3V3
pin_t pin_bt_uart_rx(PORTF, 2);
pin_t pin_bt_uart_tx(PORTF, 3);

pin_t pin_btn_a(PORTQ, 0);
pin_t pin_btn_b(PORTQ, 1);
pin_t pin_btn_c(PORTQ, 2);
pin_t pin_btn_d(PORTQ, 3);

pin_t pin_led_0(PORTK, 0);
pin_t pin_led_1(PORTK, 1);
pin_t pin_led_2(PORTK, 2);
pin_t pin_led_3(PORTK, 3);
pin_t pin_led_4(PORTK, 4);
pin_t pin_led_5(PORTK, 5);
pin_t pin_led_6(PORTK, 6);
pin_t pin_led_7(PORTK, 7);

pin_t pin_AREF_EN(PORTJ, 5);
pin_t pin_IR_front(PORTC, 5);
pin_t pin_IR_back_left_right(PORTC, 4);
pin_t pin_IR1(PORTA, 1);


void init_mcu_gpio()
{
	// disable JTAG	
	CCP = CCP_IOREG_gc;
	MCU_MCUCR = MCU_JTAGD_bm;
	
	pin_debug_rx.pullup();
	pin_debug_tx.make_high();

	pin_bt_uart_rx.pullup();
	pin_bt_uart_tx.make_high();

	pin_led_0.make_output();
	pin_led_1.make_output();
	pin_led_2.make_output();
	pin_led_3.make_output();
	pin_led_4.make_output();
	pin_led_5.make_output();
	pin_led_6.make_output();
	pin_led_7.make_output();
	
	pin_AREF_EN.make_output(); // enable 2,5 V stabilizer for IR phototransistor 
	pin_IR_front.make_output(); // enable front IR leds
	pin_IR_back_left_right.make_output();


}


#endif /* MCU_GPIO_DEFS_HPP_ */
