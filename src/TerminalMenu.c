#include "TerminalMenu.h"

#include <stdint.h>

#include "Dac.h"
#include "Rf.h"
#include "Serial.h"

typedef enum
{
	MENU_WAIT_START = 0,
	MENU_WAIT_OPTION,
	MENU_WAIT_RF_RETURN,
	MENU_WAIT_DAC_VALUE,
	MENU_WAIT_DAC_RETURN,
	MENU_WAIT_BAUD_VALUE,
	MENU_WAIT_BAUD_RETURN
} MenuState;

static MenuState menu_state;

static void Menu_ShowMain(void)
{
	Serial_WriteString("\r\n========== TP6 - USART ==========\r\n");
	Serial_WriteString("1 - Leer codigo RF\r\n");
	Serial_WriteString("2 - Generador DAC\r\n");
	Serial_WriteString("3 - Cambiar baudrate\r\n");
	Serial_WriteString("Seleccione una opcion y presione ENTER: ");
	menu_state = MENU_WAIT_OPTION;
}

static void Menu_ShowInvalid(void)
{
	Serial_WriteString("Parametro no valido\r\n");
}

static uint8_t Menu_ParseUnsigned(const char* text, uint32_t maximum,
	uint32_t* value)
{
	uint32_t digit;
	uint32_t result = 0U;

	if ((text == 0) || (value == 0) || (*text == '\0'))
	{
		return 0U;
	}

	while (*text != '\0')
	{
		if ((*text < '0') || (*text > '9'))
		{
			return 0U;
		}

		digit = (uint32_t)(*text - '0');
		if (result > ((maximum - digit) / 10U))
		{
			return 0U;
		}
		result = (result * 10U) + digit;
		text++;
	}

	*value = result;
	return 1U;
}

static void Menu_WriteUnsigned(uint32_t value)
{
	char digits[10];
	uint8_t count = 0U;

	do
	{
		digits[count] = (char)('0' + (value % 10U));
		count++;
		value /= 10U;
	} while ((value != 0U) && (count < (uint8_t)sizeof(digits)));

	while (count > 0U)
	{
		count--;
		Serial_WriteChar(digits[count]);
	}
}

static void Menu_WriteHex24(uint32_t value)
{
	static const char hexadecimal[] = "0123456789ABCDEF";
	int8_t shift;

	value &= 0x00FFFFFFUL;
	for (shift = 20; shift >= 0; shift -= 4)
	{
		Serial_WriteChar(hexadecimal[(value >> (uint8_t)shift) & 0x0FUL]);
	}
}

static const char* Menu_ButtonName(uint8_t button)
{
	switch (button)
	{
		case RF_BUTTON_A:
			return "A";
		case RF_BUTTON_B:
			return "B";
		case RF_BUTTON_C:
			return "C";
		case RF_BUTTON_D:
			return "D";
		default:
			return "desconocido";
	}
}

static void Menu_ShowRfCode(void)
{
	uint32_t code;

	Serial_WriteString("\r\n");
	if (Rf_GetLastCode(&code) == 0U)
	{
		Serial_WriteString("Todavia no se recibio un codigo RF.\r\n");
	}
	else
	{
		Serial_WriteString("Addr: 0x");
		Menu_WriteHex24(code);
		Serial_WriteString(" - Boton ");
		Serial_WriteString(Menu_ButtonName(Rf_GetButton(code)));
		Serial_WriteString("\r\n");
	}
	Serial_WriteString("Presione ENTER para volver al menu.\r\n");
	menu_state = MENU_WAIT_RF_RETURN;
}

static uint8_t Menu_IsSupportedBaudrate(uint32_t baudrate)
{
	switch (baudrate)
	{
		case 4800UL:
		case 9600UL:
		case 19200UL:
		case 38400UL:
		case 57600UL:
		case 115200UL:
			return 1U;
		default:
			return 0U;
	}
}

