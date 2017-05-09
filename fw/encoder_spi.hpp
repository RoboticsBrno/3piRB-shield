#ifndef ENCODER_SPI_HPP_
#define ENCODER_SPI_HPP_

class encoder_spi_t
{
	typedef uint16_t raw_type;
public:
	typedef int16_t angle_type;
	typedef int16_t magnitude_type;
	typedef uint8_t agc_type;

	encoder_spi_t(SPI::Interface& spi, pin_t& cs, const timeout::time_type read_timeout = msec(200))
		:m_spi(SPI::Device(spi, cs)),
		 m_updated(false),
		 m_read_timeout(read_timeout),
		 m_state(0),
		 m_angle(0),
		 m_magnitude(0),
		 m_agc(0),
		 m_alarm_lo(false),
		 m_alarm_hi(false),
		 m_cordic_overflow(false),
		 m_parity_error(false),
		 m_command_error(false),
		 m_framing_error(false),
		 m_in_error_flag(0),
		 m_in_parity_error(0)
	{}

	bool is_updated()
	{
		if(!m_updated)
			return false;
		m_updated = false;
		return true;
	}

	angle_type value() const { return m_angle; }
	angle_type angle() const
	{
		if(m_parity_error || error() || m_alarm_lo || m_alarm_hi || cordic_overflow())
			return -1;
		//return m_angle;
		return m_angle/4; // TODO: HACK pro snadnejsi otestovani puvodniho programu
	}
	magnitude_type magnitude() const { return m_magnitude; }
	agc_type agc() const { return m_agc; }
	bool alarm_lo() const { return m_alarm_lo; }
	bool alarm_hi() const { return m_alarm_hi; }
	bool cordic_overflow() const { return m_cordic_overflow; }
	bool error() const { return m_parity_error | m_command_error | m_framing_error; }

	bool parity_error()
	{
		if(!m_parity_error)
			return false;
		m_parity_error = false;
		return true;
	}

	bool command_error()
	{
		if(!m_command_error)
			return false;
		m_command_error = false;
		return true;
	}

	bool framing_error()
	{
		if(!m_framing_error)
			return false;
		m_framing_error = false;
		return true;
	}

	uint8_t in_error_flag()
	{
		return m_in_error_flag;
	}

	uint8_t in_parity_error()
	{
		return m_in_parity_error;
	}

	raw_type const* raw_data()
	{
		return m_raw_buff;
	}

	uint8_t raw_size()
	{
		return c_raw_size;
	}

	void start()
	{
		m_read_timeout.restart();
	}

	void stop()
	{
		m_read_timeout.cancel();
	}

	void process()
	{
		if(!m_spi.ready())
			return;
		switch(m_state)
		{
		case 0:
			if(m_read_timeout)
			{
				_read(c_addr_diagnostic);
				m_state = 1;
				m_read_timeout.ack();
				m_in_error_flag = 0;
				m_in_parity_error = 0;
			}
			break;
		case 1:
			_set_in_errors(m_state - 1);
			m_raw_buff[m_state - 1] = m_raw;
			_read(c_addr_magnitude);
			m_state = 2;
			break;
		case 2:
			m_alarm_hi			 = m_raw & c_comp_high_bm;
			m_alarm_lo			 = m_raw & c_comp_low_bm;
			m_cordic_overflow	 = m_raw & c_COF_bm;
			m_agc				 = m_raw & c_agc_gm;
			_set_in_errors(m_state - 1);
			m_raw_buff[m_state - 1] = m_raw;
			_read(c_addr_angular);
			m_state = 3;
			break;
		case 3:
			m_magnitude			 = m_raw & c_magnitude_gm;
			_set_in_errors(m_state - 1);
			m_raw_buff[m_state - 1] = m_raw;
			_read(c_addr_error);
			m_state = 4;
			break;
		case 4:
			m_angle				 = m_raw & c_angle_gm;
			_set_in_errors(m_state - 1);
			m_raw_buff[m_state - 1] = m_raw;
			_nop();
			m_state = 5;
			break;
		case 5:
			m_parity_error		|= m_raw & c_parity_error_bm;
			m_command_error		|= m_raw & c_command_error_bm;
			m_framing_error		|= m_raw & c_framing_error_bm;
			_set_in_errors(m_state - 1);
			m_raw_buff[m_state - 1] = m_raw;
			_nop();
			m_state = 6;
			break;
		case 6:
			_set_in_errors(m_state - 1);
			m_raw_buff[m_state - 1] = m_raw;
			_nop();
			m_updated = true;
			m_state = 0;
			break;
		}
	}

private:

