#ifndef IR_SENSOR_HPP_
#define IR_SENSOR_HPP_

class ir_sensor_t
{
public:
	typedef adc_t::adc_value_type value_type;

private:

	struct calibration_t
	{
		calibration_t()
			: meas_min(0),
			  meas_max(4095),
			  gain(gain_den)
			{}

		value_type meas_min;
		value_type meas_max;
		int32_t gain;
		static const value_type calib_min;
		static const value_type calib_max;
		static const uint8_t gain_den_exp;
		static const int32_t gain_den;
	};

public:

	ir_sensor_t(adc_t::adc_pin_type adc, eeprom::addr_type calibration_addr = -1)
		: m_adc(adc), m_calibration()
	{
		if (eeprom::is_valid_address(calibration_addr))
			load_calibration(calibration_addr);
	}

	value_type raw_value() { return m_adc.value(); }

	value_type value()
	{
		int32_t v = avrlib::clamp(raw_value(), m_calibration.meas_min, m_calibration.meas_max);
		v -= m_calibration.meas_min;
		v *= m_calibration.gain;
		v >>= m_calibration.gain_den_exp;
		v = avrlib::clamp(v, m_calibration.calib_min, m_calibration.calib_max);
		v = m_calibration.calib_max - v;
		return v;
	}

	void calibrate(void(*process)())
	{
		off();
		wait(msec(100), process);
		m_calibration.meas_max = raw_value();
		on();
		wait(msec(100), process);
		m_calibration.meas_min = raw_value();
		m_calibration.gain = m_calibration.gain_den
			* (m_calibration.calib_max - m_calibration.calib_min)
			/ (m_calibration.meas_max - m_calibration.meas_min);
	}

	void save_calibration(eeprom::addr_type addr, bool flush = false) { eeprom::write(addr, m_calibration, flush); }
	void load_calibration(eeprom::addr_type addr) { eeprom::read(addr, m_calibration); }
	eeprom::addr_type calibration_size() const { return sizeof m_calibration; }
	template<typename Stream>
	void print_calibration(Stream& stream)
	{
		format_spgm(stream, PSTR("meas_min %4\n")) % m_calibration.meas_min;
		format_spgm(stream, PSTR("meas_max %4\n")) % m_calibration.meas_max;
		format_spgm(stream, PSTR("calib_min %4\n")) % m_calibration.calib_min;
		format_spgm(stream, PSTR("calib_max %4\n")) % m_calibration.calib_max;
		format_spgm(stream, PSTR("gain %9\n")) % m_calibration.gain;
	}

private:
	adc_t m_adc;
	calibration_t m_calibration;

	static TC0_t& s_blink_timer;
	static TC1_t& s_current_timer;

	static const uint16_t sc_power_step = 10;

public: // static
	static void init()
	{
		PORTC.REMAP |= PORT_TC0A_bm | PORT_TC0B_bm;
		
		s_blink_timer.PER = 15625; // 2 s
		s_blink_timer.CCA = 14843;
		s_blink_timer.CCB = 14843;
		s_blink_timer.CTRLB = TC0_CCAEN_bm | TC0_CCBEN_bm | TC_WGMODE_DSTOP_gc;
		s_blink_timer.CTRLA = TC_CLKSEL_OFF_gc;
		s_blink_timer.CTRLC = TC0_CMPA_bm | TC0_CMPB_bm;

		s_current_timer.PER = 320;
		s_current_timer.CCA = 240;
		s_current_timer.CCB = 240;
		s_current_timer.CTRLB = TC1_CCAEN_bm | TC1_CCBEN_bm | TC_WGMODE_SINGLESLOPE_gc;
		s_current_timer.CTRLA = TC_CLKSEL_DIV1_gc;
	}

	static uint16_t inc_power()
	{
		uint16_t p = s_current_timer.CCA;
		if (p < sc_power_step)
			p = 0;
		else
			p -= sc_power_step;
		s_current_timer.CCABUF = p;
		s_current_timer.CCBBUF = p;
		return s_current_timer.PER - p;
	}

	static uint16_t dec_power()
	{
		uint16_t p = s_current_timer.CCA;
		const uint16_t m = s_current_timer.PER;
		p += sc_power_step;
		if (p > m)
			p = m;
		s_current_timer.CCABUF = p;
		s_current_timer.CCBBUF = p;
		return m - p;
	}

	static void on()
	{
		s_blink_timer.CTRLA = TC_CLKSEL_DIV1024_gc;
	}

	static void off()
	{
		s_blink_timer.CTRLA = TC_CLKSEL_OFF_gc;
		s_blink_timer.CTRLC = TC0_CMPA_bm | TC0_CMPB_bm;
		s_blink_timer.CNT = 0;
	}

	static bool is_on()
	{
		return s_blink_timer.CTRLA != 0;
	}
};

const ir_sensor_t::value_type ir_sensor_t::calibration_t::calib_min = 0;
const ir_sensor_t::value_type ir_sensor_t::calibration_t::calib_max = 1000;
const uint8_t ir_sensor_t::calibration_t::gain_den_exp = 16;
const int32_t ir_sensor_t::calibration_t::gain_den = 1L<<ir_sensor_t::calibration_t::gain_den_exp;

TC0_t& ir_sensor_t::s_blink_timer = TCC0;
TC1_t& ir_sensor_t::s_current_timer = TCC1;

#endif