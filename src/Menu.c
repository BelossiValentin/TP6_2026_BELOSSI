#include "Menu.h"

#include "ADC.h"
#include "Dac.h"
#include "LCD.h"
#include "PWM.h"
#include "Rf.h"
#include "global.h"

#define MENU_CANTIDAD_ITEMS       5U
#define MENU_LINEA_LCD            16U
#define MENU_REFRESCO_ANALOG_MS   50U
#define MENU_VALOR_INVALIDO       0xFFFFU
#define MENU_ADC_REFERENCIA_MV    3300U
#define MENU_ADC_MAX_CUENTAS      4095U
#define MENU_ADC_R_SERIE_OHM      2200U
#define MENU_ADC_R_TIERRA_OHM     4700U
#define MENU_DAC_MAX_MV           3300U
#define MENU_PWM_PASO             5U

typedef enum
{
	MENU_INIT = 0,
	MENU_DIBUJAR,
	MENU_ESPERAR_TECLA,
	MENU_TECLA_PRESIONADA,
	MENU_BACKLIGHT,
	MENU_CONTROL_PWM,
	MENU_SEGUIDOR_ADC_DAC,
	MENU_VOLTIMETRO
} MenuEstado_t;

static MenuEstado_t menu_estado = MENU_INIT;
static uint8_t item_actual = 0U;
static uint8_t primer_ingreso = 1U;
static uint8_t backlight_mostrado = 0xFFU;
static char tecla_mostrada = 0;
static uint8_t pwm_mostrado = 0xFFU;
static uint32_t mando_mostrado = 0U;
static uint8_t mando_valido_mostrado = 0U;
static uint32_t ultimo_refresco_analogico = 0U;
static uint16_t ad1_mostrado = MENU_VALOR_INVALIDO;
static uint16_t dac_mostrado = MENU_VALOR_INVALIDO;
static uint16_t ad2_mostrado = MENU_VALOR_INVALIDO;
static uint16_t hold_mostrado = MENU_VALOR_INVALIDO;

/* Opciones visibles del menu principal. */
static const char* const menu_items[MENU_CANTIDAD_ITEMS] =
{
	"Tecla presion.",
	"Backlight",
	"Control PWM",
	"Seg. ADC-DAC",
	"Voltimetro"
};

static void Menu_limpiarBuffer(char* linea)
{
	uint8_t i;

	for (i = 0U; i < MENU_LINEA_LCD; i++)
	{
		linea[i] = ' ';
	}
	linea[MENU_LINEA_LCD] = '\0';
}

static void Menu_copiarTexto(char* destino, uint8_t posicion,
							 const char* texto)
{
	while ((posicion < MENU_LINEA_LCD) && (*texto != '\0'))
	{
		destino[posicion] = *texto;
		posicion++;
		texto++;
	}
}

static void Menu_escribirLinea(uint8_t y, const char* texto)
{
	uint8_t i;
	uint8_t fin_texto = 0U;

	LCD_gotoxy(0U, y);
	for (i = 0U; i < MENU_LINEA_LCD; i++)
	{
		if ((fin_texto == 0U) && (texto[i] != '\0'))
		{
			LCD_putc(texto[i]);
		}
		else
		{
			fin_texto = 1U;
			LCD_putc(' ');
		}
	}
}

static void Menu_escribirItem(uint8_t y, uint8_t item, uint8_t seleccionado)
{
	uint8_t i;
	uint8_t fin_texto = 0U;
	const char* texto = menu_items[item];

	LCD_gotoxy(0U, y);
	LCD_putc((seleccionado != 0U) ? '>' : ' ');

	for (i = 0U; i < (MENU_LINEA_LCD - 1U); i++)
	{
		if ((fin_texto == 0U) && (texto[i] != '\0'))
		{
			LCD_putc(texto[i]);
		}
		else
		{
			fin_texto = 1U;
			LCD_putc(' ');
		}
	}
}

static void Menu_dibujarMenu(void)
{
	uint8_t segundo_item = item_actual + 1U;

	/* Muestra dos opciones y marca la actual. */
	if (segundo_item >= MENU_CANTIDAD_ITEMS)
	{
		segundo_item = item_actual - 1U;
		Menu_escribirItem(0U, segundo_item, 0U);
		Menu_escribirItem(1U, item_actual, 1U);
	}
	else
	{
		Menu_escribirItem(0U, item_actual, 1U);
		Menu_escribirItem(1U, segundo_item, 0U);
	}
}

