# TP6_2026_BELOSSI

## Depuración

Usar la configuración compartida `TP6_2026_BELOSSI Debug - Corregida`.
Está preparada para STM32F407VETx, Cortex-M4, ST-Link por SWD a 1 MHz,
OpenOCD en el puerto 3333, carga del ELF de Debug y parada inicial en `main`.

Antes de iniciarla, Windows debe detectar el ST-Link y la placa debe estar
alimentada. Revisar USB, GND, SWDIO y SWCLK si OpenOCD informa
`no matching adapter found`.

## Programa TP6

El firmware conserva la arquitectura acumulativa de TP3, TP4 y TP5 y agrega
las funciones de TP6. Los archivos `.c` están en `src` y las cabeceras `.h` en
`inc`.

La aplicación ejecuta dos máquinas de estados independientes:

- `Menu`: menú local en el LCD 16x2, operado con teclado o control RF.
- `TerminalMenu`: menú TP6 enviado y operado desde PuTTY.

También se incluyen los módulos `LCD`, `Teclado`, `Control`, `ADC`, `Dac`,
`PWM`, `Rf`, `Serial`, `global` y `App`. El procesamiento se realiza en el
lazo principal; las interrupciones capturan caracteres, duraciones RF y la
base de tiempo de un milisegundo.

Configuración de hardware utilizada:

- Terminal RS-232: UART4, PC10 (TX) y PC11 (RX), 9600 baud iniciales, 8N1.
- Salida DAC: DAC2 por PA5, rango lógico de 0 a 3300 mV.
- Entrada RF: PD1/EXTI1, temporización con TIM2 a 1 MHz.
- LCD 16x2: PE1 a PE6 y PC13; backlight por PD8.
- Teclado matricial: PC0, PC2, PC3, PA0 y PE8 a PE11.

Temporización del control RF:

- Cristal HSE de 8 MHz y reloj del núcleo de 168 MHz.
- TIM2 se ajusta dinámicamente desde PCLK1 para medir 1 microsegundo por cuenta.
- Sincronismo bajo válido: 8000 a 16000 us.
- Pulso corto válido: 150 a 650 us; pulso largo válido: 700 a 1600 us.
- Una pulsación se libera después de 120 ms sin recibir otra trama del control.

Para verificar la recepción durante la depuración se pueden observar
`SystemCoreClock` (168000000), `rf_timer_clock_hz` (84000000),
`rf_last_low_us`, `rf_valid_frame_count`, `rf_last_code` y
`rf_dropped_pulses`.

PuTTY debe tener control de flujo deshabilitado. Al encender, presionar ENTER
para mostrar el menú. Los baudrates aceptados son 4800, 9600, 19200, 38400,
57600 y 115200.

La selección de pines de la interfaz serie está concentrada al principio de
`src/Serial.c`. Se utiliza PC10/PC11 porque PA0 pertenece al teclado matricial.
