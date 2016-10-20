

#include <MKL46Z4.H>
#include <stdint.h>
#include "lcd.h"
#include "gpio.h"

typedef struct
{
	PORT_Type * port;
	uint8_t pin;
	uint8_t lcd_num; //Number in LCD registers
} pin_mapping_t;

//Mapping of backplane (phase) pins
const pin_mapping_t backplanes[] = { {PORTD,0,40}, //COM0
                                 {PORTE,4,52}, //COM1
																 {PORTB,23,19}, //COM2
																 {PORTB,22,18}}; //COM3

const int noBackplanes = sizeof(backplanes)/sizeof(pin_mapping_t);

//Mapping of frontplane pins
const pin_mapping_t frontplanes[] = { {PORTC,17,37}, //1D/1E/1G/1F
                                   {PORTB,21,17}, //DP1/1E/1G/1F
																	 {PORTB,7,7}, //2D/2E/2G/2F
																	 {PORTB,8,8}, //DP2/2C/2B/2A
																	 {PORTE,5,53}, //3D/3D/3G/3F
																	 {PORTC,18,38}, //DP3/3C/3B/3A
																	 {PORTB,10,10}, //4D/4E/4G/4F
																	 {PORTB,11,11}}; //COL/4C/4B/4A

const int noFrontplanes = sizeof(frontplanes)/sizeof(pin_mapping_t);
																	 
//Mapping between frontplanes/phases and digits
//Note this is for all four digits due to careful layout of the frontplanes array
//i.e. digit0 is frontplanes[0] and frontplanes[1]
//i.e. digit1 is frontplanes[2] and frontplanes[3] etc.
typedef struct
{
	uint8_t fplne_ofst; //0 or 1 depending on which of the frontplane pairs to use
	uint8_t phase; //0-4 to select phase
} digit_map_t;

//Format is digit_map_t[0] = a, digit_map_t[1] = b ... digit_map_t[8] = dp
//digit3 has a colon instead of dp

//Note datasheet shows the order of digits incorrectly (anticlockwise rather than clockwise)
#ifdef LCD_UPSIDE_DOWN
	const digit_map_t digit_map[8] = { {0,0}, //a (physical d)
																		 {0,1}, //b (physical e)
																		 {0,3}, //c (physical f)
																		 {1,3}, //d (physical a)
																		 {1,2}, //e (physical b)
																		 {1,1}, //f (physical c)
																		 {0,2}, //g (physical g)
																		 {1,0}}; //dp (physical dp)
#else
	const digit_map_t digit_map[8] = { {1,3}, //a
																		 {1,2}, //b
																		 {1,1}, //c
																		 {0,0}, //d
																		 {0,1}, //e
																		 {0,3}, //f
																		 {0,2}, //g
																		 {1,0}}; //dp
#endif



																	 
const uint8_t lcdNums[10] = { BIN_TO_BYTE(0,0,1,1,1,1,1,1), // Zero
                              BIN_TO_BYTE(0,0,0,0,0,1,1,0), // One
                              BIN_TO_BYTE(0,1,0,1,1,0,1,1), // Two
                              BIN_TO_BYTE(0,1,0,0,1,1,1,1), // Three
                              BIN_TO_BYTE(0,1,1,0,0,1,1,0), // Four
                              BIN_TO_BYTE(0,1,1,0,1,1,0,1), // Five
                              BIN_TO_BYTE(0,1,1,1,1,1,0,1), // Six
                              BIN_TO_BYTE(0,0,0,0,0,1,1,1), // Seven
                              BIN_TO_BYTE(0,1,1,1,1,1,1,1), // Eight
                              BIN_TO_BYTE(0,1,1,0,1,1,1,1)};// Nine

