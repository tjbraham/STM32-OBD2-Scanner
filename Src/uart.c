#include "uart.h"

void UART2_Init(void) {
    // Enable clocks for USART2 and GPIOA
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // Enable USART2 clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   // Enable GPIOA clock

    // configure GPIOA pins 2 and 3 (PA2 = TX, PA3 = RX)
    GPIOA->CRL &= ~(GPIO_CRL_MODE2 | GPIO_CRL_CNF2 | GPIO_CRL_MODE3 | GPIO_CRL_CNF3);

    // PA2 (TX) as alternate function push-pull, 50 MHz
    GPIOA->CRL |= (2 << 10) | (2 << 8);
    // MODE2 = 10 (Output mode, max speed 50 MHz), CNF2 = 10 (Alternate function output Push-pull)

    // PA3 (RX) as input floating (MODE3 is already 00 from clearing above)
    GPIOA->CRL |= (0 << 14);
    // CNF3 = 01 (Floating input)

    // Set baud rate to 9600 (using 36 MHz clock)
    USART2->BRR = (234 << 4) | 6; // BRR = 234.375, mantissa = 234, fraction = 6

    // Enable USART2, transmitter (TE) and receiver (RE)
    USART2->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void UART2_SendChar(char c) {
    while (!(USART2->SR & USART_SR_TXE)); // Wait until transmit data register is empty (TXE)
    USART2->DR = (c & 0xFF); // Send the character
}

void UART2_SendString(const char *str) {
    while (*str) {
        UART2_SendChar(*str++); // Send each character in the string
    }
}

uint8_t UART2_ReadChar(char *c) {
    if (USART2->SR & USART_SR_RXNE) { // Check if data is available to read (RXNE)
        *c = (char)(USART2->DR & 0xFF); // Read the received character
        return 1; // Return 1 to indicate a character was read
    }
    return 0; // Return 0 to indicate no character was read
}