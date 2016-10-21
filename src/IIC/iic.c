//IIC (master) Driver for Freescale Kinetis KL46Z
//Specialised for KL46Z development board
//Polling mode only
//No FreeRTOS integration is provided, as it is assumed that there is only one appication trying to use the port at once

#include "iic.h"
#include "gpio.h"
#include "debug.h"
#include <MKL46Z4.H>
#include <stdbool.h>
#include <stdint.h>

void iic_init(void)
{
	//Setup SDA and SCL Pins
	init_pin(SDA_PORT,SDA_PIN,false,true,5);
	init_pin(SCL_PORT,SCL_PIN,false,true,5);
	//Set low drive strength
	//Set manually as not supported in init_pin()
	SDA_PORT->PCR[SDA_PIN] &= ~PORT_PCR_DSE_MASK;
	SCL_PORT->PCR[SCL_PIN] &= ~PORT_PCR_DSE_MASK;
	
	//Enable clock to I2C peripheral
	SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK;
	
	//Set clock divider
	//I2C0 is clocked from the bus clock
	//This is Core clock/4 = 12MHz in our system
	//Set mul=1 and ICR = 31
	//i2c baud rate = bus speed/(mul*clk) = 387kbit/s
	//This is fine as our accelerometer supports much higher
	I2C0->F = I2C_F_MULT(0) | I2C_F_ICR(31);
	
	//Enable module
	I2C0->C1 = I2C_C1_IICEN_MASK;
	
	//Check that accelerometer is connected by checking whoami register
	uint8_t data = 0x0D;
	iic_transaction(0x1D,&data,false);
	iic_transaction(0x1D,&data,true);
	if(data != 0x1A)
	{
		dbg_puts("Inertial sensor comms error\r\n");
		while(1);
	}
}

void iic_transaction(uint8_t addr, uint8_t *data, bool read)
{
	//Clear stop detected bit
	if(I2C0->FLT & I2C_FLT_STOPF_MASK)
	{
		I2C0->FLT |= I2C_FLT_STOPF_MASK; //Stop detected
	}
	
	//Wait until peripheral ready
	while(!(I2C0->S & I2C_S_TCF_MASK))
	{
	}
	
	//SEND START AND ADDRESS
	iic_startLl(addr, read);
	
	
	//Check for arbitration loss/ NAK
	if(I2C0->S & I2C_S_ARBL_MASK || I2C0->S & I2C_S_RXAK_MASK)
	{
		dbg_puts("I2C Error\r\n");
		while(1);
	}
	
	//READ/WRITE DATA
	if(read)
	{
		*data = iic_receiveLl();
	} else {
		iic_writeLl(*data);
	}
	
	//STOP
	iic_stopLl();
	
	
}

static void iic_startLl(uint8_t addr, bool read)
{
	//Halt if bus is in use
	if(I2C0->S & I2C_S_BUSY_MASK)
	{
		while(1);
	}
	I2C0->C1 |= I2C_C1_MST_MASK | I2C_C1_TX_MASK; //Start 
	I2C0-> D = read? (addr << 1) | 1 : (addr << 1); //Address
	//END START AND ADDRESS
	
	//Wait until start and slave address sends
	while (!(I2C0->S & I2C_S_IICIF_MASK));
}

static void iic_stopLl(void)
{
	//Issue stop command
	I2C0->C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);
	
	//Wait until bus is stopped
	while (I2C0->S & I2C_S_BUSY_MASK);
	
}


//Low level send routine for use in iic_transaction
static void iic_writeLl(uint8_t data)
{
	//Wait until ready
	while(!(I2C0->S & I2C_S_TCF_MASK));
	
	I2C0->S = I2C_S_IICIF_MASK; //Clear interrupt pending flag

	//Setup to send data
	I2C0->C1 |= I2C_C1_TX_MASK;
	
	//Send data
	I2C0-> D = data;
	
	//Wait until transfer is complete
	while(!(I2C0->S & I2C_S_IICIF_MASK));
	
	I2C0->S = I2C_S_IICIF_MASK; //Clear interrupt pending flag
	
	//Check for NAK or arbitration loss
	if(I2C0->S & I2C_S_ARBL_MASK || I2C0->S & I2C_S_RXAK_MASK)
	{
		dbg_puts("I2C Error\r\n");
		while(1);
	}

}

static uint8_t iic_receiveLl(void)
{
	//Wait until ready
	while(!(I2C0->S & I2C_S_TCF_MASK));
	
	//Clear interrupt pending flag
	I2C0->S = I2C_S_IICIF_MASK;
	
	//Setup device to receive data
	I2C0->C1 &= ~I2C_C1_TX_MASK;
	
	//Setup device not to send ACK, as we only one one byte
//	I2C0->C1 |= I2C_C1_TXAK_MASK;
	
	//Wait for transfer to complete
	while(!(I2C0->S & I2C_S_IICIF_MASK));
	
	//Clear interrupt pending flag
	I2C0->S = I2C_S_IICIF_MASK;
		
	return I2C0->D;
}
