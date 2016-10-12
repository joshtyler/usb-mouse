// Basic polling driver for Freescale KL46 family microcontroller
// This driver is TX only for now

void uartSetup(int baud);

void uart_putChar(char c);

void uart_puts(const char *str);
