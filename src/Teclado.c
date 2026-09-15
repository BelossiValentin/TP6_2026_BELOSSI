#include "Teclado.h"

#include "global.h"
#include "stm32f4xx.h"

#define TECLADO_FILAS        4U
#define TECLADO_COLUMNAS     4U
#define TECLADO_DEBOUNCE_MS  20U
#define TECLADO_SETTLE_US    10U

typedef struct
{
	GPIO_TypeDef* puerto;
	uint16_t pin;
} TecladoPin_t;

typedef enum
{
	TECLADO_REPOSO = 0,
	TECLADO_DEBOUNCE,
	TECLADO_CONFIRMACION,
	TECLADO_ESPERAR_LIBERACION,
	TECLADO_MULTIPLE
} TecladoEstado_t;

static const char teclas[TECLADO_FILAS][TECLADO_COLUMNAS] =
{
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

static const TecladoPin_t filas[TECLADO_FILAS] =
{
	{GPIOC, GPIO_Pin_0},
	{GPIOC, GPIO_Pin_3},
	{GPIOC, GPIO_Pin_2},
	{GPIOA, GPIO_Pin_0}
};

static const TecladoPin_t columnas[TECLADO_COLUMNAS] =
{
	{GPIOE, GPIO_Pin_8},
	{GPIOE, GPIO_Pin_9},
	{GPIOE, GPIO_Pin_10},
	{GPIOE, GPIO_Pin_11}
};

static void Teclado_apagarFilas(void)
{
	uint8_t i;

	for (i = 0U; i < TECLADO_FILAS; i++)
	{
		GPIO_ResetBits(filas[i].puerto, filas[i].pin);
	}
}

static char Teclado_escanear(uint8_t* cantidad_teclas)
{
	uint8_t fila;
	uint8_t columna;
	char tecla_detectada;

	*cantidad_teclas = 0U;
	tecla_detectada = TECLADO_SIN_TECLA;

	/* Activa una fila y lee todas las columnas. */
	for (fila = 0U; fila < TECLADO_FILAS; fila++)
	{
		Teclado_apagarFilas();
		GPIO_SetBits(filas[fila].puerto, filas[fila].pin);
		delay_us(TECLADO_SETTLE_US);

		for (columna = 0U; columna < TECLADO_COLUMNAS; columna++)
		{
			if (GPIO_ReadInputDataBit(columnas[columna].puerto,
									 columnas[columna].pin) != Bit_RESET)
			{
				if (*cantidad_teclas == 0U)
				{
					tecla_detectada = teclas[fila][columna];
				}
				(*cantidad_teclas)++;
			}
		}
	}

	Teclado_apagarFilas();
	return tecla_detectada;
}

void Teclado_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	uint8_t i;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |
						   RCC_AHB1Periph_GPIOC |
						   RCC_AHB1Periph_GPIOE,
						   ENABLE);

	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

	for (i = 0U; i < TECLADO_FILAS; i++)
	{
		GPIO_InitStruct.GPIO_Pin = filas[i].pin;
		GPIO_Init(filas[i].puerto, &GPIO_InitStruct);
		GPIO_ResetBits(filas[i].puerto, filas[i].pin);
	}

	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;

	for (i = 0U; i < TECLADO_COLUMNAS; i++)
	{
		GPIO_InitStruct.GPIO_Pin = columnas[i].pin;
		GPIO_Init(columnas[i].puerto, &GPIO_InitStruct);
	}
}

char Teclado_getKey(void)
{
	static TecladoEstado_t estado = TECLADO_REPOSO;
	static char candidata = TECLADO_SIN_TECLA;
	static uint32_t tiempo_cambio = 0U;
	char actual;
	uint8_t cantidad_teclas;

	/* FSM de antirrebote y liberacion. */
	switch (estado)
	{
	case TECLADO_REPOSO:
		actual = Teclado_escanear(&cantidad_teclas);
		if (cantidad_teclas == 1U)
		{
			candidata = actual;
			tiempo_cambio = returnSystick();
			estado = TECLADO_DEBOUNCE;
		}
		else if (cantidad_teclas > 1U)
		{
			estado = TECLADO_MULTIPLE;
		}
		break;

	case TECLADO_DEBOUNCE:
		if ((uint32_t)(returnSystick() - tiempo_cambio) >=
			TECLADO_DEBOUNCE_MS)
		{
			estado = TECLADO_CONFIRMACION;
		}
		break;

	case TECLADO_CONFIRMACION:
		actual = Teclado_escanear(&cantidad_teclas);
		if ((cantidad_teclas == 1U) && (actual == candidata))
		{
			estado = TECLADO_ESPERAR_LIBERACION;
			return candidata;
		}
		else if (cantidad_teclas > 1U)
		{
			estado = TECLADO_MULTIPLE;
		}
		else
		{
			estado = TECLADO_REPOSO;
		}
		break;

	case TECLADO_ESPERAR_LIBERACION:
		(void)Teclado_escanear(&cantidad_teclas);
		if (cantidad_teclas == 0U)
		{
			candidata = TECLADO_SIN_TECLA;
			estado = TECLADO_REPOSO;
		}
		else if (cantidad_teclas > 1U)
		{
			candidata = TECLADO_SIN_TECLA;
			estado = TECLADO_MULTIPLE;
		}
		break;

	case TECLADO_MULTIPLE:
		(void)Teclado_escanear(&cantidad_teclas);
		if (cantidad_teclas == 0U)
		{
			candidata = TECLADO_SIN_TECLA;
			estado = TECLADO_REPOSO;
		}
		break;

	default:
		candidata = TECLADO_SIN_TECLA;
		estado = TECLADO_REPOSO;
		break;
	}

	return TECLADO_SIN_TECLA;
}

