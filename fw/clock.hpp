/*
 *  clock.hpp
 *
 * Created: 28.5.2016 14:45:10
 *  Author: kubas
 */ 


#ifndef CLOCK_HPP_
#define CLOCK_HPP_


template<int blink_delay, int reset_delay, int delay_denominator = 1>
void wait_for_stable_clock(const uint8_t& clock_status_bits_mask, const uint8_t& on_timeout_blink_count)
{
	uint32_t timeout_cnt = 0;
	while ((OSC.STATUS & clock_status_bits_mask) != clock_status_bits_mask)
	{
		if(++timeout_cnt == 1000000)
		{
			for(uint8_t i = 0; i != on_timeout_blink_count; ++i)
			{
				_delay_ms(blink_delay / double(delay_denominator));
				pin_led_2.toggle_dir();
			}
			_delay_ms(reset_delay / double(delay_denominator));
			reset();
		}
	}
}

void init_clock()
{

	OSC.CTRL = OSC_RC32MEN_bm | OSC_RC2MEN_bm | OSC_RC32KEN_bm;
	wait_for_stable_clock<500, 2000, 15>(OSC_RC32MRDY_bm, 7);
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
	OSC.CTRL = OSC_RC32MEN_bm | OSC_RC32KEN_bm;
	wait_for_stable_clock<500, 2000, 15>(OSC_RC32KRDY_bm, 9);
	DFLLRC32M.CTRL = DFLL_ENABLE_bm;
	
}


#endif /* CLOCK_HPP_ */
