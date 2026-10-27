#include "Dac.h"

#include "stm32f4xx.h"

#define DAC_MAX_COUNTS  4095UL

static uint16_t dac_millivolts;

void Dac_Init(void)
{
	GPIO_InitTypeDef gpio;
	DAC_InitTypeDef dac;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	/* PA5 corresponde a DAC_OUT2 en el STM32F407. */
	gpio.GPIO_Pin = GPIO_Pin_5;
	gpio.GPIO_Mode = GPIO_Mode_AN;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &gpio);

	dac.DAC_Trigger = DAC_Trigger_None;
	dac.DAC_WaveGeneration = DAC_WaveGeneration_None;
	dac.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
	dac.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DAC_Init(DAC_Channel_2, &dac);
	DAC_Cmd(DAC_Channel_2, ENABLE);

	dac_millivolts = 0U;
	DAC_SetChannel2Data(DAC_Align_12b_R, 0U);
}

uint8_t Dac_SetMillivolts(uint16_t millivolts)
{
	uint32_t counts;

	if (millivolts > DAC_REFERENCE_MV)
	{
		return 0U;
	}

	/* El medio divisor aplica redondeo al valor de 12 bits mas cercano. */
	counts = (((uint32_t)millivolts * DAC_MAX_COUNTS) +
		(DAC_REFERENCE_MV / 2U)) / DAC_REFERENCE_MV;
	Dac_SetCounts((uint16_t)counts);
	dac_millivolts = millivolts;

	return 1U;
}

void Dac_SetCounts(uint16_t counts)
{
	if (counts > DAC_MAX_COUNTS)
	{
		counts = (uint16_t)DAC_MAX_COUNTS;
	}

	DAC_SetChannel2Data(DAC_Align_12b_R, counts);
	dac_millivolts = (uint16_t)((((uint32_t)counts * DAC_REFERENCE_MV) +
		(DAC_MAX_COUNTS / 2U)) / DAC_MAX_COUNTS);
}

uint16_t Dac_GetMillivolts(void)
{
	return dac_millivolts;
}
