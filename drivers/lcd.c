
#include <stdint.h>
#include "lcd.h"
#include "LPC17xx.h"
#include "delay.h"

#define LCD_ENABLE_LOW 	LPC_GPIO0->FIOCLR |= (1 << LCD_EN)
#define LCD_ENABLE_HIGH LPC_GPIO0->FIOSET |= (1 << LCD_EN)
#define LCD_RS_LOW 		LPC_GPIO0->FIOCLR |= (1 << LCD_RS)
#define LCD_RS_HIGH 	LPC_GPIO0->FIOSET |= (1 << LCD_RS)

void lcd_write_4(uint8_t theByte)
{

	LPC_GPIO0->FIOCLR |= (1 << LCD_D7);	
	if(theByte & 1 << 7)
		LPC_GPIO0->FIOSET |= (1 << LCD_D7);

	LPC_GPIO0->FIOCLR |= (1 << LCD_D6);
	if(theByte & 1 << 6)
		LPC_GPIO0->FIOSET |= (1 << LCD_D6);

	LPC_GPIO0->FIOCLR |= (1 << LCD_D5);
	if(theByte & 1 << 5)
		LPC_GPIO0->FIOSET |= (1 << LCD_D5);

	LPC_GPIO0->FIOCLR |= (1 << LCD_D4);
	if(theByte & 1 << 4)
		LPC_GPIO0->FIOSET |= (1 << LCD_D4);

	LCD_ENABLE_HIGH;
	_delay_us(2);
	LCD_ENABLE_LOW;
	_delay_us(2);
}

void lcd_write_instruction_4d(uint8_t theInstruction)
{
	LCD_RS_LOW;
	LCD_ENABLE_LOW;
	lcd_write_4(theInstruction);
	lcd_write_4(theInstruction << 4);
}

void lcd_write_character_4d(uint8_t theData)
{
	LCD_RS_HIGH;
	LCD_ENABLE_LOW;
	lcd_write_4(theData);
	lcd_write_4(theData << 4);
}

void lcd_init(void)
{
	LPC_GPIO0->FIODIR |= (1 << LCD_RS);
	LPC_GPIO0->FIODIR |= (1 << LCD_EN);
	LPC_GPIO0->FIODIR |= (1 << LCD_D4);
	LPC_GPIO0->FIODIR |= (1 << LCD_D5);
	LPC_GPIO0->FIODIR |= (1 << LCD_D6);
	LPC_GPIO0->FIODIR |= (1 << LCD_D7);

	// Power up delay
	_delay_ms(100);

	// Low RS
	LCD_RS_LOW;

	// Low Enable
	LCD_ENABLE_LOW;

	lcd_write_4(lcd_FunctionReset);
	_delay_ms(10);

	lcd_write_4(lcd_FunctionReset);
	_delay_us(200);

	lcd_write_4(lcd_FunctionReset);
	_delay_us(200);

	lcd_write_4(lcd_FunctionSet4bit);
	_delay_us(80);

	lcd_write_instruction_4d(lcd_DisplayOff);
	_delay_us(80);

	lcd_write_instruction_4d(lcd_Clear);
	_delay_ms(4);

	lcd_write_instruction_4d(lcd_EntryMode);
	_delay_us(80);

	lcd_write_instruction_4d(lcd_DisplayOn);
	_delay_us(80);
}

void lcd_print(uint8_t * buff)
{
	for(;* buff != '\0';lcd_write_character_4d(* buff++))
	{
		_delay_us(80);
	}
}