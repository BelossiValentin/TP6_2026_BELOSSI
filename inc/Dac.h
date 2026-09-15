#ifndef DAC_MODULE_H_
#define DAC_MODULE_H_

#include <stdint.h>

#define DAC_REFERENCE_MV  3300U

void Dac_Init(void);
void Dac_SetCounts(uint16_t counts);
uint8_t Dac_SetMillivolts(uint16_t millivolts);
uint16_t Dac_GetMillivolts(void);

#endif /* DAC_MODULE_H_ */
