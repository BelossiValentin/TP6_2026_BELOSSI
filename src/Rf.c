#include "Rf.h"

#include "stm32f4xx.h"

#define RF_GPIO                    GPIOD
#define RF_PIN                     GPIO_Pin_1
#define RF_EXTI_LINE               EXTI_Line1

#define RF_TIMER_TICK_HZ           1000000U
#define RF_FRAME_BITS              24U
#define RF_SYNC_LOW_MIN_US         8000U
#define RF_SYNC_LOW_MAX_US         16000U
#define RF_SHORT_LOW_MIN_US         150U
#define RF_SHORT_LOW_MAX_US         650U
#define RF_LONG_LOW_MIN_US          700U
#define RF_LONG_LOW_MAX_US         1600U
#define RF_QUEUE_LENGTH             256U
#define RF_QUEUE_MASK       (RF_QUEUE_LENGTH - 1U)

volatile uint32_t rf_edge_count;
volatile uint32_t rf_last_code;
volatile uint8_t rf_last_bits;
volatile uint32_t rf_dropped_pulses;
volatile uint32_t rf_last_low_us;
volatile uint32_t rf_valid_frame_count;
volatile uint32_t rf_timer_clock_hz;

static volatile uint16_t rf_pulse_queue[RF_QUEUE_LENGTH];
static volatile uint8_t rf_queue_head;
static volatile uint8_t rf_queue_tail;
static volatile uint32_t rf_falling_time;
static volatile uint8_t rf_have_falling;

static uint32_t rf_frame;
static uint8_t rf_bit_count;
static uint8_t rf_receiving;
static uint8_t rf_has_code;
static uint32_t rf_frame_code;
static uint8_t rf_frame_ready;

static void Rf_ResetDecoder(void)
{
	rf_frame = 0U;
	rf_bit_count = 0U;
	rf_receiving = 0U;
}

static uint8_t Rf_DecodeBit(uint16_t low_time, uint8_t* bit)
{
	if ((low_time >= RF_SHORT_LOW_MIN_US) &&
		(low_time <= RF_SHORT_LOW_MAX_US))
	{
		*bit = 1U;
		return 1U;
	}

	if ((low_time >= RF_LONG_LOW_MIN_US) &&
		(low_time <= RF_LONG_LOW_MAX_US))
	{
		*bit = 0U;
		return 1U;
	}

	return 0U;
}

static void Rf_ProcessPulse(uint16_t low_time)
{
	uint8_t bit;

	if ((low_time >= RF_SYNC_LOW_MIN_US) &&
		(low_time <= RF_SYNC_LOW_MAX_US))
	{
		rf_frame = 0U;
		rf_bit_count = 0U;
		rf_receiving = 1U;
		return;
	}

	if (rf_receiving == 0U)
	{
		return;
	}

	if (Rf_DecodeBit(low_time, &bit) == 0U)
	{
		Rf_ResetDecoder();
		return;
	}

	rf_frame = (rf_frame << 1U) | bit;
	rf_bit_count++;
	if (rf_bit_count == RF_FRAME_BITS)
	{
		rf_last_code = rf_frame & 0x00FFFFFFUL;
		rf_last_bits = RF_FRAME_BITS;
		rf_has_code = 1U;
		rf_frame_code = rf_last_code;
		rf_frame_ready = 1U;
		rf_valid_frame_count++;
		Rf_ResetDecoder();
	}
}

