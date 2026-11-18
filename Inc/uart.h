#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32f103xb.h" // Device header for stm32 bluepill

void UART2_Init(void);
void UART2_SendChar(char c);
void UART2_SendString(const char *str);
uint8_t UART2_ReadChar(char *c);

#endif // UART_H