#include "global.h"

static volatile uint32_t systick_ms = 0U;

/* Incrementa el tiempo global cada 1 ms. */
void SysTick_Handler(void)
{
	systick_ms++;
}

void delay_us(uint32_t us)
{
	volatile uint32_t i;
	uint32_t ciclos;

	ciclos = (SystemCoreClock / 3000000U) * us;

	for (i = 0U; i < ciclos; i++)
	{
		__NOP();
	}
}

void delay_ms(uint16_t ms)
{
	uint32_t inicio;

	inicio = systick_ms;
	while ((uint32_t)(systick_ms - inicio) < (uint32_t)ms)
	{
		/* Espera no bloqueante para las interrupciones. */
	}
}

uint32_t returnSystick(void)
{
	return systick_ms;
}