const uint8_t lcdLetters[] = { BIN_TO_BYTE(0,1,1,1,0,1,1,1), // A
                               BIN_TO_BYTE(0,1,1,1,1,1,0,0), // B
                               BIN_TO_BYTE(0,0,1,1,1,0,0,1), // C
                               BIN_TO_BYTE(0,1,0,1,1,1,1,0), // D
                               BIN_TO_BYTE(0,1,1,1,1,0,0,1), // E
                               BIN_TO_BYTE(0,1,1,1,0,0,0,1), // F
                               BIN_TO_BYTE(0,1,1,0,1,1,1,1), // G
                               BIN_TO_BYTE(0,1,1,1,0,1,1,0), // H
                               BIN_TO_BYTE(0,0,1,1,0,0,0,0), // I
                               BIN_TO_BYTE(0,0,0,1,1,1,1,0), // J
                               BIN_TO_BYTE(0,1,1,1,0,1,1,0), // K
                               BIN_TO_BYTE(0,0,1,1,1,0,0,0), // L
                               BIN_TO_BYTE(0,0,0,1,0,1,0,1), // M
                               BIN_TO_BYTE(0,1,0,1,0,1,0,0), // N
                               BIN_TO_BYTE(0,0,1,1,1,1,1,1), // O
                               BIN_TO_BYTE(0,1,1,1,0,0,1,1), // P
                               BIN_TO_BYTE(0,1,1,0,0,1,1,1), // Q
                               BIN_TO_BYTE(0,1,0,1,0,0,0,0), // R
                               BIN_TO_BYTE(0,1,1,0,1,1,0,1), // S
                               BIN_TO_BYTE(0,1,1,1,1,0,0,0), // T
                               BIN_TO_BYTE(0,0,1,1,1,1,1,0), // U
                               BIN_TO_BYTE(0,0,0,1,1,1,0,0), // V
                               BIN_TO_BYTE(0,0,1,0,1,0,1,0), // W
                               BIN_TO_BYTE(0,1,1,1,0,1,1,0), // X
                               BIN_TO_BYTE(0,1,0,1,1,0,1,1), // Y
                               BIN_TO_BYTE(0,1,1,0,1,1,1,0), // Z
                               BIN_TO_BYTE(0,0,0,0,0,0,0,0)}; // SPACE
																 
void lcd_init(void)
{
	int i;
	
	//Enable clock to LCD
	SIM->SCGC5 |= SIM_SCGC5_SLCD_MASK;
	
	//Mask to set BPEN
	//Each pin used as a backplane is a high bit in the mask
	uint64_t backplaneEnableMask = 0;
	
	//Setup backplane pins
	for(i=0; i<noBackplanes; i++)
	{
		init_pin(backplanes[i].port, backplanes[i].pin, 0, 0, 0); //Initialise pin
		backplaneEnableMask |= ( ((uint64_t)1) << backplanes[i].lcd_num); //Activate bit in backplane mask
		LCD->WF8B[backplanes[i].lcd_num] = (1 << i); //Set backplane 0 to be phase A, backplane 1 to be phase B etc.
	}

	//Mask to set PEN
	//Each used pin (segment or phase) is a high bit in the mask
	uint64_t pinEnableMask = backplaneEnableMask;
	
	//Setup frontplane pins
	for(i=0; i<noFrontplanes; i++)
	{
		init_pin(frontplanes[i].port, frontplanes[i].pin, 0, 0, 0); //Initialise pin
		pinEnableMask |= ( ((uint64_t)1) << frontplanes[i].lcd_num); //Activate bit in pin mask
	}
	
	//Pin enable
	LCD->PEN[0] = (uint32_t) (pinEnableMask & 0xFFFFFFFF);
	LCD->PEN[1] = (uint32_t) ( (pinEnableMask >> 32) & 0xFFFFFFFF);

	//Backplane enable
	LCD->BPEN[0] = (uint32_t) (backplaneEnableMask & 0xFFFFFFFF);
	LCD->BPEN[1] = (uint32_t) ( (backplaneEnableMask >> 32) & 0xFFFFFFFF);
	
	LCD->GCR = 0x8B000CD;
	

	for(i=0;i<4;i++)
	{
		setDigit(LCD_DIGIT(i), lcdLetters[i]);
	}
	

	
}

//Set the segments of a digit
//This assumes all segments are off to start with
//segment data is of the format [dp]gfedcba (a=lsb, dp=msb)
void setDigit(uint8_t digit, uint8_t data)
{
	int i;
	digit_map_t mapping;
	for(i=0; i<8; i++)
	{
		//If we should set the segment
		if(data & (1 << i))
		{
			mapping = digit_map[i];
			LCD->WF8B[ frontplanes[mapping.fplne_ofst + (digit*2)].lcd_num] |= 1 << mapping.phase;
		}
	}
}
