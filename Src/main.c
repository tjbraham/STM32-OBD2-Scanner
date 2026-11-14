/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Tyler Braham
 * @brief          : Main program body
 ******************************************************************************
 */

#include <stdint.h>
#include "stm32f103xb.h" // Device header for stm32 bluepill
#include "can.h" // Include the CAN header file
#include "systick.h" // Include the SysTick header file


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

CAN_Frame_t tx_frame = {
    .id = 0x7DF, // Standard OBD2 broadcast ID
    .dlc = 8, // Data length code (8 bytes)
    .data = {0x02, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00} // Example OBD2 request for engine RPM (PID 0x0C))
};

CAN_Frame_t rx_frame = {0}; // Initialize received frame to zero

int main(void)
{
    SysTick_Init();
    CAN_Init();

    CAN_Transmit(&tx_frame);

    /* Loop forever */
	for(;;){
        if (CAN_Receive(&rx_frame)) {
            // rx_frame now holds the looped-back 0x7DF message
        }
    }
}