	void _set_in_errors(uint8_t pos)
	{
		pos = c_raw_size - pos - 1;
		m_in_error_flag |= ((m_raw & c_error_mask) != 0) << pos;
		m_in_parity_error |= (_add_parity(m_raw & ~c_parity_mask) != m_raw) << pos;
	}

	void _transfer()
	{
		m_spi.xchange(reinterpret_cast<uint8_t*>(const_cast<raw_type*>(&m_raw)), sizeof(m_raw));
	}

	void _nop()
	{
		m_raw = c_addr_nop;
		_transfer();
	}

	SPI::Device m_spi;
	bool m_updated;
	timeout m_read_timeout;
	uint8_t m_state;
	angle_type m_angle;
	magnitude_type m_magnitude;
	agc_type m_agc;
	bool m_alarm_lo;
	bool m_alarm_hi;
	bool m_cordic_overflow;
	bool m_parity_error;
	bool m_command_error;
	bool m_framing_error;
	uint8_t m_in_error_flag;
	uint8_t m_in_parity_error;
	volatile raw_type m_raw;

	static const uint8_t c_raw_size = 6;
	raw_type m_raw_buff[c_raw_size];

	static raw_type _add_parity(raw_type data)
	{
		for(raw_type tmp = data; tmp != 0; tmp >>= 1)
			if(tmp & 1)
				data ^= c_parity_mask;
		return data;
	}

	void _read(const raw_type& addr)
	{
		m_raw = _add_parity(c_read_cmd | (addr & c_addr_mask));
		_transfer();
	}
	
	// communication bits
	static const raw_type c_parity_mask				= 0x8000;
	static const raw_type c_error_mask				= 0x4000;
	static const raw_type c_read_cmd				= 0x4000;
	static const raw_type c_addr_mask				= 0x3FFF;

	// register addresses
	static const raw_type c_addr_nop				= 0x0000; // R or W ?
	static const raw_type c_addr_error				= 0x0001; // R
	static const raw_type c_addr_programming		= 0x0003; // R/W
	static const raw_type c_addr_zero_position_H	= 0x0016; // R/W + program
	static const raw_type c_addr_zero_position_L	= 0x0017; // R/W + program
	static const raw_type c_addr_settings1			= 0x0018; // R/W + program
	static const raw_type c_addr_settings2			= 0x0019; // R/W + program
	static const raw_type c_addr_diagnostic			= 0x3FFC; // R
	static const raw_type c_addr_magnitude			= 0x3FFD; // R
	static const raw_type c_addr_raw_angular		= 0x3FFE; // R
	static const raw_type c_addr_angular			= 0x3FFF; // R

	// bits of c_addr_error
	static const raw_type c_parity_error_bm			= 0x0004;
	static const raw_type c_command_error_bm		= 0x0002;
	static const raw_type c_framing_error_bm		= 0x0001;

	// bits of c_addr_programming
	static const raw_type c_verify_bm				= 0x0040;
	static const raw_type c_burn_bm					= 0x0008;
	static const raw_type c_reload_otp				= 0x0004;
	static const raw_type c_programming_enable_bm	= 0x0001;

	// bits of c_addr_zero_position_H
	static const raw_type c_zero_position_H_gm		= 0x00FF;
	
	// bits of c_addr_zero_position_L
	static const raw_type c_zero_position_L_gm		= 0x003F;
	static const raw_type c_comp_l_error_en_bm		= 0x0040;
	static const raw_type c_comp_h_error_en_bm		= 0x0080;

	// bits of c_addr_settings1
	static const raw_type c_noiseset_bm				= 0x0002;
	static const raw_type c_dir_bm					= 0x0004;
	static const raw_type c_uvw_abi_bm				= 0x0008;
	static const raw_type c_daecdis_bm				= 0x0010;
	static const raw_type c_abibin_bm				= 0x0020;
	static const raw_type c_dataselect_bm			= 0x0040;
	static const raw_type c_pwm_on_bm				= 0x0080;

	// bits of c_addr_settings2
	static const raw_type c_uvwpp_gm				= 0x0007;
	static const raw_type c_hys_gm					= 0x0018;
	static const raw_type c_abires_gm				= 0x00E0;

	// bits of c_addr_diagnostic
	static const raw_type c_comp_high_bm			= 0x0800;
	static const raw_type c_comp_low_bm				= 0x0400;
	static const raw_type c_COF_bm					= 0x0200;
	static const raw_type c_LF_bm					= 0x0100;
	static const raw_type c_agc_gm					= 0x00FF;

	// bits of c_addr_magnitude
	static const raw_type c_magnitude_gm			= 0x3FFF;

	// bits of c_addr_raw_angular and c_addr_angular
	static const raw_type c_angle_gm				= 0x3FFF;
};

#endif