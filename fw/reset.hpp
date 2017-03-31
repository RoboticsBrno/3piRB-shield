/*
 *  reset.hpp
 *
 * Created: 29.5.2016 21:57:00
 *  Author: kubas
 */ 


#ifndef RESET_HPP_
#define RESET_HPP_


struct reset_t
{
	reset_t()
		:source(RST_STATUS)
	{
		RST_STATUS = 0x3F;
	}
	
	template <typename Serial>
	void print_source(Serial& serial, const char* prefix = nullptr, const char* postfix = nullptr)
	{
		if(prefix != nullptr)
			send_spgm(serial, prefix);
		bool first = true;
		print_item(serial, RST_SRF_bm  , PSTR("SW") , first);
		print_item(serial, RST_PDIRF_bm, PSTR("PDI"), first);
		print_item(serial, RST_WDRF_bm , PSTR("WDR"), first);
		print_item(serial, RST_BORF_bm , PSTR("BOR"), first);
		print_item(serial, RST_EXTRF_bm, PSTR("EXT"), first);
		print_item(serial, RST_PORF_bm , PSTR("POR"), first);
		if(postfix != nullptr)
			send_spgm(serial, postfix);
	}

	void operator()()
	{
		cli();
		CCP = CCP_IOREG_gc;
		RST_CTRL = RST_SWRST_bm;
		for(;;);
	}
	
	const uint8_t source;
	
private:
	template <typename Serial>
	void print_item(Serial& serial, const uint8_t& mask, const char* name, bool& first)
	{
		if(source & mask)
		{
			if(!first)
				send_spgm(serial, PSTR(", "));
			send_spgm(serial, name);
			first = false;
		}
	}
	
};

reset_t reset;


#endif /* RESET_HPP_ */
