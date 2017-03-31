/*
 *  baudrate.hpp
 *
 * Created: 28.5.2016 17:00:00
 *  Author: kubas
 */ 


#ifndef BAUDRATE_HPP_
#define BAUDRATE_HPP_


uint16_t calc_baudrate(const uint32_t& baudrate, const uint32_t& base_freq = F_CPU)  // FIX ME: use fixed point arithmetic
{
	double bsel = (double(base_freq) / (baudrate<<4)) - 1;
	if(bsel == 0)
		return 0;
	int8_t bscale = 0;
	while(bscale > -6 && bsel < 2048)
	{
		bsel *= 2;
		--bscale;
	}
	return uint16_t(bsel + 0.5) | bscale << 12;
}

int16_t calc_baudrate_error(const uint32_t& baudrate, const uint16_t& baudctrl, const uint32_t& base_freq = F_CPU)  // FIX ME: use fixed point arithmetic
{
	int8_t bscale = (baudctrl & 0xF000)>>12;
	if(bscale & 0x08)
		bscale |= 0xF0;
	uint32_t calc = base_freq / (16 * (pow(2, bscale) * (baudctrl & 0x0FFF) + 1));
	//format(debug, "real baud: % ; bscale: % ; bsel: % ; calc baud: % \n") % baudrate % bscale % (baudctrl & 0x0FFF) % calc;
	if(baudrate < 400000)
		return (baudrate * 10000) / (calc)-10000;
	return ((baudrate * 1000) / (calc)-1000) * 10;
}


#endif /* BAUDRATE_HPP_ */
