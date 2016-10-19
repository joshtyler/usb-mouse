//Routines to use the buttons and LEDs on the KL46Z dev board

#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include <MKL46Z4.H>

//LED1 (green) is at PTD5
#define LED1_PORT PORTD
#define LED1_GPIO PTD
#define LED1_PIN 5

//LED2 (red) is at PTE29
#define LED2_PORT PORTE
#define LED2_GPIO PTE
#define LED2_PIN 29

//SW1 is at PTC3
#define SW1_PORT PORTC
#define SW1_GPIO PTC
#define SW1_PIN 3

 //SW2 is at PTC12
#define SW2_PORT PORTC
#define SW2_GPIO PTC
#define SW2_PIN 12


void gpio_init(void);

static void init_pin(PORT_Type * port, uint8_t pin, bool output, bool pullup);

//Macros to perform common functions
//LEDs
#define setLED1() LED1_GPIO->PSOR = (1u<<LED1_PIN)
#define setLED2() LED2_GPIO->PSOR = (1u<<LED2_PIN)

#define clearLED1() LED1_GPIO->PCOR = (1u<<LED1_PIN)
#define clearLED2() LED2_GPIO->PCOR = (1u<<LED2_PIN)

#define toggleLED1() LED1_GPIO->PTOR = (1u<<LED1_PIN)
#define toggleLED2() LED2_GPIO->PTOR = (1u<<LED2_PIN)

//Switches
//Note: This library uses the convention 1= pressed, 0= not pressed
//Therefore the logic is reversed here due to pullup resistors
#define readSW1() (SW1_GPIO->PDIR & (1u<<SW1_PIN)? 0 : 1)
#define readSW2() (SW2_GPIO->PDIR & (1u<<SW2_PIN)? 0 : 1)

#endif