static char Menu_hex(uint8_t valor)
{
	valor &= 0x0FU;
	return (valor < 10U) ? (char)('0' + valor)
						 : (char)('A' + valor - 10U);
}

static void Menu_copiarDecimal(char* destino, uint8_t posicion,
								uint8_t ancho, uint32_t valor)
{
	uint8_t i;

	/* Conversion acotada para evitar sprintf y desbordes. */
	for (i = 0U; (i < ancho) && ((posicion + i) < MENU_LINEA_LCD); i++)
	{
		destino[posicion + i] = ' ';
	}

	do
	{
		if (ancho == 0U)
		{
			break;
		}

		ancho--;
		if ((posicion + ancho) < MENU_LINEA_LCD)
		{
			destino[posicion + ancho] = (char)('0' + (valor % 10U));
		}
		valor /= 10U;
	}
	while (valor != 0U);
}

static void Menu_mostrarPWM(uint8_t duty)
{
	char linea[MENU_LINEA_LCD + 1U];

	Menu_limpiarBuffer(linea);
	Menu_copiarTexto(linea, 0U, "PWM:");
	Menu_copiarDecimal(linea, 5U, 3U, duty);
	linea[8] = '%';
	Menu_escribirLinea(0U, linea);
}

static void Menu_mostrarMando(uint32_t code, uint8_t valido)
{
	char linea[MENU_LINEA_LCD + 1U];
	uint8_t i;

	Menu_limpiarBuffer(linea);
	Menu_copiarTexto(linea, 0U, "Mando:");

	for (i = 0U; i < 6U; i++)
	{
		linea[7U + i] = (valido != 0U)
			? Menu_hex((uint8_t)(code >> ((5U - i) * 4U)))
			: '-';
	}

	Menu_escribirLinea(1U, linea);
}

static uint16_t Menu_cuentasAmV(uint16_t cuentas)
{
	uint32_t milivoltios_adc;
	uint32_t milivoltios_entrada;

	milivoltios_adc = ((uint32_t)cuentas * MENU_ADC_REFERENCIA_MV) +
		(MENU_ADC_MAX_CUENTAS / 2U);
	milivoltios_adc /= MENU_ADC_MAX_CUENTAS;

	milivoltios_entrada =
		(milivoltios_adc * (MENU_ADC_R_SERIE_OHM +
			MENU_ADC_R_TIERRA_OHM)) + (MENU_ADC_R_TIERRA_OHM / 2U);
	milivoltios_entrada /= MENU_ADC_R_TIERRA_OHM;
	return (uint16_t)milivoltios_entrada;
}

static uint16_t Menu_mVAcuentasDAC(uint16_t milivoltios)
{
	uint32_t cuentas;

	if (milivoltios > MENU_DAC_MAX_MV)
	{
		milivoltios = MENU_DAC_MAX_MV;
	}

	cuentas = ((uint32_t)milivoltios * MENU_ADC_MAX_CUENTAS) +
		(MENU_DAC_MAX_MV / 2U);
	cuentas /= MENU_DAC_MAX_MV;
	return (uint16_t)cuentas;
}

static void Menu_mostrarMilivoltios(uint8_t y,
									const char* etiqueta,
									uint16_t milivoltios,
									uint8_t valido)
{
	char linea[MENU_LINEA_LCD + 1U];
	uint8_t posicion = 0U;

	Menu_limpiarBuffer(linea);
	while ((posicion < MENU_LINEA_LCD) && (etiqueta[posicion] != '\0'))
	{
		linea[posicion] = etiqueta[posicion];
		posicion++;
	}

	if ((posicion + 7U) <= MENU_LINEA_LCD)
	{
		if (valido != 0U)
		{
			if (milivoltios > 9999U)
			{
				milivoltios = 9999U;
			}

			Menu_copiarDecimal(linea, posicion, 4U, milivoltios);
		}
		else
		{
			linea[posicion] = '-';
			linea[posicion + 1U] = '-';
			linea[posicion + 2U] = '-';
			linea[posicion + 3U] = '-';
		}

		linea[posicion + 4U] = ' ';
		linea[posicion + 5U] = 'm';
		linea[posicion + 6U] = 'V';
	}

	Menu_escribirLinea(y, linea);
}

