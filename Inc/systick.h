#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>
#include "stm32f103xb.h"


void SysTick_Init(void);
uint32_t SysTick_GetMs(void);
void SysTick_DelayMs(uint32_t ms);

#endif // SYSTICK_H