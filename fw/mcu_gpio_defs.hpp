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

// BT_XMEGA_3V3
pin_t pin_bt_i2c_sda (PORTF, 0);
pin_t pin_bt_i2c_scl (PORTF, 1);
pin_t pin_bt_uart0_rx(PORTF, 2);
pin_t pin_bt_uart0_tx(PORTF, 3);
pin_t pin_bt_uart1_rx(PORTF, 6);
pin_t pin_bt_uart1_tx(PORTF, 7);

// ARM_XMEGA_3V3
pin_t pin_arm_i2c0_sda (PORTC, 0);
pin_t pin_arm_i2c0_scl (PORTC, 1);
pin_t pin_arm_i2c1_sda (PORTD, 0);
pin_t pin_arm_i2c1_scl (PORTD, 1);
pin_t pin_arm_uart0_rx(PORTC, 2);
pin_t pin_arm_uart0_tx(PORTC, 3);
pin_t pin_arm_uart1_rx(PORTC, 6);
pin_t pin_arm_uart1_tx(PORTC, 7);

// 3pi
pin_t pin_3pi_rx(PORTE, 2);
pin_t pin_3pi_tx(PORTE, 3);

// USB
pin_t pin_usb_dp(PORTD, 7);
pin_t pin_usb_dm(PORTD, 6);
pin_t pin_usb_vsence(PORTJ, 1);

// buttons
pin_t pin_btn_a(PORTQ, 0);
pin_t pin_btn_b(PORTQ, 1);
pin_t pin_btn_c(PORTQ, 2);
pin_t pin_btn_d(PORTQ, 3);
pin_t pin_btn_pwr(PORTJ, 2);

// LEDs
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

// internal SPI
pin_t pin_MOSI(PORTE, 5);
pin_t pin_MISO(PORTE, 6);
pin_t pin_SCK (PORTE, 7);

// encoders
pin_t pin_enc_left_a(PORTF, 4);
pin_t pin_enc_left_b(PORTF, 5);
pin_t pin_enc_left_cs(PORTJ, 6);
pin_t pin_enc_right_a(PORTD, 4);
pin_t pin_enc_right_b(PORTD, 5);
pin_t pin_enc_right_cs(PORTJ, 7);
	// TCE1, TCD1

// IMU
pin_t pin_imu_int(PORTH, 3);
pin_t pin_imu_cs(PORTH, 6);

// LDC
pin_t pin_SDA(PORTE, 0);
pin_t pin_SCL(PORTE, 1);
pin_t pin_LDC_clk(PORTE, 4);
pin_t pin_LDC_int(PORTJ, 3);
pin_t pin_LDC_shdn(PORTJ, 4);

// 3pi
pin_t pin_3pi_power(PORTH, 7);
pin_t pin_3pi_reset(PORTH, 0);

// enables
pin_t pin_AREF_EN(PORTJ, 5);

pin_t pin_IR_front(PORTC, 5);
pin_t pin_IR_back_left_right(PORTC, 4);
	// TCC0 + TCC1

pin_t pin_SHDN(PORTJ, 0);

pin_t pin_bat_chg_en(PORTH, 1);
pin_t pin_stepup_disable(PORTH, 2);
pin_t pin_arm_bt_uart_en(PORTH, 4);
pin_t pin_disp_backlight_en(PORTH, 5);

// analog
pin_t pin_AREF(PORTA, 0);
pin_t pin_IR1(PORTA, 1);
pin_t pin_IR2(PORTA, 2);
pin_t pin_IR3(PORTA, 3);
pin_t pin_IR4(PORTA, 4);
pin_t pin_IR8(PORTA, 5);
pin_t pin_IR9(PORTA, 6);
pin_t pin_IR7(PORTA, 7);
pin_t pin_IR6(PORTB, 0);
pin_t pin_IR5(PORTB, 1);
pin_t pin_IOREF(PORTB, 2);
pin_t pin_I(PORTB, 3);
pin_t pin_IREF(PORTB, 4);
pin_t pin_V_BAT(PORTB, 5);
pin_t pin_3pi_VBST(PORTB, 6);
pin_t pin_bat_chg_stat(PORTB, 7);

void init_mcu_gpio()
{
	// disable JTAG	
	CCP = CCP_IOREG_gc;
	MCU_MCUCR = MCU_JTAGD_bm;
	
	pin_debug_rx.pullup();
	pin_debug_tx.make_high();

	pin_bt_i2c_sda.make_input();
	pin_bt_i2c_scl.make_input();
	pin_bt_uart0_rx.pullup();
	pin_bt_uart0_tx.make_high();
	pin_bt_uart1_rx.make_input();
	pin_bt_uart1_tx.make_input();

	pin_arm_i2c0_sda.make_input();
	pin_arm_i2c0_scl.make_input();
	pin_arm_i2c1_sda.make_input();
	pin_arm_i2c1_scl.make_input();
	pin_arm_uart0_rx.pullup();
	pin_arm_uart0_tx.make_high();
	pin_arm_uart1_rx.make_input();
	pin_arm_uart1_tx.make_input();

	pin_3pi_rx.pullup();
	pin_3pi_tx.make_high();

	pin_usb_dp.input_disable();
	pin_usb_dm.input_disable();
	pin_usb_vsence.make_input();

	pin_btn_a.pullup();
	pin_btn_b.pullup();
	pin_btn_c.pullup();
	pin_btn_d.pullup();
	pin_btn_a.make_inverted();
	pin_btn_b.make_inverted();
	pin_btn_c.make_inverted();
	pin_btn_d.make_inverted();
	pin_btn_pwr.pullup();
	pin_btn_pwr.make_inverted();

	pin_led_0.make_output();
	pin_led_1.make_output();
	pin_led_2.make_output();
	pin_led_3.make_output();
	pin_led_4.make_output();
	pin_led_5.make_output();
	pin_led_6.make_output();
	pin_led_7.make_output();

	pin_MOSI.make_output();
	pin_MISO.pullup();
	pin_SCK.make_output();

	pin_enc_left_a.pulldown();
	pin_enc_left_b.pulldown();
	pin_enc_left_cs.make_inverted();
	pin_enc_left_cs.make_low();
	pin_enc_right_a.pulldown();
	pin_enc_right_b.pulldown();
	pin_enc_right_cs.make_inverted();
	pin_enc_right_cs.make_low();

	pin_imu_int.pullup();
	pin_imu_int.make_inverted();
	// pin_imu_cs.make_inverted(); // because of short
	pin_imu_cs.make_low();

	pin_SDA.make_input();
	pin_SCL.make_input();

	pin_LDC_clk.make_output();
	pin_LDC_int.pullup();
	pin_LDC_int.make_inverted();
	pin_LDC_shdn.make_high();

	pin_3pi_power.make_low();
	pin_3pi_reset.make_low();
	
	pin_AREF_EN.make_low(); // enable 2,5 V stabilizer for IR phototransistor 
	pin_IR_front.make_low(); // enable front IR leds
	pin_IR_back_left_right.make_low();

	pin_SHDN.make_inverted();
	pin_SHDN.make_low();

	pin_bat_chg_en.make_low();
	pin_stepup_disable.make_low();
	pin_arm_bt_uart_en.make_inverted();
	pin_arm_bt_uart_en.make_low();
	pin_disp_backlight_en.make_low();

	// disable digital input buffer of analog pins
	PORTCFG.MPCMASK = 0xFF;
	PORTA.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTCFG.MPCMASK = 0xFF;
	PORTB.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
}


#endif /* MCU_GPIO_DEFS_HPP_ */