void Rf_Init(void)
{
	GPIO_InitTypeDef gpio;
	TIM_TimeBaseInitTypeDef timer;
	EXTI_InitTypeDef exti;
	NVIC_InitTypeDef nvic;
	RCC_ClocksTypeDef clocks;
	uint32_t prescaler;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* PD1 recibe la salida DATA del receptor RF. */
	gpio.GPIO_Pin = RF_PIN;
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(RF_GPIO, &gpio);

	/* Los timers APB reciben el doble de PCLK cuando su prescaler no es 1. */
	RCC_GetClocksFreq(&clocks);
	rf_timer_clock_hz = clocks.PCLK1_Frequency;
	if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
	{
		rf_timer_clock_hz *= 2U;
	}
	prescaler = (rf_timer_clock_hz / RF_TIMER_TICK_HZ) - 1U;
	timer.TIM_Period = 0xFFFFFFFFUL;
	timer.TIM_Prescaler = (uint16_t)prescaler;
	timer.TIM_ClockDivision = TIM_CKD_DIV1;
	timer.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &timer);
	TIM_SetCounter(TIM2, 0U);
	TIM_Cmd(TIM2, ENABLE);

	rf_edge_count = 0U;
	rf_last_code = 0U;
	rf_last_bits = 0U;
	rf_dropped_pulses = 0U;
	rf_last_low_us = 0U;
	rf_valid_frame_count = 0U;
	rf_queue_head = 0U;
	rf_queue_tail = 0U;
	rf_have_falling = 0U;
	rf_has_code = 0U;
	rf_frame_code = 0U;
	rf_frame_ready = 0U;
	Rf_ResetDecoder();

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource1);
	EXTI_ClearITPendingBit(RF_EXTI_LINE);
	exti.EXTI_Line = RF_EXTI_LINE;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	exti.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvic.NVIC_IRQChannel = EXTI1_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 1U;
	nvic.NVIC_IRQChannelSubPriority = 0U;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
}

void Rf_Process(void)
{
	uint16_t low_time;
	uint8_t tail;

	while (rf_queue_tail != rf_queue_head)
	{
		tail = rf_queue_tail;
		low_time = rf_pulse_queue[tail];
		rf_queue_tail = (uint8_t)((tail + 1U) & RF_QUEUE_MASK);
		Rf_ProcessPulse(low_time);
	}
}

uint8_t Rf_GetLastCode(uint32_t* code)
{
	if ((code == 0) || (rf_has_code == 0U))
	{
		return 0U;
	}

	*code = rf_last_code;
	return 1U;
}

uint32_t Rf_ReadLastCode(void)
{
	return (rf_has_code != 0U) ? rf_last_code : 0U;
}

uint8_t Rf_GetFrame(uint32_t* code)
{
	if ((code == 0) || (rf_frame_ready == 0U))
	{
		return 0U;
	}

	*code = rf_frame_code;
	rf_frame_ready = 0U;
	return 1U;
}

void Rf_Flush(void)
{
	NVIC_DisableIRQ(EXTI1_IRQn);
	rf_queue_tail = rf_queue_head;
	rf_have_falling = 0U;
	NVIC_EnableIRQ(EXTI1_IRQn);

	rf_frame_ready = 0U;
	Rf_ResetDecoder();
}

uint8_t Rf_GetButton(uint32_t code)
{
	code &= 0x00FFFFFFUL;
	if (code == RF_CODE_A)
	{
		return RF_BUTTON_A;
	}
	if (code == RF_CODE_B)
	{
		return RF_BUTTON_B;
	}
	if (code == RF_CODE_C)
	{
		return RF_BUTTON_C;
	}
	if (code == RF_CODE_D)
	{
		return RF_BUTTON_D;
	}

	return RF_BUTTON_NONE;
}

void EXTI1_IRQHandler(void)
{
	uint32_t current_time;
	uint32_t low_time;
	uint8_t next_head;

	if (EXTI_GetITStatus(RF_EXTI_LINE) == RESET)
	{
		return;
	}

	current_time = TIM_GetCounter(TIM2);
	rf_edge_count++;
	if (GPIO_ReadInputDataBit(RF_GPIO, RF_PIN) == Bit_RESET)
	{
		rf_falling_time = current_time;
		rf_have_falling = 1U;
	}
	else if (rf_have_falling != 0U)
	{
		low_time = current_time - rf_falling_time;
		rf_have_falling = 0U;
		rf_last_low_us = low_time;
		next_head = (uint8_t)((rf_queue_head + 1U) & RF_QUEUE_MASK);
		if ((next_head != rf_queue_tail) && (low_time <= 0xFFFFUL))
		{
			rf_pulse_queue[rf_queue_head] = (uint16_t)low_time;
			rf_queue_head = next_head;
		}
		else
		{
			rf_dropped_pulses++;
		}
	}

	EXTI_ClearITPendingBit(RF_EXTI_LINE);
}
