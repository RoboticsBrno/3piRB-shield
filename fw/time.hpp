/*
 *  time.hpp
 *
 * Created: 20.1.2015 13:37:53
 *  Author: kubas
 */ 


#ifndef TIME_HPP_
#define TIME_HPP_


#include "avrlib/timer_xmega.hpp"
#include "avrlib/counter.hpp"
#include "avrlib/stopwatch.hpp"

AVRLIB_XMEGA_TIMER(base_timer_tc, TCD0);
typedef avrlib::counter<base_timer_tc, uint32_t, uint32_t, true> base_timer_type;
base_timer_type base_timer;
ISR(TCD0_OVF_vect) { base_timer.tov_interrupt(); }

void init_time()
{
	base_timer_tc::tov_level(TC_OVFINTLVL_MED_gc);
	base_timer_tc::clock_source(avrlib::timer_fosc_2);
}

struct stopwatch
	:avrlib::stopwatch<base_timer_type>
{
	stopwatch(bool run = true)
		:avrlib::stopwatch<base_timer_type>(base_timer)
	{
		if(run)
			return;
		stop();
		clear();
	}
};

struct timeout
	:avrlib::timeout<base_timer_type>
{
	timeout(avrlib::timeout<base_timer_type>::time_type timeout)
		:avrlib::timeout<base_timer_type>(base_timer, timeout)
	{
	}
};

void wait(base_timer_type::time_type time)
{
	avrlib::wait(base_timer, time);
}

template <typename Process>
void wait(base_timer_type::time_type time, Process process)
{
	avrlib::wait(base_timer, time, process);
}

template <typename Process>
void wait(base_timer_type::time_type time, Process process, int)
{
	avrlib::wait(base_timer, time, process, 0);
}

#define tick_per_usec (F_CPU / 1000000 / 2)

#define usec(value) (value * tick_per_usec)
#define msec(value) (value * tick_per_usec * 1000UL)
#define  sec(value) (value * tick_per_usec * 1000000UL)

#define to_usec(value) (value /  tick_per_usec)
#define to_msec(value) (value / (tick_per_usec * 1000UL))
#define to_sec (value) (value / (tick_per_usec * 1000000UL))


#endif /* TIME_HPP_ */
