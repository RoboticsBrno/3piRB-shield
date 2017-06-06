#ifndef TIME_HPP_
#define TIME_HPP_

struct base_timer_type
{
	typedef uint32_t time_type;
	time_type value() const { return getTicksCount(); }
	time_type operator()() const { return getTicksCount(); }
}; base_timer_type base_timer;

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
base_timer_type::time_type wait(base_timer_type::time_type time, Process process, int)
{
	return avrlib::wait(base_timer, time, process, 0);
}

#define msec(value) (value)
#define  sec(value) ((value) * 1000L)

#define to_msec(value) (value)
#define to_sec (value) ((value) / 1000)

#endif