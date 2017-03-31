/*
 *  serial_number.hpp
 *
 * Created: 29.5.2016 17:49:23
 *  Author: Kubas
 */ 


#ifndef SERIAL_NUMBER_HPP_
#define SERIAL_NUMBER_HPP_

const uint8_t serial_number_length = 15;
char serial_number[serial_number_length + 1] = {0};
char serial_number_str[2 * serial_number_length + 1] = {0};

void init_serial_number()
{
	const char digits[] = "0123456789ABCDEF";
	serial_number[0x00] = MCU_DEVID0;
	serial_number[0x01] = MCU_DEVID1;
	serial_number[0x02] = MCU_DEVID2;
	serial_number[0x03] = MCU_REVID;
	serial_number[0x04] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, LOTNUM0));
	serial_number[0x05] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, LOTNUM1));
	serial_number[0x06] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, LOTNUM2));
	serial_number[0x07] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, LOTNUM3));
	serial_number[0x08] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, LOTNUM4));
	serial_number[0x09] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, LOTNUM5));
	serial_number[0x0A] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, WAFNUM));
	serial_number[0x0B] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, COORDX0));
	serial_number[0x0C] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, COORDX1));
	serial_number[0x0D] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, COORDY0));
	serial_number[0x0E] = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, COORDY1));
	serial_number[0x0F] = 0;
	for(uint8_t i = 0; i != serial_number_length; ++i)
	{
		const uint8_t n = serial_number[i];
		serial_number_str[2 * i + 0] = digits[n >> 4];
		serial_number_str[2 * i + 1] = digits[n & 0x0F];
	}
	serial_number_str[2 * serial_number_length] = 0;
}


#endif