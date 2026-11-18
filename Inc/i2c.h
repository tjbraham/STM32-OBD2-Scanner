#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "stm32f103xb.h" // Device header for stm32 bluepill

void I2C_Init(void);
void I2C_Write(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
void I2C_WriteBuffer(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint16_t size);

#endif // I2C_H