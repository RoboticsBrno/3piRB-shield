/*
 *  miscellaneous.hpp
 *
 * Created: 28.5.2016 11:50:00
 *  Author: kubas
 */ 


#ifndef MISCELLANEOUS_HPP_
#define MISCELLANEOUS_HPP_


#define nop() __asm__ volatile ("nop")

void avrlib::assertion_failed(char const * msg, char const * file, int lineno)
{
	send_spgm(debug, PSTR("Assertion failed: "));
	send(debug, msg);
	send(debug, "\n    ");
	send(debug, file);
	send(debug, "(");
	send_int(debug, lineno);
	send(debug, ")\n");
	debug.flush();
	for (;;);
}

#define assert(x) AVRLIB_ASSERT(x)

extern "C" void __cxa_pure_virtual(void)
{
	send_spgm(debug, PSTR("Pure virtual function called!\n  Execution halted.\n"));
	debug.flush();
	for (;;);
}


void flush_eeprom_page_buffer(void* const  addr)
{
	cli();
	while((NVM_STATUS & NVM_NVMBUSY_bm) != 0){};
	uint16_t addrv = uint16_t(addr);
	addrv -= MAPPED_EEPROM_START;
	NVM_ADDR0 = addrv;
	NVM_ADDR1 = addrv >> 8;
	NVM_CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
	CCP = CCP_IOREG_gc;
	NVM_CTRLA = NVM_CMDEX_bm;
	while((NVM_STATUS & NVM_NVMBUSY_bm) != 0){};
	sei();
}

uint8_t ReadCalibrationByte(uint8_t index)
{
	uint8_t result;

	/* Load the NVM Command register to read the calibration row. */
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	result = pgm_read_byte(index);

	/* Clean up NVM Command register. */
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;

	return result;
}

template <typename T>
volatile void dummy_read(T v) {}

uint8_t pin_event_mux(const pin_t& pin)
{
	return EVSYS_CHMUX_PORTA_PIN0_gc + pin.bp +
		(((uint16_t(&pin.port) - uint16_t(&PORTA)) / (uint16_t(&PORTB) - uint16_t(&PORTA)))<<3);
}


#endif /* MISCELLANEOUS_HPP_ */
