/*
 * adc.h
 *
 * Created: 01.04.2017 0:17:49
 *  Author: JarekParal
 */ 

#ifndef ADC_H_
#define ADC_H_

#ifndef MAX_ADC_PINS_COUNT
#define MAX_ADC_PINS_COUNT 16
#endif

class adc_t
{
	public:
	typedef uint8_t adc_pin_type;
	typedef int16_t adc_value_type;

	adc_t(const adc_pin_type adc_pin)
	:m_adc_pin(adc_pin), m_value(-1), m_updated(false), m_index(s_max_index < MAX_ADC_PINS_COUNT ? s_max_index : -1)
	{
		if(s_max_index < MAX_ADC_PINS_COUNT)
			s_adcs[s_max_index] = this;
		else
			AVRLIB_ASSERT(s_max_index < MAX_ADC_PINS_COUNT);

		if (s_max_index == 0)
			ADCA.CH0.CTRL |= ADC_CH_START_bm;
		
		s_max_index++;
	}
	
	~adc_t()
	{
		if(m_index != uint8_t(-1))
		{
			s_adcs[m_index] = nullptr;
			if(s_max_index > (m_index + 1))
			{
				s_adcs[m_index] = s_adcs[s_max_index - 1];
				s_adcs[m_index]->m_index = m_index;
			}
			--s_max_index;
		}
	}
	
	adc_value_type value()
	{
		m_updated = false;
		return m_value;
	} 
	adc_value_type operator ()() { return value(); }

	bool new_value() const { return m_updated; }

	adc_pin_type pin() { return m_adc_pin; }
	uint8_t index() { return m_index; }
	uint8_t max_index() { return s_max_index; }

	protected:
	adc_pin_type m_adc_pin;
	adc_value_type m_value;
	bool m_updated;
	uint8_t m_index;
	
	// static
	public:
	static void init()
	{
		ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;
		ADCA.CH0.MUXCTRL = 0; // ADC_PIN 0

		ADCA.PRESCALER = ADC_PRESCALER_DIV64_gc;
		ADCA.REFCTRL = ADC_REFSEL_INTVCC_gc; // Ref: Vcc/1.6
		ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc;
		ADCA.CTRLA = ADC_ENABLE_bm;
	}

	static void process_all()
	{
		if (s_max_index != 0)
		{
			if(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm)
			{
				ADCA.CH0.INTFLAGS = ADC_CH_CHIF_bm;
				
				s_adcs[s_act_index]->m_value = ADCA.CH0RES;
				s_adcs[s_act_index]->m_updated = true;

				if(++s_act_index == s_max_index)
					s_act_index = 0;

				ADCA.CH0.MUXCTRL = ((s_adcs[s_act_index]->m_adc_pin)<<3);

				ADCA.CH0.CTRL |= ADC_CH_START_bm;
			}
		}
	}
	protected:
	static uint8_t s_max_index;
	static uint8_t s_act_index;
	static adc_t* s_adcs[];
};


uint8_t adc_t::s_max_index = 0;
uint8_t adc_t::s_act_index = 0;
adc_t* adc_t::s_adcs[MAX_ADC_PINS_COUNT] = { nullptr };

#endif /* ADC_H_ */