#include "systick.h"

#define SYSCLK_FREQ_HZ 72000000UL

static volatile uint32_t ms_ticks = 0; // Millisecond tick counter

void SysTick_Init(void) {
    // Initialize SysTick timer
    SysTick->LOAD = (SYSCLK_FREQ_HZ / 1000) - 1; // Set reload register for 1ms interrupts
    SysTick->VAL = 0; // Reset the SysTick counter value
    // Enable SysTick timer with interrupts
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
                    SysTick_CTRL_TICKINT_Msk | 
                    SysTick_CTRL_ENABLE_Msk; 
}

void SysTick_Handler(void) {
    // Handle SysTick interrupt
    ms_ticks++; // Increment millisecond tick counter
}

uint32_t SysTick_GetMs(void) {
    // Return milliseconds
    return ms_ticks;
}

void SysTick_DelayMs(uint32_t ms) {
    // Delay for specified milliseconds
    uint32_t start = SysTick_GetMs();
    while ((SysTick_GetMs() - start) < ms) {
        // Wait until the specified time has passed
    }
}