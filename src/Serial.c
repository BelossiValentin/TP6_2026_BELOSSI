#include "Serial.h"

#include "stm32f4xx.h"

/*
 * Interfaz RS-232 del kit elegida segun la guia de la catedra:
 * UART4_TX = PC10, UART4_RX = PC11, funcion alternativa AF8.
 * Esta alternativa evita el conflicto con PA0, utilizado por el teclado.
 */
#define SERIAL_USART              UART4
#define SERIAL_USART_CLOCK        RCC_APB1Periph_UART4
#define SERIAL_USART_IRQn         UART4_IRQn
#define SERIAL_GPIO               GPIOC
#define SERIAL_GPIO_CLOCK         RCC_AHB1Periph_GPIOC
#define SERIAL_TX_PIN             GPIO_Pin_10
#define SERIAL_RX_PIN             GPIO_Pin_11
#define SERIAL_TX_PIN_SOURCE      GPIO_PinSource10
#define SERIAL_RX_PIN_SOURCE      GPIO_PinSource11
#define SERIAL_GPIO_AF            GPIO_AF_UART4

static volatile char serial_rx_buffer[SERIAL_LINE_LENGTH];
static volatile uint8_t serial_rx_length;
static volatile uint8_t serial_line_ready;
static volatile uint8_t serial_line_overflow;
static volatile uint8_t serial_ignore_lf;
static uint32_t serial_baudrate;

static void Serial_ConfigurePins(void)
{
	GPIO_InitTypeDef gpio;

	RCC_AHB1PeriphClockCmd(SERIAL_GPIO_CLOCK, ENABLE);
	GPIO_PinAFConfig(SERIAL_GPIO, SERIAL_TX_PIN_SOURCE, SERIAL_GPIO_AF);
	GPIO_PinAFConfig(SERIAL_GPIO, SERIAL_RX_PIN_SOURCE, SERIAL_GPIO_AF);

	gpio.GPIO_Pin = SERIAL_TX_PIN | SERIAL_RX_PIN;
	gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(SERIAL_GPIO, &gpio);
}

static void Serial_ConfigurePeripheral(uint32_t baudrate)
{
	USART_InitTypeDef usart;

	usart.USART_BaudRate = baudrate;
	usart.USART_WordLength = USART_WordLength_8b;
	usart.USART_StopBits = USART_StopBits_1;
	usart.USART_Parity = USART_Parity_No;
	usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(SERIAL_USART, &usart);
	USART_ITConfig(SERIAL_USART, USART_IT_RXNE, ENABLE);
	USART_ITConfig(SERIAL_USART, USART_IT_ERR, ENABLE);
	USART_Cmd(SERIAL_USART, ENABLE);
}

static void Serial_ClearReception(void)
{
	serial_rx_length = 0U;
	serial_line_ready = 0U;
	serial_line_overflow = 0U;
	serial_ignore_lf = 0U;
}

void Serial_Init(uint32_t baudrate)
{
	NVIC_InitTypeDef nvic;

	if (baudrate == 0U)
	{
		baudrate = SERIAL_DEFAULT_BAUDRATE;
	}

	Serial_ConfigurePins();
	RCC_APB1PeriphClockCmd(SERIAL_USART_CLOCK, ENABLE);
	Serial_ClearReception();
	Serial_ConfigurePeripheral(baudrate);
	serial_baudrate = baudrate;

	nvic.NVIC_IRQChannel = SERIAL_USART_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 2U;
	nvic.NVIC_IRQChannelSubPriority = 0U;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
}

uint8_t Serial_SetBaudrate(uint32_t baudrate)
{
	if (baudrate == 0U)
	{
		return 0U;
	}

	Serial_WaitTransmissionComplete();
	NVIC_DisableIRQ(SERIAL_USART_IRQn);
	USART_Cmd(SERIAL_USART, DISABLE);
	Serial_ClearReception();
	Serial_ConfigurePeripheral(baudrate);
	serial_baudrate = baudrate;
	NVIC_EnableIRQ(SERIAL_USART_IRQn);

	return 1U;
}

uint32_t Serial_GetBaudrate(void)
{
	return serial_baudrate;
}

void Serial_WriteChar(char character)
{
	while ((SERIAL_USART->SR & USART_SR_TXE) == 0U)
	{
	}

	SERIAL_USART->DR = (uint16_t)(uint8_t)character;
}

void Serial_WriteString(const char* text)
{
	if (text == 0)
	{
		return;
	}

	while (*text != '\0')
	{
		Serial_WriteChar(*text);
		text++;
	}
}

void Serial_WaitTransmissionComplete(void)
{
	while ((SERIAL_USART->SR & USART_SR_TC) == 0U)
	{
	}
}

SerialLineStatus Serial_ReadLine(char* destination, uint8_t capacity)
{
	SerialLineStatus status;
	uint8_t length;
	uint8_t index;

	if ((destination == 0) || (capacity == 0U))
	{
		return SERIAL_LINE_NONE;
	}

	NVIC_DisableIRQ(SERIAL_USART_IRQn);
	if (serial_line_ready == 0U)
	{
		NVIC_EnableIRQ(SERIAL_USART_IRQn);
		return SERIAL_LINE_NONE;
	}

	length = serial_rx_length;
	status = (serial_line_overflow != 0U) ?
		SERIAL_LINE_OVERFLOW : SERIAL_LINE_OK;
	if (length >= capacity)
	{
		length = (uint8_t)(capacity - 1U);
		status = SERIAL_LINE_OVERFLOW;
	}

	for (index = 0U; index < length; index++)
	{
		destination[index] = serial_rx_buffer[index];
	}
	destination[length] = '\0';

	serial_rx_length = 0U;
	serial_line_ready = 0U;
	serial_line_overflow = 0U;
	NVIC_EnableIRQ(SERIAL_USART_IRQn);

	return status;
}

void Serial_IRQHandler(void)
{
	uint32_t status_register;
	uint8_t received;

	status_register = SERIAL_USART->SR;
	if ((status_register & (USART_SR_RXNE | USART_SR_ORE |
		USART_SR_NE | USART_SR_FE | USART_SR_PE)) == 0U)
	{
		return;
	}

	/* Leer DR limpia RXNE y, junto con la lectura de SR, los errores. */
	received = (uint8_t)SERIAL_USART->DR;
	if ((status_register & (USART_SR_ORE | USART_SR_NE |
		USART_SR_FE | USART_SR_PE)) != 0U)
	{
		serial_line_overflow = 1U;
		return;
	}

	if ((received == '\n') && (serial_ignore_lf != 0U))
	{
		serial_ignore_lf = 0U;
		return;
	}

	serial_ignore_lf = 0U;
	if ((received == '\r') || (received == '\n'))
	{
		if (serial_line_ready == 0U)
		{
			serial_line_ready = 1U;
		}
		if (received == '\r')
		{
			serial_ignore_lf = 1U;
		}
		return;
	}

	if (serial_line_ready != 0U)
	{
		return;
	}

	if ((received == 8U) || (received == 127U))
	{
		if (serial_rx_length > 0U)
		{
			serial_rx_length--;
		}
		return;
	}

	if (serial_rx_length < (SERIAL_LINE_LENGTH - 1U))
	{
		serial_rx_buffer[serial_rx_length] = (char)received;
		serial_rx_length++;
	}
	else
	{
		serial_line_overflow = 1U;
	}
}

void UART4_IRQHandler(void)
{
	Serial_IRQHandler();
}
