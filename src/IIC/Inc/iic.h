//IIC (master) Driver for Freescale Kinetis KL46Z
//Specialised for KL46Z development board
//Polling mode only
//No FreeRTOS integration is provided, as it is assumed that there is only one appication trying to use the port at once

#ifndef IIC_H
#define IIC_H

#include <stdbool.h>
#include <stdint.h>

//SDA is at PTE24
#define SDA_PORT PORTE
#define SDA_GPIO PTE
#define SDA_PIN 24

//SCL is at PTE25
#define SCL_PORT PORTE
#define SCL_GPIO PTE
#define SCL_PIN 25

void iic_init(void);
void iic_transaction(uint8_t addr, uint8_t *data, bool read);
static void iic_startLl(uint8_t addr, bool read);
static void iic_stopLl(void);
static void iic_writeLl(uint8_t data);
static uint8_t iic_receiveLl(void);

#endif
