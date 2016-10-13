//Functions to allow debug data to be output over UART
//Only compile the functions in if NDEBUG is not defined

#ifndef DEBUG_H
#define DEBUG_H

#include "uart.h"
#include <stdio.h>

#ifdef NDEBUG

	#define dbg_prinf(...)
	#define dbg_puts(str)

#else
	
	#define dbg_printf(...) printf(...) //retarget.c redefined fputc, so we can use Keil's printf
	#define dbg_puts(str) uart_puts(str)

#endif


#endif
