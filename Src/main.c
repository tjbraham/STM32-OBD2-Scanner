/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Tyler Braham
 * @brief          : Bare-Metal STM32F103 OBD-II Scanner Application
 ******************************************************************************
 */

#include <stdint.h>
#include <stdio.h>
#include "stm32f103xb.h" // Device header for stm32 bluepill
#include "can.h" // Include the CAN header file
#include "systick.h" // Include the SysTick header file
#include "uart.h" // Include the UART header file
#include "i2c.h" // Include the I2C header file
#include "button.h" // Include the Button header file
#include "obd2.h" // Include the OBD2 header file
#include "ssd1306.h" // Include the SSD1306 header file

typedef enum {
    SCANNER_MODE_RPM = 0,
    SCANNER_MODE_SPEED,
    SCANNER_MODE_COOLANT,
    SCANNER_MODE_READ_DTC,
    SCANNER_MODE_CLEAR_DTC,
    SCANNER_MODE_COUNT
} ScannerMode_t;

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

static void Render_StatusScreen(const char *header, const char *line1, const char *line2) {
    SSD1306_Clear();
    SSD1306_DrawString(0U, 0U, header);
    SSD1306_DrawString(0U, 20U, line1);
    if (line2 != NULL) {
        SSD1306_DrawString(0U, 36U, line2);
    }
    SSD1306_UpdateScreen();
}

/*
 **************************
 * TESTING
 **************************
 */
#define BENCH_LOOPBACK_TEST 1

#if BENCH_LOOPBACK_TEST
static void Bench_InjectMockResponse(ScannerMode_t mode) {
    static uint16_t fake_rpm = 800U;
    CAN_Frame_t mock = { .id = 0x7E8U, .dlc = 8U, .data = {0} };

    if (mode == SCANNER_MODE_RPM) {
        fake_rpm = (fake_rpm >= 6000U) ? 800U : (uint16_t)(fake_rpm + 250U);
        uint16_t raw = (uint16_t)(fake_rpm * 4U);
        mock.data[0] = 0x04U; mock.data[1] = 0x41U; mock.data[2] = OBD2_PID_ENGINE_RPM;
        mock.data[3] = (uint8_t)(raw >> 8);
        mock.data[4] = (uint8_t)(raw & 0xFFU);
        CAN_Transmit(&mock);
    } else if (mode == SCANNER_MODE_SPEED) {
        mock.data[0] = 0x03U; mock.data[1] = 0x41U; mock.data[2] = OBD2_PID_VEHICLE_SPEED;
        mock.data[3] = 88U; // 88 km/h
        CAN_Transmit(&mock);
    } else if (mode == SCANNER_MODE_COOLANT) {
        mock.data[0] = 0x03U; mock.data[1] = 0x41U; mock.data[2] = OBD2_PID_COOLANT_TEMP;
        mock.data[3] = 130U; // 130 - 40 = 90 C
        CAN_Transmit(&mock);
    } else if (mode == SCANNER_MODE_READ_DTC) {
        // 2 fake codes: P0171 (0x01, 0x71) and P0300 (0x03, 0x00)
        mock.data[0] = 0x06U; mock.data[1] = 0x43U; mock.data[2] = 0x02U;
        mock.data[3] = 0x01U; mock.data[4] = 0x71U;
        mock.data[5] = 0x03U; mock.data[6] = 0x00U;
        CAN_Transmit(&mock);
    }
}
#endif


