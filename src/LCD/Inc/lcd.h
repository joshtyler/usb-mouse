//Driver for LCD on KL46Z

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#define LCD_UPSIDE_DOWN
#define NUM_DIGITS 4
#define SET_NUM_MULT 1000 //This is 10**(NUM_DIGITS-1), but better to know at compile time

#ifdef LCD_UPSIDE_DOWN
	#define LCD_DIGIT(n) (NUM_DIGITS -1)-n
#else
	#define LCD_DIGIT(n) n
#endif

//Needed for binary constants
//From http://www.keil.com/support/docs/1156.htm
#define BIN_TO_BYTE(b7,b6,b5,b4,b3,b2,b1,b0) ((b7 << 7)+(b6 << 6)+(b5 << 5)+(b4 << 4)+(b3 << 3)+(b2 << 2)+(b1 << 1)+b0)

void lcd_init(void);
void lcd_setDigit(uint8_t digit, uint8_t data);
uint8_t lcd_setStr(const char *str);
void lcd_setNum(unsigned int num);
void lcd_clear(void);

#endif
