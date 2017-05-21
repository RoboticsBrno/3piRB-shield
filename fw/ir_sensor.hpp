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

public: // static
	static void on()
	{
		pin_IR_front.set_high();
		pin_IR_back_left_right.set_high();
	}

	static void off()
	{
		pin_IR_front.set_low();
		pin_IR_back_left_right.set_low();
	}

	static bool is_on()
	{
		return pin_IR_front.read() && pin_IR_back_left_right.read();
	}
};

const ir_sensor_t::value_type ir_sensor_t::calibration_t::calib_min = 0;
const ir_sensor_t::value_type ir_sensor_t::calibration_t::calib_max = 1000;
const uint8_t ir_sensor_t::calibration_t::gain_den_exp = 16;
const int32_t ir_sensor_t::calibration_t::gain_den = 1L<<ir_sensor_t::calibration_t::gain_den_exp;

#endif