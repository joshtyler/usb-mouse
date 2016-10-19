// Basic polling driver for Freescale KL46 family microcontroller
// The driver is FreeRTOS aware, and only allows one device access at a time.
// This driver is TX only for now

#ifndef UART_H
#define UART_H

//Setup UART 0
void uart_init(int baud);

//Send char from UART 0
void uart_putchar(char c);

//Send string from UART 0
void uart_puts(const char *str);

//Internal function to calculate error between transmit baud rate and ideal baud rate
static int calcBaudError(int clk, int sbr, int osr, int baud);

#endif
