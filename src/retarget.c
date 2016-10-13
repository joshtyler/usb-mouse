/*----------------------------------------------------------------------------
 * Name:    Retarget.c
 * Purpose: 'Retarget' layer for target-dependent low level functions
 * Note(s):
 *----------------------------------------------------------------------------
 * This file is part of the �Vision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <rt_misc.h>
#include "uart.h"

#pragma import(__use_no_semihosting_swi)


extern int sendchar (int c);
extern int getkey   (void);


struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;


int fputc(int c, FILE *f) {
	uart_putchar(c);
	return c;
}


int fgetc(FILE *f) {
  return 0; //SYSTEM DOES NOT SUPPORT INPUT
}


int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}


void _ttywrch(int c) {
  uart_putchar(c);
}


void _sys_exit(int return_code) {
label:  goto label;  /* endless loop */
}
