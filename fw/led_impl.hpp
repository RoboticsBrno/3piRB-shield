#ifndef LED_INPL_HPP_
#define LED_IMPL_HPP_

namespace detail
{
	
typedef timeout::time_type led_time_type;

const led_time_type led_process_period = msec(10);

class led_impl_t
{
public:
	typedef led_time_type time_type;
	typedef int32_t diff_type;
	typedef uint8_t count_type;

	led_impl_t()
		:m_timer(0), m_on_time(0), m_off_time(0), m_stop_counter(0), m_state(0), m_blink_state(false), m_value(false)
	{
		m_timer.cancel();
	}
	void set() { m_state = 1; }
	void clear() { m_state = 2; }
	void toggle()
	{
		switch(m_state)
		{
			case 0: m_state = 3; break;
			case 1: m_state = 2; break;
			case 2: m_state = 1; break;
			case 3: break;
		}
	}
	void set(const uint8_t& value)
	{
		if(value)
			set();
		else
			clear();
	}
	void blink(time_type period, diff_type difference = 0, const count_type& count = 0)
	{
		period >>= 1;
		if (period < led_process_period)
			period = led_process_period;
		difference >>= 1;
		if ((diff_type(period) - avrlib::abs(difference)) < diff_type(led_process_period))
			difference = (difference < 0) ? (-diff_type(period) + led_process_period) : (period - led_process_period);
		m_on_time = period + difference;
		m_off_time = period - difference;
		m_stop_counter = count;
		m_timer.set_timeout(m_on_time);
		if (blinking())
			return;
		m_timer.restart();
		m_blink_state = true;
		toggle();
	}
	void on()
	{
		m_timer.cancel();
		set();
	}
	void off()
	{
		m_timer.cancel();
		clear();
	}
	void operator() (const bool& value) { if (value) on(); else off(); }
	void operator = (const bool& value) { if (value) on(); else off(); }
	uint8_t get() const { return m_value; }
	uint8_t blinking() const { return m_timer.running(); }
	virtual bool process()
	{
		switch(m_state)
		{
			case 0: break;
			case 1: _set(); m_state = 0; break;
			case 2: _clear(); m_state = 0; break;
			case 3: _toggle(); m_state = 0; break;
		}
		if(m_timer)
		{
			m_timer.ack();
			_toggle();
			m_timer.set_timeout(m_blink_state ? m_off_time : m_on_time);
			if(m_on_time == 0) // last blink ends
			{
				m_timer.cancel();
				_toggle();
			}
			else
			{
				if (m_blink_state)
				{
					if (--m_stop_counter == 0)
					{
						m_on_time = 0;
					}
					m_blink_state = false;
				}
				else
				{
					m_blink_state = true;
				}
			}
		}
		return m_value;
	}
private:
	void _set()    { m_value = true; }
	void _clear()  { m_value = false; }
	void _toggle() { m_value = !m_value; }
	
	timeout m_timer;
	time_type m_on_time;
	time_type m_off_time;
	count_type m_stop_counter;
	uint8_t m_state;
	bool m_blink_state;
	bool m_value;
};

} // namespace detail

#endif