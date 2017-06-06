/*
 *  settings.hpp
 *
 * Created: 31.5.2016 11:58:20
 *  Author: kubas
 */ 


#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

// MPU frequency in Hz
// F_CPU should be defined in project settings
// This value is informative only, corresponding frequency must be set in init_clock() [clock.hpp]
#ifndef F_CPU
#define F_CPU 32000000UL
#endif


// debug UART

#ifndef DEBUG_UART_BAUDRATE
#define DEBUG_UART_BAUDRATE 115200
#endif

#ifndef DEBUG_UART_RX_BUFF_SIZE
#define DEBUG_UART_RX_BUFF_SIZE 16
#endif

#ifndef DEBUG_UART_TX_BUFF_SIZE
#define DEBUG_UART_TX_BUFF_SIZE 128
#endif


// DATA UARTs

#ifndef DATA_UART_RX_BUFF_SIZE
#define DATA_UART_RX_BUFF_SIZE 128
#endif

#ifndef DATA_UART_TX_BUFF_SIZE
#define DATA_UART_TX_BUFF_SIZE 128
#endif

// BT UART

#ifndef BT_UART_BAUDRATE
#define BT_UART_BAUDRATE 115200
#endif

// 3pi UART

#ifndef R3PI_UART_BAUDRATE
#define R3PI_UART_BAUDRATE 115200
#endif

#ifndef R3PI_UART_FAST_BAUDRATE
#define R3PI_UART_FAST_BAUDRATE 1250000
#endif


#endif /* SETTINGS_HPP_ */