static void Menu_entrarSubmenu(void)
{
	primer_ingreso = 1U;

	switch (item_actual)
	{
	case 0U:
		menu_estado = MENU_TECLA_PRESIONADA;
		break;
	case 1U:
		menu_estado = MENU_BACKLIGHT;
		break;
	case 2U:
		menu_estado = MENU_CONTROL_PWM;
		break;
	case 3U:
		menu_estado = MENU_SEGUIDOR_ADC_DAC;
		break;
	case 4U:
		menu_estado = MENU_VOLTIMETRO;
		break;
	default:
		menu_estado = MENU_DIBUJAR;
		break;
	}
}

static void Menu_salirSubmenu(void)
{
	primer_ingreso = 1U;
	menu_estado = MENU_DIBUJAR;
}

void Menu_init(void)
{
	menu_estado = MENU_INIT;
	item_actual = 0U;
	primer_ingreso = 1U;
	backlight_mostrado = 0xFFU;
	tecla_mostrada = 0;
	pwm_mostrado = 0xFFU;
	mando_mostrado = 0U;
	mando_valido_mostrado = 0U;
	ultimo_refresco_analogico = 0U;
	ad1_mostrado = MENU_VALOR_INVALIDO;
	dac_mostrado = MENU_VALOR_INVALIDO;
	ad2_mostrado = MENU_VALOR_INVALIDO;
	hold_mostrado = MENU_VALOR_INVALIDO;
}

