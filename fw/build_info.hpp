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
const BUILD_INFO::build_number_type BUILD_INFO::build_number				PROGMEM = 177;
const BUILD_INFO::build_time_type   BUILD_INFO::build_time					PROGMEM = 1494244986;
const char                          BUILD_INFO::build_time_str			[] PROGMEM = "08.05.2017 14:03:06";
const char                          BUILD_INFO::build_config			[] PROGMEM = "Release";
const char                          BUILD_INFO::build_author			[] PROGMEM = "kubas";


#endif