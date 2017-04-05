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

const uint8_t pin_led_count = 8;
pin_t pin_led_0(PORTK, 0);
pin_t pin_led_1(PORTK, 1);
pin_t pin_led_2(PORTK, 2);
pin_t pin_led_3(PORTK, 3);
pin_t pin_led_4(PORTK, 4);
pin_t pin_led_5(PORTK, 7);
pin_t pin_led_6(PORTK, 6);
pin_t pin_led_7(PORTK, 5);

const uint8_t led_near_sensor[pin_led_count] = {
	4, 3, 2, 1, 9, 7, 6, 5
};
uint8_t get_led_near_sensor(int8_t led) {
	if(led < 0)
		led += pin_led_count;

	return led_near_sensor[led] - 1;
}

uint8_t sensor_to_led(uint8_t number_of_sensor)
{
	switch(number_of_sensor) 
	{
	case 1:
		return 3;
	case 2:
		return 2;
	case 3:
		return 1;
	case 4:
		return 0;
	case 5:
		return 7;
	case 6: 
		return 6;
	case 7: 
		return 5;
	case 9:
	default:
		return 4;

	}
}

pin_t pin_AREF_EN(PORTJ, 5);
pin_t pin_IR_front(PORTC, 5);
pin_t pin_IR_back_left_right(PORTC, 4);
pin_t pin_IR1(PORTA, 1);

pin_t pin_SHDN(PORTJ, 0);
pin_t pin_PWR_BTN(PORTJ, 2);

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

	pin_SHDN.make_inverted();
	pin_SHDN.make_low();
	pin_PWR_BTN.pullup();
	pin_PWR_BTN.make_inverted();
}


#endif /* MCU_GPIO_DEFS_HPP_ */
