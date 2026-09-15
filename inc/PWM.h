#ifndef PWM_H_
#define PWM_H_

#include "stm32f4xx.h"

void PWM_Init(uint32_t frecuencia_hz);
void PWM_SetDuty(uint8_t duty);
uint8_t PWM_GetDuty(void);

#endif /* PWM_H_ */

