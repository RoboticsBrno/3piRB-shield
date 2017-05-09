#ifndef SPI_HPP_
#define SPI_HPP_

#include "miscellaneous.hpp"

namespace SPI
{

class Interface;

class Device
{
	friend class Interface;
public:
	Device(Interface& spi, pin_t& cs)
		:m_spi(spi), m_cs(cs), m_buffer(nullptr), m_length(0), m_next_device(nullptr)
	{}

	bool xchange(uint8_t* buffer, const uint8_t& length);
	
	bool ready() const { return m_buffer == nullptr; }

	void wait() const
	{
		while (!ready())
			nop();
	}
	
private:
	Interface& m_spi;
	pin_t &m_cs;
	uint8_t * volatile m_buffer;
	volatile uint8_t m_length;
	Device * volatile m_next_device;
};

class Interface
{
	friend class Device;
public:
	Interface(SPI_t& spi)
		:m_spi(spi), m_active_device(nullptr), m_int_lvl(SPI_INTLVL_MED_gc)
	{}

	void open()
	{
		m_spi.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_1_gc | SPI_PRESCALER_DIV128_gc;
		dummy_read(m_spi.STATUS);
		dummy_read(m_spi.DATA);
		_enable_intr();
	}
	
	void process_intr()
	{
		uint8_t d = m_spi.DATA;
		*(m_active_device->m_buffer) = d;
		if(--m_active_device->m_length == 0)
		{
			m_active_device->m_cs.set_low();
			m_active_device->m_buffer = nullptr;
			_next();
		}
		else
		{
			d = *(--m_active_device->m_buffer);
			m_spi.DATA = d;
		}
	}

private:
	void transfer(Device* device)
	{
		_enable_intr(false);
		if (m_active_device == nullptr)
		{
			m_active_device = device;
			_start();
		}
		else
		{
			Device* p = m_active_device;
			while(p->m_next_device != nullptr)
				p = p->m_next_device;
			p->m_next_device = device;
		}
		_enable_intr();
	}

	void _start()
	{
		m_active_device->m_cs.set_high();
		if(m_active_device->m_length == 0 || m_active_device->m_buffer == nullptr)
		{
			m_active_device->m_buffer = nullptr;
			m_active_device->m_cs.set_low();
			_next();
		}
		else
		{
			m_spi.DATA = *(m_active_device->m_buffer);
		}
	}

	void _next()
	{
		Device* old = m_active_device;
		m_active_device = m_active_device->m_next_device;
		if(m_active_device != nullptr)
		{
			old->m_next_device = nullptr;
			_start();
		}
	}

	void _enable_intr(bool en = true)
	{
		m_spi.INTCTRL = en ? m_int_lvl : SPI_INTLVL_OFF_gc;
	}

	SPI_t& m_spi;
	Device * volatile m_active_device;
	SPI_INTLVL_t m_int_lvl;
};

bool Device::xchange(uint8_t* buffer, const uint8_t& length)
{
	if(!ready())
		return false;
	m_length = length;
	m_buffer = buffer + m_length - 1;
	m_spi.transfer(this);
	return true;
}

} // namespace spi

#endif