int main(void)
{
    SysTick_Init();
    CAN_Init();
    UART2_Init();
    I2C_Init();
    Button_Init();
    SSD1306_Init();

    ScannerMode_t mode = SCANNER_MODE_RPM;
    uint8_t mode_changed = 1U;
    uint32_t last_poll_ms = 0U;

    CAN_Frame_t rx_frame = {0};
    char line1[24];
    char line2[24];
    char bt_msg[48];
    char bt_cmd = 0;

    UART2_SendString("STM32 OBD-II Scanner Ready\r\n");

    for (;;) {
        uint32_t now = SysTick_GetMs();

        // 1. Check physical button (PB12) or Bluetooth 'n' command to cycle modes
        uint8_t next_mode_requested = Button_IsPressed();
        uint8_t bt_rx = UART2_ReadChar(&bt_cmd);
        if (bt_rx && (bt_cmd == 'n' || bt_cmd == 'N')) {
            next_mode_requested = 1U;
        }

        if (next_mode_requested) {
            mode = (ScannerMode_t)((mode + 1U) % SCANNER_MODE_COUNT);
            mode_changed = 1U;
        }

        // 2. Handle one-time screen setup / single-shot requests on mode entry
        if (mode_changed) {
            mode_changed = 0U;
            last_poll_ms = now;

            switch (mode) {
                case SCANNER_MODE_RPM:
                    Render_StatusScreen("MODE: ENGINE RPM", "RPM: ---", NULL);
                    OBD2_RequestPID(OBD2_PID_ENGINE_RPM);
                    #if BENCH_LOOPBACK_TEST
                        Bench_InjectMockResponse(mode);
                    #endif
                    break;

                case SCANNER_MODE_SPEED:
                    Render_StatusScreen("MODE: VEH SPEED", "SPD: --- KM/H", NULL);
                    OBD2_RequestPID(OBD2_PID_VEHICLE_SPEED);
                    #if BENCH_LOOPBACK_TEST
                        Bench_InjectMockResponse(mode);
                    #endif
                    break;

                case SCANNER_MODE_COOLANT:
                    Render_StatusScreen("MODE: COOLANT", "TEMP: --- C", NULL);
                    OBD2_RequestPID(OBD2_PID_COOLANT_TEMP);
                    #if BENCH_LOOPBACK_TEST
                        Bench_InjectMockResponse(mode);
                    #endif
                    break;

                case SCANNER_MODE_READ_DTC:
                    Render_StatusScreen("MODE: READ DTCS", "SCANNING ECU...", NULL);
                    OBD2_RequestDTCs();
                    #if BENCH_LOOPBACK_TEST
                        Bench_InjectMockResponse(mode);
                    #endif
                    break;

                case SCANNER_MODE_CLEAR_DTC:
                    // Safety gate: requires explicit 'C' over Bluetooth to prevent accidental clears
                    Render_StatusScreen("MODE: CLEAR DTCS", "SEND 'C' ON BT", "TO ERASE CODES");
                    UART2_SendString("Clear DTC mode: send 'C' to confirm erase.\r\n");
                    break;

                default:
                    break;
            }
        }

        // 3. Handle Bluetooth actions inside DTC modes ('r' to re-scan, 'c' to confirm clear)
        if (bt_rx) {
            if (mode == SCANNER_MODE_READ_DTC && (bt_cmd == 'r' || bt_cmd == 'R')) {
                Render_StatusScreen("MODE: READ DTCS", "SCANNING ECU...", NULL);
                OBD2_RequestDTCs();
                #if BENCH_LOOPBACK_TEST
                    Bench_InjectMockResponse(mode);
                #endif
            } else if (mode == SCANNER_MODE_CLEAR_DTC && (bt_cmd == 'c' || bt_cmd == 'C')) {
                OBD2_ClearDTCs();
                Render_StatusScreen("MODE: CLEAR DTCS", "CLEAR SENT", "CHECK ENGINE OFF");
                UART2_SendString("Mode 04 Clear DTC request transmitted.\r\n");
            }
        }

        // 4. Periodic polling for live-data modes
        if (mode == SCANNER_MODE_RPM && (now - last_poll_ms) >= 200U) {
            last_poll_ms = now;
            OBD2_RequestPID(OBD2_PID_ENGINE_RPM);
            #if BENCH_LOOPBACK_TEST
                Bench_InjectMockResponse(mode);
            #endif
        } else if (mode == SCANNER_MODE_SPEED && (now - last_poll_ms) >= 200U) {
            last_poll_ms = now;
            OBD2_RequestPID(OBD2_PID_VEHICLE_SPEED);
            #if BENCH_LOOPBACK_TEST
                Bench_InjectMockResponse(mode);
            #endif
        } else if (mode == SCANNER_MODE_COOLANT && (now - last_poll_ms) >= 500U) {
            last_poll_ms = now;
            OBD2_RequestPID(OBD2_PID_COOLANT_TEMP);
            #if BENCH_LOOPBACK_TEST
                Bench_InjectMockResponse(mode);
            #endif
        }

        // 5. Process any incoming CAN frame immediately
        if (CAN_Receive(&rx_frame)) {
            if (mode == SCANNER_MODE_RPM) {
                uint16_t rpm = 0U;
                if (OBD2_ParseRPM(&rx_frame, &rpm)) {
                    snprintf(line1, sizeof(line1), "RPM: %u", rpm);
                    Render_StatusScreen("MODE: ENGINE RPM", line1, NULL);

                    snprintf(bt_msg, sizeof(bt_msg), "RPM: %u\r\n", rpm);
                    UART2_SendString(bt_msg);
                }
            } else if (mode == SCANNER_MODE_SPEED) {
                uint8_t speed_kmh = 0U;
                if (OBD2_ParseSpeed(&rx_frame, &speed_kmh)) {
                    snprintf(line1, sizeof(line1), "SPD: %u KM/H", speed_kmh);
                    Render_StatusScreen("MODE: VEH SPEED", line1, NULL);

                    snprintf(bt_msg, sizeof(bt_msg), "Speed: %u km/h\r\n", speed_kmh);
                    UART2_SendString(bt_msg);
                }
            } else if (mode == SCANNER_MODE_COOLANT) {
                int16_t temp_c = 0;
                if (OBD2_ParseCoolantTemp(&rx_frame, &temp_c)) {
                    snprintf(line1, sizeof(line1), "TEMP: %d C", temp_c);
                    Render_StatusScreen("MODE: COOLANT", line1, NULL);

                    snprintf(bt_msg, sizeof(bt_msg), "Coolant: %d C\r\n", temp_c);
                    UART2_SendString(bt_msg);
                }
            } else if (mode == SCANNER_MODE_READ_DTC) {
                char dtcs[2][6] = {{0}};
                uint8_t count = OBD2_ParseDTCs(&rx_frame, dtcs);

                if (count == 0U && rx_frame.data[1] == (OBD2_MODE_READ_DTC + 0x40U)) {
                    Render_StatusScreen("MODE: READ DTCS", "NO CODES FOUND", NULL);
                    UART2_SendString("DTCs: None stored\r\n");
                } else if (count > 0U) {
                    snprintf(line1, sizeof(line1), "1: %s", dtcs[0]);
                    if (count > 1U) {
                        snprintf(line2, sizeof(line2), "2: %s", dtcs[1]);
                        Render_StatusScreen("MODE: READ DTCS", line1, line2);
                        snprintf(bt_msg, sizeof(bt_msg), "DTCs: %s, %s\r\n", dtcs[0], dtcs[1]);
                    } else {
                        Render_StatusScreen("MODE: READ DTCS", line1, NULL);
                        snprintf(bt_msg, sizeof(bt_msg), "DTCs: %s\r\n", dtcs[0]);
                    }
                    UART2_SendString(bt_msg);
                }
            }
        }
    }
}
