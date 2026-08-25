# TP6_2026_BELOSSI

## Depuración

Usar la configuración compartida `TP6_2026_BELOSSI Debug - Corregida`.
Está preparada para STM32F407VETx, Cortex-M4, ST-Link por SWD a 1 MHz,
OpenOCD en el puerto 3333, carga del ELF de Debug y parada inicial en `main`.

Antes de iniciarla, Windows debe detectar el ST-Link y la placa debe estar
alimentada. Revisar USB, GND, SWDIO y SWCLK si OpenOCD informa
`no matching adapter found`.