static void Menu_ProcessValidLine(const char* line)
{
	uint32_t value;

	switch (menu_state)
	{
		case MENU_WAIT_START:
			if (*line == '\0')
			{
				Menu_ShowMain();
			}
			else
			{
				Menu_ShowInvalid();
			}
			break;

		case MENU_WAIT_OPTION:
			if ((line[0] == '1') && (line[1] == '\0'))
			{
				Menu_ShowRfCode();
			}
			else if ((line[0] == '2') && (line[1] == '\0'))
			{
				Serial_WriteString("\r\nIngrese la salida DAC en mV (0 a 3300) y ENTER: ");
				menu_state = MENU_WAIT_DAC_VALUE;
			}
			else if ((line[0] == '3') && (line[1] == '\0'))
			{
				Serial_WriteString("\r\nBaudrates: 4800, 9600, 19200, 38400, 57600, 115200\r\n");
				Serial_WriteString("Ingrese el nuevo baudrate y ENTER: ");
				menu_state = MENU_WAIT_BAUD_VALUE;
			}
			else
			{
				Menu_ShowInvalid();
				Serial_WriteString("Seleccione 1, 2 o 3: ");
			}
			break;

		case MENU_WAIT_RF_RETURN:
			if (*line == '\0')
			{
				Menu_ShowMain();
			}
			else
			{
				Menu_ShowInvalid();
				Serial_WriteString("Presione ENTER para volver al menu.\r\n");
			}
			break;

		case MENU_WAIT_DAC_VALUE:
			if ((Menu_ParseUnsigned(line, DAC_REFERENCE_MV, &value) != 0U) &&
				(Dac_SetMillivolts((uint16_t)value) != 0U))
			{
				Serial_WriteString("Salida DAC configurada en ");
				Menu_WriteUnsigned(value);
				Serial_WriteString(" mV.\r\n");
				Serial_WriteString("Presione ENTER para volver al menu.\r\n");
				menu_state = MENU_WAIT_DAC_RETURN;
			}
			else
			{
				Menu_ShowInvalid();
				Serial_WriteString("Ingrese un valor entre 0 y 3300 mV: ");
			}
			break;

		case MENU_WAIT_DAC_RETURN:
			if (*line == '\0')
			{
				Menu_ShowMain();
			}
			else
			{
				Menu_ShowInvalid();
				Serial_WriteString("Presione ENTER para volver al menu.\r\n");
			}
			break;

		case MENU_WAIT_BAUD_VALUE:
			if ((Menu_ParseUnsigned(line, 115200UL, &value) != 0U) &&
				(Menu_IsSupportedBaudrate(value) != 0U))
			{
				Serial_WriteString("Baudrate aceptado: ");
				Menu_WriteUnsigned(value);
				Serial_WriteString(" baud.\r\n");
				Serial_WriteString("Cambie PuTTY al nuevo baudrate y presione ENTER.\r\n");
				Serial_SetBaudrate(value);
				menu_state = MENU_WAIT_BAUD_RETURN;
			}
			else
			{
				Menu_ShowInvalid();
				Serial_WriteString("Ingrese uno de los baudrates indicados: ");
			}
			break;

		case MENU_WAIT_BAUD_RETURN:
			if (*line == '\0')
			{
				Menu_ShowMain();
			}
			else
			{
				Menu_ShowInvalid();
				Serial_WriteString("Presione ENTER con el nuevo baudrate.\r\n");
			}
			break;

		default:
			menu_state = MENU_WAIT_START;
			Serial_WriteString("Estado recuperado. Presione ENTER para ver el menu.\r\n");
			break;
	}
}

void TerminalMenu_Init(void)
{
	menu_state = MENU_WAIT_START;
	Serial_WriteString("\r\nTP6 USART listo a ");
	Menu_WriteUnsigned(Serial_GetBaudrate());
	Serial_WriteString(" baud, 8N1.\r\n");
	Serial_WriteString("Presione ENTER para ver el menu.\r\n");
}

void TerminalMenu_Process(void)
{
	char line[SERIAL_LINE_LENGTH];
	SerialLineStatus status;

	status = Serial_ReadLine(line, (uint8_t)sizeof(line));
	if (status == SERIAL_LINE_NONE)
	{
		return;
	}

	if (status == SERIAL_LINE_OVERFLOW)
	{
		Menu_ShowInvalid();
		return;
	}

	Menu_ProcessValidLine(line);
}
