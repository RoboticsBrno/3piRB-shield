/*
 *  build_info.hpp
 *
 * Created: 29.5.2016 13:06:23
 *  Author: Kubas
 *
 * This file is updated by python script build_info_updater.py at the beginning of every build.
 * DO NOT MODIFY IT MANUALLY!
 */ 

#if !defined(BUILD_INFO_HPP_)

#include "build_info_printer.hpp"

#else

/***** VALUES *****/
const BUILD_INFO::build_number_type BUILD_INFO::build_number				PROGMEM = 149;
const BUILD_INFO::build_time_type   BUILD_INFO::build_time					PROGMEM = 1491363979;
const char                          BUILD_INFO::build_time_str			[] PROGMEM = "05.04.2017 05:46:19";
const char                          BUILD_INFO::build_config			[] PROGMEM = "Debug";
const char                          BUILD_INFO::build_author			[] PROGMEM = "noname";


#endif