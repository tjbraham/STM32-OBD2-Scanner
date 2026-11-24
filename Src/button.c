#include "button.h"
#include "systick.h"

#define DEBOUNCE_DELAY_MS 50U // Debounce delay in milliseconds

void Button_Init(void) {
    // enable clock for GPIOB
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    // clear MODE12 and CNF12 bits for PB12
    GPIOB->CRH &= ~(GPIO_CRH_MODE12 | GPIO_CRH_CNF12);

    // config PB12 as input with pull up / pull down (CNF12 = 10, MODE12 = 00)
    GPIOB->CRH |= GPIO_CRH_CNF12_1; // CNF12 = 10 (input with pull-up/pull-down)

    // enable internal pull-up resistor for PB12 (set ODR12 to 1)
    GPIOB->ODR |= GPIO_ODR_ODR12;
}

uint8_t Button_IsPressed(void) {
    static uint8_t last_raw_state = 0U; // Assume button is not pressed initially (pull-up)
    static uint8_t stable_state = 0U; 
    static uint32_t last_debounce_time = 0U; // Last time the button was debounced

    // active low: 0v on PB12 means button is pressed (1)
    uint8_t raw_state = ((GPIOB->IDR & GPIO_IDR_IDR12) == 0U) ? 1U : 0U;

    // reset debounce timer when raw pin reading changes
    if (raw_state != last_raw_state) {
        last_debounce_time = SysTick_GetMs(); // Get current tick count
        last_raw_state = raw_state; // Update last raw state
    }

    // only update stable state if the raw state has been stable for at least 50 ms
    if ((SysTick_GetMs() - last_debounce_time) >= DEBOUNCE_DELAY_MS) {
        if (raw_state != stable_state) {
            stable_state = raw_state; // Update stable state

            // return 1 on transition from released(0) to pressed(1)
            if (stable_state == 1U) {
                return 1U; // Button was pressed
            }
        }
    }
    return 0U;
}