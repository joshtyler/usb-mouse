//IIC (master) Driver for Freescale Kinetis KL46Z
//Specialised for KL46Z development board
//Polling mode only
//No FreeRTOS integration is provided, as it is assumed that there is only one appication trying to use the port at once

#ifndef IIC_H
#define IIC_H

#include <stdint.h>
#include <stdbool.h>

#include "iic.h"
#include "fsl_i2c.h"
#include "fsl_gpio.h"
#include "fsl_port.h"

/*******************************************************************************
 * Definitions for accel
 ******************************************************************************/
#define ACCEL_I2C_CLK_SRC I2C0_CLK_SRC

#define I2C_RELEASE_SDA_PORT PORTE
#define I2C_RELEASE_SCL_PORT PORTE
#define I2C_RELEASE_SDA_GPIO GPIOE
#define I2C_RELEASE_SDA_PIN 25U
#define I2C_RELEASE_SCL_GPIO GPIOE
#define I2C_RELEASE_SCL_PIN 24U
#define I2C_RELEASE_BUS_COUNT 100U
#define I2C_BAUDRATE 100000U
#define FOXS8700_WHOAMI 0xC7U
#define MMA8451_WHOAMI 0x1AU
#define ACCEL_STATUS 0x00U
#define ACCEL_XYZ_DATA_CFG 0x0EU
#define ACCEL_CTRL_REG1 0x2AU
/* FOXS8700 and MMA8451 have the same who_am_i register address. */
#define ACCEL_WHOAMI_REG 0x0DU
#define ACCEL_READ_TIMES 10U

#define BOARD_ACCEL_I2C_BASEADDR I2C0

void readAccel(int16_t *x, int16_t *y);
void BOARD_I2C_ReleaseBus(void);
bool I2C_ReadAccelWhoAmI(void);
bool I2C_WriteAccelReg(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value);
bool I2C_ReadAccelRegs(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize);
void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData);
void BOARD_I2C_ConfigurePins(void);


#endif
