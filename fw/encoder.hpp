/*
 *  encoder.hpp
 *
 * Created: 2.6.2016 9:55:15
 *  Author: kubas
 */ 


#ifndef ENCODER_HPP_
#define ENCODER_HPP_


class encoder_t
{
	typedef int16_t overflow_type;
	
public:
	typedef int32_t value_type;
	enum class index_recognition : uint8_t
	{
		none = 0,
		state_00 = EVSYS_QDIRM_00_gc,
		state_01 = EVSYS_QDIRM_01_gc,
		state_10 = EVSYS_QDIRM_10_gc,
		state_11 = EVSYS_QDIRM_00_gc
	};
	
private:
	class comparator_t
	{
	public:
		typedef Lambda<void()> callback_type;
	
		comparator_t(encoder_t& encoder, const uint8_t& channel)
			:m_encoder(encoder), m_channel(channel), m_ovf_cnt(0)
		{}
			
		void value(const value_type& v)
		{
			m_ovf_cnt = v >> 16;
			*(&m_encoder.m_tc.CCA + m_channel) = v & 0xFFFF;
		}
		value_type value() const { return (value_type(m_ovf_cnt) << 16) | *(&m_encoder.m_tc.CCA + m_channel); }
		
		void enable(const TC_CCAINTLVL_t& priority)
		{
			m_encoder.m_tc.INTCTRLB = (m_encoder.m_tc.INTCTRLB & ~(TC1_CCAINTLVL_gm << (2 * m_channel))) | ((priority & TC1_CCAINTLVL_gm) << (2 * m_channel));
		}
		bool is_enabled() const { return (m_encoder.m_tc.INTCTRLB & (TC1_CCAINTLVL_gm << (2 * m_channel))) != 0; }
		
		void clear_flag() { m_encoder.m_tc.INTFLAGS = TC1_CCAIF_bm << m_channel; }
		
		void callback(callback_type callback)
		{
			m_callback = callback;
		}
		
		void process_intr()
		{
			if(m_encoder.m_overflow == m_ovf_cnt)
				m_callback();
		}
		
	private:
		encoder_t& m_encoder;
		const uint8_t m_channel;
		volatile overflow_type m_ovf_cnt;
		callback_type m_callback;
	};
	
	friend class comparator_t;
	
public:

	encoder_t(TC1_t& tc, pin_t pin_in, const uint8_t& event_channel, const EVSYS_DIGFILT_t& input_filter = EVSYS_DIGFILT_4SAMPLES_gc)
		:m_tc(tc),
		 m_pin(pin_in),
		 m_period(0),
		 m_check_period(false),
		 m_long_value(false),
		 m_incremental_mode(false),
		 m_overflow(0),
		 comparator_a(*this, 0),
		 comparator_b(*this, 1),
		 c_event_channel(event_channel),
		 c_input_filter(input_filter)
	{}
	
	void mode_incremental(const value_type period = 0, const bool& set_source = true)
	{
		m_tc.CTRLA = TC_CLKSEL_OFF_gc;
		m_tc.CTRLFSET = TC_CMD_RESET_gc;
		m_incremental_mode = true;
		m_overflow = 0;
		if(set_source)
		{
			m_pin.sence_both_edges();
			*(&EVSYS_CH0MUX + c_event_channel) = pin_event_mux(m_pin);
			*(&EVSYS_CH0CTRL + c_event_channel) = c_input_filter;
		}
		m_check_period = period != 0;
		if(m_check_period && uint32_t(period) < 65536)
		{
			m_tc.PER = period;
			m_check_period = false;
			m_long_value = false;
		}
		else
		{
			m_long_value = true;
			m_tc.INTCTRLA = TC_OVFINTLVL_MED_gc;
		}
		m_period = period;
		resume();
	}
	
