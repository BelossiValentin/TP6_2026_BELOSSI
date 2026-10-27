#include "PWM.h"

#define PWM_TIMER_CLOCK_HZ  (SystemCoreClock / 2U)
#define PWM_TICK_HZ         1000000U
#define PWM_MAX_PERIOD      0xFFFFU

static uint32_t pwm_periodo = 999U;
static uint8_t pwm_duty = 0U;

void PWM_Init(uint32_t frecuencia_hz)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	TIM_TimeBaseInitTypeDef TIM_BaseStruct;
	TIM_OCInitTypeDef TIM_OCStruct;
	uint32_t prescaler;

	if (frecuencia_hz == 0U)
	{
		frecuencia_hz = 1000U;
	}
	else if (frecuencia_hz > PWM_TICK_HZ)
	{
		frecuencia_hz = PWM_TICK_HZ;
	}

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12, ENABLE);

	/* PB14 corresponde a TIM12_CH1 y a la entrada PWM del cooler. */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM12);

	/* Ajusta TIM12 a una cuenta de 1 us. */
	prescaler = (PWM_TIMER_CLOCK_HZ / PWM_TICK_HZ) - 1U;
	pwm_periodo = (PWM_TICK_HZ / frecuencia_hz) - 1U;
	/* TIM12 es de 16 bits. */
	if (pwm_periodo > PWM_MAX_PERIOD)
	{
		pwm_periodo = PWM_MAX_PERIOD;
	}

	TIM_BaseStruct.TIM_Period = pwm_periodo;
	TIM_BaseStruct.TIM_Prescaler = (uint16_t)prescaler;
	TIM_BaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_BaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM12, &TIM_BaseStruct);

	TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCStruct.TIM_Pulse = 0U;
	/* Q2 invierte la senal antes de entregarla al cooler. */
	TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC1Init(TIM12, &TIM_OCStruct);
	TIM_OC1PreloadConfig(TIM12, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM12, ENABLE);
	TIM_Cmd(TIM12, ENABLE);

	PWM_SetDuty(0U);
}

void PWM_SetDuty(uint8_t duty)
{
	uint32_t pulso;

	if (duty > 100U)
	{
		duty = 100U;
	}

	/* CCR1 fija el ciclo de trabajo. */
	pwm_duty = duty;
	pulso = ((uint32_t)duty * (pwm_periodo + 1U)) / 100U;
	TIM_SetCompare1(TIM12, pulso);
}

uint8_t PWM_GetDuty(void)
{
	return pwm_duty;
}

