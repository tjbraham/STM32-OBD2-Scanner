#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include "stm32f103xb.h"

void Button_Init(void);
uint8_t Button_IsPressed(void);

#endif // BUTTON_H