#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

void LCD_init(void);
void LCD_clrscr(void);
void LCD_gotoxy(uint8_t x, uint8_t y);
void LCD_putc(char caracter);
void LCD_display_on(void);
void LCD_cursor_off(void);
void LCD_blink_off(void);

#endif /* LCD_H_ */

