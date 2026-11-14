#ifndef CAN_H
#define CAN_H

#include "stm32f103xb.h"
#include <stdint.h>

// standard 11-bit CAN message structure
typedef struct {
    uint32_t id;          // 11-bit identifier (for OBD2, this is usually 0x7DF for requests and 0x7E8-0x7EF for responses)
    uint8_t data[8];     // Data field/payload (0-8 bytes)
    uint8_t dlc;         // Data Length Code (0-8 in bytes)
} CAN_Frame_t;

void CAN_Init(void);
uint8_t CAN_Transmit(const CAN_Frame_t *frame);
uint8_t CAN_Receive(CAN_Frame_t *frame);

#endif // CAN_H