	void mode_quadrature(const value_type period = 0, const index_recognition& index_recognition_mode = index_recognition::none)
	{
		m_tc.CTRLA = TC_CLKSEL_OFF_gc;
		m_tc.CTRLFSET = TC_CMD_RESET_gc;
		m_incremental_mode = false;
		m_overflow = 0;
		m_pin.sence_level();
		pin_t(m_pin.port, m_pin.bp + 1).sence_level();
		*(&EVSYS_CH0MUX + c_event_channel) = pin_event_mux(m_pin);
		*(&EVSYS_CH0CTRL + c_event_channel) = c_input_filter | EVSYS_QDEN_bm;
		m_check_period = period != 0;
		if(m_check_period && uint32_t(period) < 65536)
		{
			if(index_recognition_mode != index_recognition::none)
			{
				pin_t(m_pin.port, m_pin.bp + 2).sence_both_edges();
				*(&EVSYS_CH0CTRL + c_event_channel) |= uint8_t(index_recognition_mode) | EVSYS_QDIEN_bm;
			}
			m_tc.PER = period;
			m_check_period = false;
			m_long_value = false;
		}
		else
		{
			m_long_value = true;
			m_tc.INTCTRLA = TC_OVFINTLVL_MED_gc;
		}
		m_period = period;
		m_tc.CTRLD = TC_EVACT_QDEC_gc | (TC_EVSEL_CH0_gc + (c_event_channel<<TC1_EVSEL_gp));
		resume();
	}
	
	void disable()
	{
		m_tc.CTRLA = TC_CLKSEL_OFF_gc;
		m_tc.CTRLFSET = TC_CMD_RESET_gc;
		*(&EVSYS_CH0MUX + c_event_channel)  = EVSYS_CHMUX_OFF_gc;
		*(&EVSYS_CH0CTRL + c_event_channel)  = 0;
		if(!m_incremental_mode)
		{
			m_pin.sence_both_edges();
			pin_t(m_pin.port, m_pin.bp + 1).sence_both_edges();
		}
		m_incremental_mode = false;
		m_long_value = false;
		m_check_period = false;
		m_period = 0;
		m_overflow = 0;
	}
	
	void pause() { m_tc.CTRLA = TC_CLKSEL_OFF_gc; }
	void resume() { m_tc.CTRLA = (TC_CLKSEL_EVCH0_gc + (c_event_channel<<TC1_CLKSEL_gp)); }
	
	bool is_running() const { return m_tc.CTRLA != TC_CLKSEL_OFF_gc; }
	
	value_type value() const
	{
		if(m_long_value)
		{
			overflow_type overflow, cnt;
			do 
			{
				overflow = m_overflow;
				cnt = m_tc.CNT;
			} while (overflow != m_overflow);
			value_type t = (value_type(overflow)<<16) + uint16_t(cnt);
			if(m_check_period)
				return t % m_period;
			return t;
		}
		else
		{
			return m_tc.CNT;
		}
	}
	
	value_type operator()() const
	{
		return value();
	}
	
	void clear()
	{
		uint8_t clksrc = m_tc.CTRLA;
		m_tc.CTRLA = TC_CLKSEL_OFF_gc;
		m_tc.INTFLAGS = TC1_OVFIF_bm;
		m_tc.CNT = 0;
		m_overflow = 0;
		m_tc.CTRLA = clksrc;
	}
	
	void make_inverted()
	{
		if(m_incremental_mode)
			m_tc.CTRLFSET = TC1_DIR_bm;
		else
			m_pin.make_inverted();
	}
	
	void make_noninverted()
	{
		if(m_incremental_mode)
			m_tc.CTRLFCLR = TC1_DIR_bm;
		else
			m_pin.make_noninverted();
	}
	
	bool is_inverted() const
	{
		if(m_incremental_mode)
			return (m_tc.CTRLFSET & TC1_DIR_bm) != 0;
		else
			return m_pin.is_inverted();
	}
	
	void process_intr()
	{
		if((m_tc.CTRLFSET & TC1_DIR_bm) == 0)
			++m_overflow;
		else
			--m_overflow;
	}
	
	void set_value(value_type v)
	{
		if(m_period != 0)
			v %= m_period;
		m_tc.CNT = v & 0xFFFF;
		m_overflow = v >> 16;
	}
	
	
	
private:
	TC1_t& m_tc;
	pin_t m_pin;
	volatile value_type m_period;
	volatile bool m_check_period;
	volatile bool m_long_value;
	volatile bool m_incremental_mode;
	volatile overflow_type m_overflow;
	
public:
	comparator_t comparator_a;
	comparator_t comparator_b;
	
	const uint8_t c_event_channel;
	const EVSYS_DIGFILT_t c_input_filter;
};


#endif // ENCODER_HPP_
