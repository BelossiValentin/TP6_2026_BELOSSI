#ifndef ADC_H_
#define ADC_H_

#include "stm32f4xx.h"

void initADC(void);
void readADCs(uint16_t* ADC_1, uint16_t* ADC_2);
void readADCsFiltrados(uint16_t* ADC_1, uint16_t* ADC_2);

#endif /* ADC_H_ */

