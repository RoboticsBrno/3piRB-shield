#ifndef EEPROM_HPP_
#define EEPROM_HPP_


class eeprom
{
	typedef uint16_t int_addr_type;
public:
	typedef int_addr_type addr_type;
	typedef uint16_t length_type;

	static const addr_type size = EEPROM_SIZE;

	static void init()
	{
		NVM.CTRLB = NVM_EEMAPEN_bm;
		while((NVM.STATUS & NVM_NVMBUSY_bm) != 0){};
	}

	static bool is_valid_address(addr_type addr) { return addr < size; }

	static bool write_byte(const addr_type& addr, const uint8_t& byte, const bool& Flush = true)
	{
		if(!is_valid_address(addr))
			return false;
		int_addr_type faddr = MAPPED_EEPROM_START + addr;
		++m_addr;
		uint8_t& eeprom = *((uint8_t*)(faddr));
		if(eeprom != byte)
		{
			if(!m_flushed && page_address(m_last_addr) != page_address(faddr))
				flush(m_last_addr);
			eeprom = byte;
			m_flushed = false;
			if(Flush)
				flush(addr);
			else
				m_last_addr = faddr;
			return true;
		}
		return false;
	}
	
	static uint8_t read_byte(const addr_type& addr)
	{
		++m_addr;
		return *((uint8_t*)(MAPPED_EEPROM_START + addr));
	}
		
	static bool write_block(addr_type addr, length_type length, uint8_t const * data, const bool& Flush = true)
	{
		bool changed = false;
		for(; length != 0; --length, ++addr, ++data)
			changed |= write_byte(addr, *data);
		if(Flush)
			flush();
		return changed;
	}

	static void read_block(addr_type addr, length_type length, uint8_t *data)
	{
		for(; length != 0; --length, ++addr, ++data)
			*data = read_byte(addr);
	}

	template <typename T>
	static bool write(addr_type address, T data, const bool& Flush = true)
	{
		return write_block(address, sizeof data, (uint8_t const *)&data, Flush);
	}

	template <typename T>
	static T read(addr_type address)
	{
		T res;
		read_block(address, sizeof res, (uint8_t *)&res);
		return res;
	}

	template <typename T>
	static T read(addr_type address, T& res)
	{
		read_block(address, sizeof res, (uint8_t *)&res);
		return res;
	}

	static bool set_address(const addr_type& addr)
	{
		if(addr >= EEPROM_SIZE)
			return false;
		m_addr = addr;
		return true;
	}

	static addr_type get_next_address()
	{
		return m_addr;
	}

	static void write(const uint8_t& data)
	{
		write_byte(m_addr, data, false);
	}

	static uint8_t read()
	{
		return read_byte(m_addr);
	}

	static bool needs_flush()
	{
		return !m_flushed;
	}
	
	static bool flush(addr_type addr = -1, const bool& force = false)
	{
		if(m_flushed && !force)
			return false;
		if(addr == addr_type(-1))
			addr = m_last_addr;
		cli();
		while((NVM_STATUS & NVM_NVMBUSY_bm) != 0){};
		NVM_ADDR0 = addr;
		NVM_ADDR1 = addr >> 8;
		NVM_CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
		CCP = CCP_IOREG_gc;
		NVM_CTRLA = NVM_CMDEX_bm;
		while((NVM_STATUS & NVM_NVMBUSY_bm) != 0){};
		sei();
		m_flushed = true;
		return true;
	}

	template <typename T>
	static bool flush(T const * const ptr)
	{
		return flush(int_addr_type(ptr) - MAPPED_EEPROM_START, true);
	}

	template <typename T>
	static const T& get_var(const int_addr_type& addr) { return *((T*)(MAPPED_EEPROM_START + addr)); }
private:
	static int_addr_type page_address(const int_addr_type& addr) { return addr & ~(EEPROM_PAGE_SIZE - 1); }

	static bool m_flushed;
	static addr_type m_last_addr;
	static addr_type m_addr;
};

bool eeprom::m_flushed;
eeprom::addr_type eeprom::m_last_addr;
eeprom::addr_type eeprom::m_addr;

 
#endif