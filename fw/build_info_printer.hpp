/*
 *  build_info_printer.hpp
 *
 * Created: 29.5.2016 13:06:23
 *  Author: Kubas
 */ 

#ifndef BUILD_INFO_HPP_
#define BUILD_INFO_HPP_


#include <avr/pgmspace.h>

struct BUILD_INFO
{
	typedef uint16_t build_number_type;
	typedef uint32_t build_time_type;
	typedef uint8_t indent_count_type;
	
	template <typename Serial>
	static void print(Serial& serial, const indent_count_type& indent = 0, const char& indent_char = '\t')
	{
		print_string(serial, PSTR("Build "), indent, indent_char, false);
		print_number(serial, pgm_read_word(&build_number));
		print_string(serial, build_time_str, indent + 1, indent_char);
		print_string(serial, PSTR("config: ")  , indent + 1, indent_char, false);
		print_string(serial, build_config);
		print_string(serial, PSTR("author: ")  , indent + 1, indent_char, false);
		print_string(serial, build_author);
	}
	
	static const build_number_type build_number;
	static const build_time_type   build_time;
	static const char              build_time_str			[];
	static const char              build_config				[];
	static const char              build_author				[];
	
private:
	template <typename Serial>
	static void print_indent(Serial& serial, uint8_t n, const char& c)
	{
		for(; n != 0; --n)
			serial.write(c);
	}
	template <typename Serial>
	static void print_number(Serial& serial, build_number_type n, const indent_count_type& indent = 0, const char& indent_char = '\t')
	{
		print_indent(serial, indent, indent_char);
		char buf[5];
		uint8_t i = 0;
		do {
			buf[i++] = (n % 10) + '0';
			n /= 10;
		} while (n != 0);
		for(--i; i != 255; --i)
			serial.write(buf[i]);
		serial.write('\n');
	}
	template <typename Serial>
	static void print_string(Serial& serial, const char* str, const indent_count_type& indent = 0, const char& indent_char = '\t', const bool& endline = true)
	{
		print_indent(serial, indent, indent_char);
		for(char c; (c = pgm_read_byte(str)) != 0; ++str)
			serial.write(c);
		if(endline)
			serial.write('\n');
	}
};

#include "build_info.hpp"

#endif