#include "App.h"

#include "ADC.h"
#include "Control.h"
#include "Dac.h"
#include "LCD.h"
#include "Menu.h"
#include "PWM.h"
#include "Rf.h"
#include "Serial.h"
#include "Teclado.h"
#include "TerminalMenu.h"
#include "global.h"
#include "stm32f4xx.h"

static uint8_t app_backlight_enabled = 1U;

static void App_GpioInit(void)
{
	GPIO_InitTypeDef gpio;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	gpio.GPIO_Pin = GPIO_Pin_8;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &gpio);
	GPIO_ResetBits(GPIOD, GPIO_Pin_8);
}

static void App_SetBacklight(uint8_t enabled)
{
	app_backlight_enabled = enabled;
	if (enabled != 0U)
	{
		GPIO_SetBits(GPIOD, GPIO_Pin_8);
	}
	else
	{
		GPIO_ResetBits(GPIOD, GPIO_Pin_8);
	}
}

static void App_UpdateBacklight(char key)
{
	if (key == '#')
	{
		App_SetBacklight(1U);
	}
	else if (key == '*')
	{
		App_SetBacklight(0U);
	}
}

void App_Init(void)
{
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000U);

	App_GpioInit();
	App_SetBacklight(1U);
	Teclado_init();
	initADC();
	Dac_Init();
	PWM_Init(1000U);
	Rf_Init();
	Control_init();
	Serial_Init(SERIAL_DEFAULT_BAUDRATE);

	delay_ms(100U);
	LCD_init();
	LCD_clrscr();
	LCD_display_on();
	LCD_cursor_off();
	LCD_blink_off();
	Menu_init();
	TerminalMenu_Init();
}

void App_Process(void)
{
	char keypad_key;
	char remote_key;
	char input;

	/* La decodificacion RF se realiza fuera de EXTI1. */
	Rf_Process();
	keypad_key = Teclado_getKey();
	remote_key = Control_getKey();

	if ((keypad_key != TECLADO_SIN_TECLA) &&
		(remote_key != CONTROL_SIN_TECLA))
	{
		input = TECLADO_SIN_TECLA;
	}
	else if (keypad_key != TECLADO_SIN_TECLA)
	{
		input = keypad_key;
	}
	else
	{
		input = remote_key;
	}

	App_UpdateBacklight(input);
	Menu_FSM(input, app_backlight_enabled);
	TerminalMenu_Process();
}
