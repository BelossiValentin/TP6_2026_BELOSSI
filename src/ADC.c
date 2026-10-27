#include "ADC.h"

#define ADC_CANTIDAD_MUESTRAS  16U

void initADC(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	/* PB0=ADC1_IN8 y PB1=ADC1_IN9. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1U;
	ADC_Init(ADC1, &ADC_InitStructure);

	/* Convierte ambos canales en una secuencia. */
	ADC_InjectedSequencerLengthConfig(ADC1, 2U);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_8, 1U,
						  ADC_SampleTime_144Cycles);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_9, 2U,
						  ADC_SampleTime_144Cycles);
	ADC_ExternalTrigInjectedConvEdgeConfig(
		ADC1, ADC_ExternalTrigInjecConvEdge_None);
	ADC_Cmd(ADC1, ENABLE);
}

void readADCs(uint16_t* ADC_1, uint16_t* ADC_2)
{
	if ((ADC_1 == 0) || (ADC_2 == 0))
	{
		return;
	}

	ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
	ADC_SoftwareStartInjectedConv(ADC1);
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC) == RESET)
	{
	}

	*ADC_1 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
	*ADC_2 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2);
}

void readADCsFiltrados(uint16_t* ADC_1, uint16_t* ADC_2)
{
	uint32_t acumulador_1 = 0U;
	uint32_t acumulador_2 = 0U;
	uint16_t muestra_1;
	uint16_t muestra_2;
	uint8_t i;

	if ((ADC_1 == 0) || (ADC_2 == 0))
	{
		return;
	}

	/* Promedia 16 muestras para reducir ruido. */
	for (i = 0U; i < ADC_CANTIDAD_MUESTRAS; i++)
	{
		readADCs(&muestra_1, &muestra_2);
		acumulador_1 += muestra_1;
		acumulador_2 += muestra_2;
	}

	*ADC_1 = (uint16_t)((acumulador_1 +
		(ADC_CANTIDAD_MUESTRAS / 2U)) / ADC_CANTIDAD_MUESTRAS);
	*ADC_2 = (uint16_t)((acumulador_2 +
		(ADC_CANTIDAD_MUESTRAS / 2U)) / ADC_CANTIDAD_MUESTRAS);
}

