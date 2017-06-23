#ifndef IR_SENSOR_HPP_
#define IR_SENSOR_HPP_

class ir_sensor_t
{
public:
	typedef adc_t::adc_value_type value_type;
	typedef adc_t::adc_pin_type index_type;

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

	ir_sensor_t(index_type adc, eeprom::addr_type calibration_addr = -1)
		: m_adc(adc), m_calibration()
	{
		if (eeprom::is_valid_address(calibration_addr))
			load_calibration(calibration_addr);
	}

	value_type raw_value() { return s_raw_off[m_adc] - s_raw_on[m_adc]; }

	value_type value()
	{
// 		int32_t v = avrlib::clamp(raw_value(), m_calibration.meas_min, m_calibration.meas_max);
// 		v -= m_calibration.meas_min;
// 		v *= m_calibration.gain;
// 		v >>= m_calibration.gain_den_exp;
// 		v = avrlib::clamp(v, m_calibration.calib_min, m_calibration.calib_max);
// 		v = m_calibration.calib_max - v;
// 		return v;
		return raw_value();
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

	static const index_type sc_sensors_count = 9;

private:
	index_type m_adc;
	calibration_t m_calibration;

	static TC0_t& s_blink_timer;
	static ADC_t& s_adc;

	static volatile value_type s_raw_on[sc_sensors_count];
	static volatile value_type s_raw_off[sc_sensors_count];
	static const index_type s_sensors_tab[sc_sensors_count];
	static volatile uint8_t s_sensor_group;

	static register8_t* s_evch;

public: // static
	static void init(const uint8_t evch)
	{
		s_evch = (&EVSYS_CH0MUX + evch);
		
		for(auto& v: s_raw_on)
			v = 0;
		for(auto& v: s_raw_off)
			v = 0;

		PORTC.REMAP |= PORT_TC0A_bm | PORT_TC0B_bm;
		
		const uint16_t per = 53333; // 3.3 ms
		const uint16_t pwr = 100;

		s_blink_timer.PER = per;
		s_blink_timer.CCA = pwr;
		s_blink_timer.CCB = pwr;
		s_blink_timer.CCC = 150;
		s_blink_timer.CCD = per >> 1;
		s_blink_timer.CTRLB = TC0_CCAEN_bm | TC0_CCBEN_bm | TC_WGMODE_SINGLESLOPE_gc;
		off();

		s_adc.PRESCALER = ADC_PRESCALER_DIV16_gc;
		s_adc.REFCTRL = ADC_REFSEL_INTVCC_gc; // Ref: Vcc/1.6
		s_adc.EVCTRL = ADC_SWEEP_012_gc | (ADC_EVSEL_0123_gc + (evch << ADC_EVSEL_gp)) | ADC_EVACT_SYNCSWEEP_gc;
		s_adc.CTRLB = ADC_RESOLUTION_12BIT_gc;
		s_adc.CTRLA = ADC_ENABLE_bm;
		s_adc.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;
		s_adc.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc;
		s_adc.CH1.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;
		s_adc.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN2_gc;
		s_adc.CH2.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;
		s_adc.CH2.MUXCTRL = ADC_CH_MUXPOS_PIN3_gc;
		s_adc.CH2.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_MED_gc;
	}

	static void on()
	{
		s_blink_timer.CTRLA = TC_CLKSEL_DIV2_gc;
	}

	static void off()
	{
		s_blink_timer.CTRLA = TC_CLKSEL_OFF_gc;
		s_blink_timer.CTRLC = 0;
		s_blink_timer.CNT = 0;
		*s_evch = EVSYS_CHMUX_TCC0_CCC_gc;
	}

	static bool is_on()
	{
		return s_blink_timer.CTRLA != 0;
	}

	static void adc_intr()
	{
		if(*s_evch == EVSYS_CHMUX_TCC0_CCC_gc)
		{
			*s_evch = EVSYS_CHMUX_TCC0_CCD_gc;
			s_raw_on[s_sensor_group + 0] = s_adc.CH0RES;
			s_raw_on[s_sensor_group + 1] = s_adc.CH1RES;
			s_raw_on[s_sensor_group + 2] = s_adc.CH2RES;
		}
		else
		{
			*s_evch = EVSYS_CHMUX_TCC0_CCC_gc;
			s_raw_off[s_sensor_group + 0] = s_adc.CH0RES;
			s_raw_off[s_sensor_group + 1] = s_adc.CH1RES;
			s_raw_off[s_sensor_group + 2] = s_adc.CH2RES;
			s_sensor_group += 3;
			if(s_sensor_group == sc_sensors_count)
				s_sensor_group = 0;
			s_adc.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc + (s_sensors_tab[s_sensor_group + 0] << ADC_CH_MUXPOS_gp);
			s_adc.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc + (s_sensors_tab[s_sensor_group + 1] << ADC_CH_MUXPOS_gp);
			s_adc.CH2.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc + (s_sensors_tab[s_sensor_group + 2] << ADC_CH_MUXPOS_gp);
		}
	}
};

const ir_sensor_t::value_type ir_sensor_t::calibration_t::calib_min = 0;
const ir_sensor_t::value_type ir_sensor_t::calibration_t::calib_max = 1000;
const uint8_t ir_sensor_t::calibration_t::gain_den_exp = 16;
const int32_t ir_sensor_t::calibration_t::gain_den = 1L<<ir_sensor_t::calibration_t::gain_den_exp;

volatile ir_sensor_t::value_type ir_sensor_t::s_raw_on[ir_sensor_t::sc_sensors_count];
volatile ir_sensor_t::value_type ir_sensor_t::s_raw_off[ir_sensor_t::sc_sensors_count];
const ir_sensor_t::index_type ir_sensor_t::s_sensors_tab[ir_sensor_t::sc_sensors_count] = { 9, 10, 11, 12, 1, 0, 15, 13, 14 };
volatile uint8_t ir_sensor_t::s_sensor_group = 0;

TC0_t& ir_sensor_t::s_blink_timer = TCC0;
ADC_t& ir_sensor_t::s_adc = ADCB;
register8_t* ir_sensor_t::s_evch = nullptr;

ISR(ADCB_CH2_vect)
{
	ir_sensor_t::adc_intr();
}

#endif