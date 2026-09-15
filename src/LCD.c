#include "LCD.h"

#include "global.h"
#include "stm32f4xx.h"

#define LCD_GPIO_RS  GPIOE
#define LCD_PIN_RS   GPIO_Pin_1
#define LCD_GPIO_RW  GPIOE
#define LCD_PIN_RW   GPIO_Pin_2
#define LCD_GPIO_EN  GPIOE
#define LCD_PIN_EN   GPIO_Pin_3

#define LCD_GPIO_D4  GPIOE
#define LCD_PIN_D4   GPIO_Pin_4
#define LCD_GPIO_D5  GPIOE
#define LCD_PIN_D5   GPIO_Pin_5
#define LCD_GPIO_D6  GPIOE
#define LCD_PIN_D6   GPIO_Pin_6
#define LCD_GPIO_D7  GPIOC
#define LCD_PIN_D7   GPIO_Pin_13

#define LCD_CLEAR_DISPLAY  0x01U
#define LCD_ENTRY_MODE     0x06U
#define LCD_DISPLAY_CTRL   0x08U
#define LCD_FUNCTION_SET   0x28U
#define LCD_SET_DDRAM      0x80U

#define LCD_DISPLAY_BIT    0x04U
#define LCD_CURSOR_BIT     0x02U
#define LCD_BLINK_BIT      0x01U

static uint8_t display_control = LCD_DISPLAY_BIT;

static void LCD_pulsoEnable(void)
{
	GPIO_SetBits(LCD_GPIO_EN, LCD_PIN_EN);
	delay_us(50U);
	GPIO_ResetBits(LCD_GPIO_EN, LCD_PIN_EN);
	delay_us(50U);
}

static void LCD_escribirNibble(uint8_t nibble)
{
	/* Envia cuatro bits al LCD. */
	GPIO_WriteBit(LCD_GPIO_D4, LCD_PIN_D4,
				  ((nibble & 0x01U) != 0U) ? Bit_SET : Bit_RESET);
	GPIO_WriteBit(LCD_GPIO_D5, LCD_PIN_D5,
				  ((nibble & 0x02U) != 0U) ? Bit_SET : Bit_RESET);
	GPIO_WriteBit(LCD_GPIO_D6, LCD_PIN_D6,
				  ((nibble & 0x04U) != 0U) ? Bit_SET : Bit_RESET);
	GPIO_WriteBit(LCD_GPIO_D7, LCD_PIN_D7,
				  ((nibble & 0x08U) != 0U) ? Bit_SET : Bit_RESET);
	LCD_pulsoEnable();
}

static void LCD_escribir(uint8_t dato, uint8_t es_dato)
{
	GPIO_ResetBits(LCD_GPIO_RW, LCD_PIN_RW);
	GPIO_WriteBit(LCD_GPIO_RS, LCD_PIN_RS,
				  (es_dato != 0U) ? Bit_SET : Bit_RESET);
	LCD_escribirNibble((uint8_t)(dato >> 4U));
	LCD_escribirNibble((uint8_t)(dato & 0x0FU));
}

static void LCD_comando(uint8_t comando)
{
	LCD_escribir(comando, 0U);

	if (comando == LCD_CLEAR_DISPLAY)
	{
		delay_ms(3U);
	}
	else
	{
		delay_us(80U);
	}
}

void LCD_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC |
						   RCC_AHB1Periph_GPIOE,
						   ENABLE);

	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Pin = LCD_PIN_RS | LCD_PIN_RW | LCD_PIN_EN |
							  LCD_PIN_D4 | LCD_PIN_D5 | LCD_PIN_D6;
	GPIO_Init(GPIOE, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = LCD_PIN_D7;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_ResetBits(GPIOE, LCD_PIN_RS | LCD_PIN_RW | LCD_PIN_EN |
						  LCD_PIN_D4 | LCD_PIN_D5 | LCD_PIN_D6);
	GPIO_ResetBits(GPIOC, LCD_PIN_D7);
	delay_ms(50U);

	GPIO_ResetBits(LCD_GPIO_RS, LCD_PIN_RS);
	/* Secuencia de inicio en modo de 4 bits. */
	LCD_escribirNibble(0x03U);
	delay_ms(6U);
	LCD_escribirNibble(0x03U);
	delay_ms(6U);
	LCD_escribirNibble(0x03U);
	delay_ms(1U);
	LCD_escribirNibble(0x02U);
	delay_ms(1U);

	display_control = LCD_DISPLAY_BIT;
	LCD_comando(LCD_FUNCTION_SET);
	LCD_comando((uint8_t)(LCD_DISPLAY_CTRL | display_control));
	LCD_comando(LCD_CLEAR_DISPLAY);
	LCD_comando(LCD_ENTRY_MODE);
}

void LCD_clrscr(void)
{
	LCD_comando(LCD_CLEAR_DISPLAY);
}

void LCD_gotoxy(uint8_t x, uint8_t y)
{
	uint8_t direccion;

	direccion = (y == 0U) ? x : (uint8_t)(0x40U + x);
	LCD_comando((uint8_t)(LCD_SET_DDRAM | direccion));
}

void LCD_putc(char caracter)
{
	LCD_escribir((uint8_t)caracter, 1U);
	delay_us(80U);
}

void LCD_display_on(void)
{
	display_control |= LCD_DISPLAY_BIT;
	LCD_comando((uint8_t)(LCD_DISPLAY_CTRL | display_control));
}

void LCD_cursor_off(void)
{
	display_control &= (uint8_t)~LCD_CURSOR_BIT;
	LCD_comando((uint8_t)(LCD_DISPLAY_CTRL | display_control));
}

void LCD_blink_off(void)
{
	display_control &= (uint8_t)~LCD_BLINK_BIT;
	LCD_comando((uint8_t)(LCD_DISPLAY_CTRL | display_control));
}

