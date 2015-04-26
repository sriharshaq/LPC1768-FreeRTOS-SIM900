
#ifndef __lcd_h__
#define __lcd_h__

#define LCD_RS 10
#define LCD_EN 11
#define LCD_D4 19
#define LCD_D5 20
#define LCD_D6 21
#define LCD_D7 22

#define EN_TRIGGER_DELAY 20

// LCD instructions
#define lcd_Clear           0b00000001          // replace all characters with ASCII 'space'
#define lcd_Home            0b00000010          // return cursor to first position on first line
#define lcd_EntryMode       0b00000110          // shift cursor from left to right on read/write
#define lcd_DisplayOff      0b00001000          // turn display off
#define lcd_DisplayOn       0b00001100          // display on, cursor off, don't blink character
#define lcd_FunctionReset   0b00110000          // reset the LCD
#define lcd_FunctionSet4bit 0b00101000          // 4-bit data, 2-line display, 5 x 7 font
#define lcd_SetCursor       0b10000000          // set cursor position

#define lcdClear() lcd_write_instruction_4d(lcd_Clear);

extern void lcd_init(void);
extern void lcd_write_4(uint8_t);
extern void lcd_write_instruction_4d(uint8_t);
extern void lcd_write_character_4d(uint8_t);
extern void lcd_print(uint8_t *);

#endif