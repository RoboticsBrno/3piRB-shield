#ifndef SPI_HPP_
#define SPI_HPP_

#include "miscellaneous.hpp"

class spi_t
{
public:
	spi_t(SPI_t& spi, const pin_t& cs)
		:m_spi(spi), m_cs(cs), m_buffer(nullptr), m_length(0)
	{}
		
	void init()
	{
		m_spi.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_1_gc | SPI_PRESCALER_DIV128_gc;
		dummy_read(m_spi.STATUS);
		dummy_read(m_spi.DATA);
		m_spi.INTCTRL = SPI_INTLVL_MED_gc;
	}
	
	bool ready() const { return m_buffer == nullptr; }

	void wait() const
	{
		while (!ready())
			nop();
// 		send(debug, "rec:\n");
// 		for(uint8_t i = 0; i != m_l; ++i)
// 			format(debug, "%x2 ") % m_b[i];
// 		debug.write('\n');
	}
	
	bool xchange(uint8_t* buffer, const uint8_t& length)
	{
		if(!ready())
			return false;
		m_cs.set_low();
		if(length == 0 || buffer == nullptr)
		{
			m_cs.set_high();
			m_buffer = nullptr;
			return true;
		}
		m_length = length;
		m_l = m_length;
		m_b = buffer;
		m_buffer = buffer + m_length - 1;
		//format(debug, "S %x2\n") % *m_buffer;
		m_spi.DATA = *m_buffer;
		return true;
	}
	
	void process_intr()
	{
		uint8_t d = m_spi.DATA;
		*m_buffer = d;
		//format(debug, "R %x2\n") % *m_buffer;
		if(--m_length == 0)
		{
			m_cs.set_high();
			m_buffer = nullptr;
			//format(debug, "E\n");
		}
		else
		{
			d = *(--m_buffer);
			//format(debug, "T %x2\n") % *m_buffer;
			m_spi.DATA = d;
		}
	}
	
private:
	SPI_t& m_spi;
	pin_t m_cs;
	uint8_t volatile * volatile m_buffer;
	volatile uint8_t m_length;
	uint8_t m_l;
	uint8_t* m_b;
};

#endif
