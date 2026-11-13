/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Tyler Braham
 * @brief          : Main program body
 ******************************************************************************
 */

#include <stdint.h>
#include "stm32f103xb.h" // Device header for stm32 bluepill


void SystemInit(void) {
    // This function is called before main() and can be used to set up the system clock and other low-level configurations.
    RCC->CR |= RCC_CR_HSEON; // Enable HSE (High-Speed External) oscillator

    while (!(RCC->CR & RCC_CR_HSERDY)); // Wait until HSE is ready

    FLASH->ACR |= FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2; // Enable prefetch buffer and set flash latency

    // HCLK (AHB) = SYSCLK (72 MHz)
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;  
    
    // PCLK2 (APB2) = HCLK (72 MHz)
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1; 
    
    // PCLK1 (APB1) = HCLK / 2 (36 MHz) - This is mandatory for the bxCAN peripheral
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

    RCC->CFGR |= RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9; // Set PLL source to HSE and multiply by 9 (8 MHz * 9 = 72 MHz)

    RCC->CR |= RCC_CR_PLLON; // Enable PLL
    while (!(RCC->CR & RCC_CR_PLLRDY)); // Wait until PLL is ready

    RCC->CFGR |= RCC_CFGR_SW_PLL; // Select PLL as system clock

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // Wait until PLL is used as system clock
}

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

int main(void)
{
    CAN_Init();
    /* Loop forever */
	for(;;);
}
