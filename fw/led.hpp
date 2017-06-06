/*
 *  led.hpp
 *
 * Created: 29.5.2016 23:41:53
 *  Author: kubas
 */ 


#ifndef LED_HPP_
#define LED_HPP_


#include "led_impl.hpp"

class led_t
{
public:
	typedef detail::led_time_type time_type;
	typedef uint8_t count_type;

	led_t(pin_t pin, const bool& run = true)
		:m_pin(pin), m_refresh_timer(detail::led_process_period), red(), green()
	{
		if(!run)
			m_refresh_timer.cancel();
	}
	
	void run()
	{
		if(m_refresh_timer.running())
			return;
		m_refresh_timer.start();
		m_refresh_timer.force();
		process();
	}
		
	bool process()
	{
		if(!m_refresh_timer)
			return false;
		m_refresh_timer.ack();
		uint8_t state = 
			(  red.process() << 0) |
			(green.process() << 1);
		switch(state)
		{
			case 0: m_pin.make_input();	break;
			case 1: m_pin.make_low();	break;
			case 2:	m_pin.make_high();	break;
			case 3:
				m_pin.make_output();
				m_pin.toggle();
				break;
		}
		return true;
	}
	
private:
	pin_t m_pin;
	timeout m_refresh_timer;
	
public:
	detail::led_impl_t red;
	detail::led_impl_t green;
};


led_t led_0 (pin_led_0, false);
led_t led_1 (pin_led_1, false);
led_t led_2 (pin_led_2, false);
led_t led_3 (pin_led_3, false);
led_t led_4 (pin_led_4, false);
led_t led_5 (pin_led_5, false);
led_t led_6 (pin_led_6, false);
led_t led_7 (pin_led_7, false);
			   
const uint8_t led_count = 8;
led_t* const leds[led_count] = { &led_0, &led_1, &led_2, &led_3, &led_4, &led_5, &led_6, &led_7 };

void run_leds()
{
	for(auto l: leds)
	{
		l->run();
		wait(detail::led_process_period / led_count);
	}
}


#endif /* LED_HPP_ */
