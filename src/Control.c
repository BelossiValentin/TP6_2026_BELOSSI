#include "Control.h"

#include "Rf.h"
#include "global.h"

/* Supera el intervalo de repeticion normal del control. */
#define CONTROL_LIBERACION_MS  120U

typedef enum
{
	CONTROL_REPOSO = 0,
	CONTROL_CONFIRMACION,
	CONTROL_ESPERAR_LIBERACION,
	CONTROL_MULTIPLE
} ControlEstado_t;

static ControlEstado_t control_estado = CONTROL_REPOSO;
static char control_candidata = CONTROL_SIN_TECLA;
static uint32_t control_ultimo_frame_ms = 0U;

static char Control_convertirCodigo(uint32_t codigo)
{
	switch (Rf_GetButton(codigo))
	{
	case RF_BUTTON_A:
		return 'A';
	case RF_BUTTON_B:
		return 'B';
	case RF_BUTTON_C:
		return 'C';
	case RF_BUTTON_D:
		return 'D';
	default:
		return CONTROL_SIN_TECLA;
	}
}

void Control_init(void)
{
	control_estado = CONTROL_REPOSO;
	control_candidata = CONTROL_SIN_TECLA;
	control_ultimo_frame_ms = returnSystick();
	Rf_Flush();
}

char Control_getKey(void)
{
	uint32_t codigo = 0U;
	uint32_t ahora = returnSystick();
	char actual = CONTROL_SIN_TECLA;
	uint8_t nuevo_frame;

	nuevo_frame = Rf_GetFrame(&codigo);
	if (nuevo_frame != 0U)
	{
		actual = Control_convertirCodigo(codigo);
	}

	/* FSM de pulsacion unica y liberacion del control. */
	switch (control_estado)
	{
	case CONTROL_REPOSO:
		if (actual != CONTROL_SIN_TECLA)
		{
			control_candidata = actual;
			control_ultimo_frame_ms = ahora;
			control_estado = CONTROL_CONFIRMACION;
		}
		break;

	case CONTROL_CONFIRMACION:
		/* RF.c ya valido la trama completa de 24 bits. */
		if (nuevo_frame != 0U)
		{
			control_ultimo_frame_ms = ahora;
			if (actual != control_candidata)
			{
				control_candidata = CONTROL_SIN_TECLA;
				control_estado = CONTROL_MULTIPLE;
				break;
			}
		}

		control_estado = CONTROL_ESPERAR_LIBERACION;
		return control_candidata;

	case CONTROL_ESPERAR_LIBERACION:
		/* Las repeticiones mantienen la pulsacion sin repetir la accion. */
		if (nuevo_frame != 0U)
		{
			control_ultimo_frame_ms = ahora;
			if (actual != control_candidata)
			{
				control_candidata = CONTROL_SIN_TECLA;
				control_estado = CONTROL_MULTIPLE;
			}
		}
		else if ((uint32_t)(ahora - control_ultimo_frame_ms) >=
				 CONTROL_LIBERACION_MS)
		{
			control_candidata = CONTROL_SIN_TECLA;
			control_estado = CONTROL_REPOSO;
		}
		break;

	case CONTROL_MULTIPLE:
		/* Bloquea nuevas acciones hasta liberar el control. */
		if (nuevo_frame != 0U)
		{
			control_ultimo_frame_ms = ahora;
		}
		else if ((uint32_t)(ahora - control_ultimo_frame_ms) >=
				 CONTROL_LIBERACION_MS)
		{
			control_estado = CONTROL_REPOSO;
		}
		break;

	default:
		control_candidata = CONTROL_SIN_TECLA;
		control_estado = CONTROL_REPOSO;
		break;
	}

	return CONTROL_SIN_TECLA;
}

