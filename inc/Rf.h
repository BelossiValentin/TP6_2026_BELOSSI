#ifndef RF_MODULE_H_
#define RF_MODULE_H_

#include <stdint.h>

#define RF_CODE_A       0x32DC01UL
#define RF_CODE_B       0x32DC02UL
#define RF_CODE_C       0x32DC04UL
#define RF_CODE_D       0x32DC08UL

#define RF_BUTTON_NONE  0U
#define RF_BUTTON_A     1U
#define RF_BUTTON_B     2U
#define RF_BUTTON_C     3U
#define RF_BUTTON_D     4U

void Rf_Init(void);
void Rf_Process(void);
uint8_t Rf_GetLastCode(uint32_t* code);
uint32_t Rf_ReadLastCode(void);
uint8_t Rf_GetFrame(uint32_t* code);
void Rf_Flush(void);
uint8_t Rf_GetButton(uint32_t code);

/* Variables utiles para observar la recepcion desde el depurador. */
extern volatile uint32_t rf_edge_count;
extern volatile uint32_t rf_last_code;
extern volatile uint8_t rf_last_bits;
extern volatile uint32_t rf_dropped_pulses;
extern volatile uint32_t rf_last_low_us;
extern volatile uint32_t rf_valid_frame_count;
extern volatile uint32_t rf_timer_clock_hz;

#endif /* RF_MODULE_H_ */
