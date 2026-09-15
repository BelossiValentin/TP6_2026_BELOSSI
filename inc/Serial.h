#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

#define SERIAL_DEFAULT_BAUDRATE  9600UL
#define SERIAL_LINE_LENGTH       32U

typedef enum
{
	SERIAL_LINE_NONE = 0,
	SERIAL_LINE_OK,
	SERIAL_LINE_OVERFLOW
} SerialLineStatus;

void Serial_Init(uint32_t baudrate);
uint8_t Serial_SetBaudrate(uint32_t baudrate);
uint32_t Serial_GetBaudrate(void);
void Serial_WriteChar(char character);
void Serial_WriteString(const char* text);
void Serial_WaitTransmissionComplete(void);
SerialLineStatus Serial_ReadLine(char* destination, uint8_t capacity);
void Serial_IRQHandler(void);

#endif /* SERIAL_H_ */