void Menu_FSM(char tecla, uint8_t backlight_encendido)
{
	uint16_t cuentas_ad1;
	uint16_t cuentas_ad2;
	uint16_t cuentas_dac;
	uint16_t milivoltios_ad1;
	uint16_t milivoltios_dac;
	uint16_t milivoltios_ad2;
	uint32_t ahora;
	uint32_t code;
	uint8_t duty;

	/* Maquina de estados del menu. */
	switch (menu_estado)
	{
	case MENU_INIT:
		item_actual = 0U;
		primer_ingreso = 1U;
		menu_estado = MENU_DIBUJAR;
		break;

	case MENU_DIBUJAR:
		Menu_dibujarMenu();
		menu_estado = MENU_ESPERAR_TECLA;
		break;

	case MENU_ESPERAR_TECLA:
		if (tecla == 'A')
		{
			item_actual = (item_actual > 0U)
				? (uint8_t)(item_actual - 1U)
				: (MENU_CANTIDAD_ITEMS - 1U);
			menu_estado = MENU_DIBUJAR;
		}
		else if (tecla == 'B')
		{
			item_actual = (item_actual < (MENU_CANTIDAD_ITEMS - 1U))
				? (uint8_t)(item_actual + 1U) : 0U;
			menu_estado = MENU_DIBUJAR;
		}
		else if (tecla == 'C')
		{
			Menu_entrarSubmenu();
		}
		break;

	case MENU_TECLA_PRESIONADA:
		if (primer_ingreso != 0U)
		{
			primer_ingreso = 0U;
			tecla_mostrada = 0;
			Menu_escribirLinea(0U, "Tecla: --");
			Menu_escribirLinea(1U, "D: Salir");
		}

		if (tecla == 'D')
		{
			Menu_salirSubmenu();
		}
		else if ((tecla != 0) && (tecla != tecla_mostrada))
		{
			tecla_mostrada = tecla;
			LCD_gotoxy(7U, 0U);
			LCD_putc(tecla);
			LCD_putc(' ');
		}
		break;

	case MENU_BACKLIGHT:
		if ((primer_ingreso != 0U) ||
			(backlight_mostrado != backlight_encendido))
		{
			primer_ingreso = 0U;
			backlight_mostrado = backlight_encendido;
			Menu_escribirLinea(0U, (backlight_encendido != 0U)
				? "Backlight: ON" : "Backlight: OFF");
			Menu_escribirLinea(1U, "#:ON *:OFF D:Sal");
		}

		if (tecla == 'D')
		{
			Menu_salirSubmenu();
		}
		break;

	case MENU_CONTROL_PWM:
		/* A baja y B sube el duty. */
		if (primer_ingreso != 0U)
		{
			primer_ingreso = 0U;
			pwm_mostrado = PWM_GetDuty();
			mando_mostrado = Rf_ReadLastCode();
			mando_valido_mostrado = (mando_mostrado != 0U) ? 1U : 0U;
			Menu_mostrarPWM(pwm_mostrado);
			Menu_mostrarMando(mando_mostrado, mando_valido_mostrado);
		}

		if (tecla == 'D')
		{
			Menu_salirSubmenu();
			break;
		}

		code = Rf_ReadLastCode();
		if ((code != 0U) && (code != mando_mostrado))
		{
			mando_mostrado = code;
			mando_valido_mostrado = 1U;
			Menu_mostrarMando(code, 1U);
		}

		if ((tecla == 'A') || (tecla == 'B'))
		{
			duty = PWM_GetDuty();

			if ((tecla == 'A') && (duty > 0U))
			{
				duty = (duty >= MENU_PWM_PASO)
					? (uint8_t)(duty - MENU_PWM_PASO) : 0U;
				PWM_SetDuty(duty);
			}
			else if ((tecla == 'B') && (duty < 100U))
			{
				duty = (duty <= (100U - MENU_PWM_PASO))
					? (uint8_t)(duty + MENU_PWM_PASO) : 100U;
				PWM_SetDuty(duty);
			}

			if (pwm_mostrado != PWM_GetDuty())
			{
				pwm_mostrado = PWM_GetDuty();
				Menu_mostrarPWM(pwm_mostrado);
			}
		}
		break;

	case MENU_SEGUIDOR_ADC_DAC:
		/* Copia AD1 sobre la salida DAC. */
		if (primer_ingreso != 0U)
		{
			primer_ingreso = 0U;
			ad1_mostrado = MENU_VALOR_INVALIDO;
			dac_mostrado = MENU_VALOR_INVALIDO;
			ultimo_refresco_analogico =
				returnSystick() - MENU_REFRESCO_ANALOG_MS;
			Menu_mostrarMilivoltios(0U, "AD1: ", 0U, 0U);
			Menu_mostrarMilivoltios(1U, "DAC: ", 0U, 0U);
		}

		if (tecla == 'D')
		{
			Dac_SetCounts(0U);
			Menu_salirSubmenu();
			break;
		}

		readADCsFiltrados(&cuentas_ad1, &cuentas_ad2);
		milivoltios_ad1 = Menu_cuentasAmV(cuentas_ad1);
		cuentas_dac = Menu_mVAcuentasDAC(milivoltios_ad1);
		Dac_SetCounts(cuentas_dac);
		ahora = returnSystick();

		if ((uint32_t)(ahora - ultimo_refresco_analogico) >=
			MENU_REFRESCO_ANALOG_MS)
		{
			ultimo_refresco_analogico = ahora;
			milivoltios_dac = Menu_cuentasAmV(cuentas_ad2);

			if (ad1_mostrado != milivoltios_ad1)
			{
				ad1_mostrado = milivoltios_ad1;
				Menu_mostrarMilivoltios(0U, "AD1: ",
					milivoltios_ad1, 1U);
			}
			if (dac_mostrado != milivoltios_dac)
			{
				dac_mostrado = milivoltios_dac;
				Menu_mostrarMilivoltios(1U, "DAC: ",
					milivoltios_dac, 1U);
			}
		}
		break;

	case MENU_VOLTIMETRO:
		/* Mide AD2 y permite congelar el valor. */
		if (primer_ingreso != 0U)
		{
			primer_ingreso = 0U;
			ad2_mostrado = MENU_VALOR_INVALIDO;
			hold_mostrado = MENU_VALOR_INVALIDO;
			ultimo_refresco_analogico =
				returnSystick() - MENU_REFRESCO_ANALOG_MS;
			Menu_mostrarMilivoltios(0U, "HOLD: ", 0U, 0U);
			Menu_mostrarMilivoltios(1U, "AD2: ", 0U, 0U);
		}

		if (tecla == 'D')
		{
			Menu_salirSubmenu();
			break;
		}

		readADCsFiltrados(&cuentas_ad1, &cuentas_ad2);
		milivoltios_ad2 = Menu_cuentasAmV(cuentas_ad2);

		if (tecla == '0')
		{
			hold_mostrado = milivoltios_ad2;
			Menu_mostrarMilivoltios(0U, "HOLD: ", hold_mostrado, 1U);
		}

		ahora = returnSystick();
		if ((uint32_t)(ahora - ultimo_refresco_analogico) >=
			MENU_REFRESCO_ANALOG_MS)
		{
			ultimo_refresco_analogico = ahora;
			if (ad2_mostrado != milivoltios_ad2)
			{
				ad2_mostrado = milivoltios_ad2;
				Menu_mostrarMilivoltios(1U, "AD2: ",
					milivoltios_ad2, 1U);
			}
		}
		break;

	default:
		menu_estado = MENU_INIT;
		break;
	}
}

