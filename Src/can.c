#include "can.h"
#include <stdint.h>

void CAN_Init(void) {
    //Enable Clocks for CAN1 and GPIOA (CAN pins are on GPIOA)
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;  // Turn on CAN1 clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;  // Turn on GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;  // Turn on Alternate Function clock

    // Configure CAN pins (PA11 and PA12)
    // PA11 (CAN_RX) as input floating
    // PA12 (CAN_TX) as alternate function push-pull, 50 MHz
    GPIOA->CRH &= ~(GPIO_CRH_MODE11 | GPIO_CRH_CNF11 | GPIO_CRH_MODE12 | GPIO_CRH_CNF12);

    // Set PA11 to Input Floating (CNF = 01, MODE = 00)
    GPIOA->CRH |= (1 << 14); 
    // Set PA12 to AF Push-Pull (CNF = 10, MODE = 11)
    GPIOA->CRH |= (2 << 18) | (3 << 16);

    //Enter CAN Initialization mode
    CAN1->MCR &= ~CAN_MCR_SLEEP; // Exit sleep mode
    CAN1->MCR |= CAN_MCR_INRQ; // Request initialization mode
    while (!(CAN1->MSR & CAN_MSR_INAK)); // Wait until initialization mode is active

    // Set CAN bit timing (500kbps which is OBD2 standard)
    // 36MHz APB1 clock, to get 500kpbs, we need 72 time quanta total (36MHz / 500kHz = 72)
    // set prescaler = 4, CAN clock is 36MHz / 4 = 9MHz, so we need 18 time quanta per bit (9MHz / 500kHz = 18)
    // 18 Tq = 1 (Sync) + 14 (time segment 1) + 3 (time segment 2)
    // Hardware registers store (value - 1), so:
    uint32_t brp = 4 - 1;   // Baud Rate Prescaler
    uint32_t ts1 = 14 - 1;  // Time Segment 1
    uint32_t ts2 = 3 - 1;   // Time Segment 2

    CAN1->BTR = (ts2 << 20) | (ts1 << 16) | (brp << 0); // Set the BTR register

    // Config filter bank 0 to accept all messages (for testing)
    CAN1->FMR |= CAN_FMR_FINIT; // Enter filter initialization mode

    CAN1->FA1R &= ~(1 << 0); // Deactivate filter 0
    CAN1->FS1R |= (1 << 0); // Set filter 0 to 32-bit scale
    CAN1->FM1R &= ~(1 << 0); // Set filter 0 to identifier mask mode

    // set ID and mask to 0 to accept all traffic
    CAN1->sFilterRegister[0].FR1 = 0;   
    CAN1->sFilterRegister[0].FR2 = 0;

    CAN1->FA1R |= (1 << 0); // Reactivate Filter 0
    CAN1->FMR &= ~CAN_FMR_FINIT; // Exit Filter Initialization mode

    // enter normal mode
    CAN1->MCR &= ~CAN_MCR_INRQ; // Exit initialization mode
    while (CAN1->MSR & CAN_MSR_INAK); // Wait until normal mode is active

}

uint8_t CAN_Transmit(const CAN_Frame_t *frame) {
    // check if transmit mailbox 0 is empty
    if (!(CAN1->TSR & CAN_TSR_TME0)) {
        return 0; // Mailbox not empty, cannot transmit
    }
    
    // set 11 bit standard ID (31:21), clear IDE and RTR bits
    CAN1->sTxMailBox[0].TIR = (uint32_t)(frame->id << 21);

    // set data length code (DLC, 3:0)
    CAN1->sTxMailBox[0].TDTR = (frame->dlc & 0x0FU);

    // copy data bytes to mailbox data registers
    CAN1->sTxMailBox[0].TDLR = ((uint32_t)frame->data[3] << 24) |
                               ((uint32_t)frame->data[2] << 16) |
                               ((uint32_t)frame->data[1] << 8)  |
                               ((uint32_t)frame->data[0] << 0);

    CAN1->sTxMailBox[0].TDHR = ((uint32_t)frame->data[7] << 24) |
                               ((uint32_t)frame->data[6] << 16) |
                               ((uint32_t)frame->data[5] << 8)  |
                               ((uint32_t)frame->data[4] << 0);

    // request transmission
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
    return 1; // Transmission requested
}

uint8_t CAN_Receive(CAN_Frame_t *frame) {
    // check if FIFO 0 has at least one message pending (FMPO bits 1:0)
    if ((CAN1->RF0R & CAN_RF0R_FMP0) == 0) {
        return 0; // No message pending
    }

    // extract 11 bit ID and DLC
    frame->id = (uint16_t)(CAN1->sFIFOMailBox[0].RIR >> 21) & 0x7FFU;
    frame->dlc = (uint8_t)(CAN1->sFIFOMailBox[0].RDTR & 0x0FU);

    // unpack 8 paylod bytes
    uint32_t rdlr = CAN1->sFIFOMailBox[0].RDLR;
    uint32_t rdhr = CAN1->sFIFOMailBox[0].RDHR;

    frame->data[0] = (uint8_t)((rdlr >> 0)  & 0xFFU);
    frame->data[1] = (uint8_t)((rdlr >> 8)  & 0xFFU);
    frame->data[2] = (uint8_t)((rdlr >> 16) & 0xFFU);
    frame->data[3] = (uint8_t)((rdlr >> 24) & 0xFFU);

    frame->data[4] = (uint8_t)((rdhr >> 0)  & 0xFFU);
    frame->data[5] = (uint8_t)((rdhr >> 8)  & 0xFFU);
    frame->data[6] = (uint8_t)((rdhr >> 16) & 0xFFU);
    frame->data[7] = (uint8_t)((rdhr >> 24) & 0xFFU);

    // release FIFO 0 output mailbox
    CAN1->RF0R |= CAN_RF0R_RFOM0;
    return 1; // Message received
}
