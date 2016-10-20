

#include <MKL46Z4.H>
#include <stdint.h>
#include <stdbool.h>
#include "lcd.h"
#include "gpio.h"

typedef struct
{
	PORT_Type * port;
	uint8_t pin;
	uint8_t lcd_num; //Number in LCD registers
} pin_mapping_t;

//Mapping of backplane (phase) pins
static const pin_mapping_t backplanes[] = { {PORTD,0,40}, //COM0
                                            {PORTE,4,52}, //COM1
                                            {PORTB,23,19}, //COM2
                                            {PORTB,22,18}}; //COM3

static const int noBackplanes = sizeof(backplanes)/sizeof(pin_mapping_t);

//Mapping of frontplane pins
static const pin_mapping_t frontplanes[] = { {PORTC,17,37}, //1D/1E/1G/1F
                                             {PORTB,21,17}, //DP1/1E/1G/1F
                                             {PORTB,7,7}, //2D/2E/2G/2F
                                             {PORTB,8,8}, //DP2/2C/2B/2A
                                             {PORTE,5,53}, //3D/3D/3G/3F
                                             {PORTC,18,38}, //DP3/3C/3B/3A
                                             {PORTB,10,10}, //4D/4E/4G/4F
                                             {PORTB,11,11}}; //COL/4C/4B/4A

static const int noFrontplanes = sizeof(frontplanes)/sizeof(pin_mapping_t);
																	 
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
	static const digit_map_t digit_map[8] = { {0,0}, //a (physical d)
	                                          {0,1}, //b (physical e)
	                                          {0,3}, //c (physical f)
	                                          {1,3}, //d (physical a)
	                                          {1,2}, //e (physical b)
	                                          {1,1}, //f (physical c)
	                                          {0,2}, //g (physical g)
	                                          {1,0}}; //dp (physical dp)
#else
	static const digit_map_t digit_map[8] = { {1,3}, //a
	                                          {1,2}, //b
	                                          {1,1}, //c
	                                          {0,0}, //d
	                                          {0,1}, //e
	                                          {0,3}, //f
	                                          {0,2}, //g
	                                          {1,0}}; //dp
#endif


//Tables defining which segments are on for letters, numbers and symbols
//segment data is of the format [dp]gfedcba (a=lsb, dp=msb)							 
static const uint8_t lcdNumbers[10] = { BIN_TO_BYTE(0,0,1,1,1,1,1,1), // Zero
                                        BIN_TO_BYTE(0,0,0,0,0,1,1,0), // One
                                        BIN_TO_BYTE(0,1,0,1,1,0,1,1), // Two
                                        BIN_TO_BYTE(0,1,0,0,1,1,1,1), // Three
                                        BIN_TO_BYTE(0,1,1,0,0,1,1,0), // Four
                                        BIN_TO_BYTE(0,1,1,0,1,1,0,1), // Five
                                        BIN_TO_BYTE(0,1,1,1,1,1,0,1), // Six
                                        BIN_TO_BYTE(0,0,0,0,0,1,1,1), // Seven
                                        BIN_TO_BYTE(0,1,1,1,1,1,1,1), // Eight
                                        BIN_TO_BYTE(0,1,1,0,1,1,1,1)};// Nine

static const uint8_t lcdLetters[] = { BIN_TO_BYTE(0,1,1,1,0,1,1,1), // A
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
                                      BIN_TO_BYTE(0,1,1,0,1,1,1,0)}; // Z

#define LCD_SYM_SPACE 0
#define LCD_SYM_DASH 1
															 
static const uint8_t lcdSymbols[10] = { BIN_TO_BYTE(0,0,0,0,0,0,0,0), // Space
                                        BIN_TO_BYTE(0,1,0,0,0,0,0,0)}; // Dash
																 
void lcd_init(void)
{
	int i;
	
	//Enable clock to LCD
	SIM->SCGC5 |= SIM_SCGC5_SLCD_MASK;
	
	//Clear GCR to ensure LCD driver is off
	LCD->GCR = 0;
	
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
	
	//Set GCR
	LCD->GCR = LCD_GCR_DUTY(5) | LCD_GCR_LCLK(1) | LCD_GCR_SOURCE_MASK | LCD_GCR_LADJ(3) | LCD_GCR_CPSEL_MASK  | LCD_GCR_RVTRIM(8);
	
	//Start LCD
	LCD->GCR |= LCD_GCR_LCDEN_MASK;
}

//Set the segments of a digit
//This assumes all segments are off to start with
//segment data is of the format [dp]gfedcba (a=lsb, dp=msb)
void lcd_setDigit(uint8_t digit, uint8_t data)
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

//Display a string on the LCD
//Return 1 if the entire string was displayed
//Return 0 if not
uint8_t lcd_setStr(char *str)
{
	lcd_clear(); //Clear LCD
	
	int i;
	char c;
	uint8_t dispByte;
	for(i=0; i< NUM_DIGITS; i++)
	{
		c = str[i];
		//Return if at the end of the string
		if(c == '\0')
		{
			return 1;
		}
		
		//Get byte to display
		if(c >= 'a' && c <= 'z') //If lower case
		{
			dispByte = lcdLetters[c - 'a'];
		} else if(c >= 'A' && c <= 'Z') { //If upper case
			dispByte = lcdLetters[c - 'A'];
		} else if(c >= '0' && c <= '9') { //If number
			dispByte = lcdLetters[c - '0'];
		} else if(c == ' ') { //Special case for space
			dispByte = lcdSymbols[LCD_SYM_SPACE];
		} else if(c == '-') { //Special case for dash
			dispByte = lcdSymbols[LCD_SYM_DASH];
		} else { //Invalid/unsupported character
			while(1);
		}
		
		lcd_setDigit(LCD_DIGIT(i), dispByte);
	}
	
	//Check if the next character is null termination
	//I.e. the entire string is displayed
	if(c == '\0')
	{
		return 1;
	}
		
	return 0;
	
}

//Display a number on the LCD
//Range 0 to 10^(NUM_DIGITS) -1
void lcd_setNum(unsigned int num)
{

	//Note this routine uses integer division and multiplication -- likely slow!
	int i;
	int mult = SET_NUM_MULT; //This is 10**(NUM_DIGITS-1), but better to know at compile time
	uint8_t digit;
	bool digitWritten = false;
	
	//Clamp number if out of range
	if(num >= SET_NUM_MULT * 10)
	{
		num = (SET_NUM_MULT * 10) -1;
	}
	
	lcd_clear(); //Clear LCD
	
	for(i=0; i< NUM_DIGITS; i++)
	{
		digit = num/mult;
		num -= digit*mult;
		mult /= 10;
		
		//Display if the digit is not zero
		//Or if there is a preceding digit
		//Or if it's the last digit (as we want at least one zero)
		if(digit != 0 || digitWritten || i == NUM_DIGITS-1)
		{
			lcd_setDigit(LCD_DIGIT(i), lcdNumbers[digit]);
			digitWritten = true;
		}
	}
	
}

//Clear LCD
void lcd_clear(void)
{
	int i;
	
	//Clear all segments and backplanes
	for(i=0; i<16; i++)
	{
		LCD->WF[i] = 0;
	}
	
	
	//Re-enable backplanes
	for(i=0; i<noBackplanes; i++)
	{
		LCD->WF8B[backplanes[i].lcd_num] = (1 << i); //Set backplane 0 to be phase A, backplane 1 to be phase B etc.
	}
}
