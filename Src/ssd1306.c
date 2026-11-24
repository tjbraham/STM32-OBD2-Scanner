#include "ssd1306.h"
#include "i2c.h"

#define SSD1306_CTRL_CMD    0x00U
#define SSD1306_CTRL_DATA   0x40U
#define SSD1306_BUF_SIZE    ((SSD1306_WIDTH * SSD1306_HEIGHT) / 8U) // 1024 bytes

static uint8_t ssd1306_buffer[SSD1306_BUF_SIZE];

static void ssd1306_cmd(uint8_t cmd) {
    I2C_Write(SSD1306_I2C_ADDR, SSD1306_CTRL_CMD, cmd);
}

// Compact 5x7 column-major glyph lookup (bit 0 = top row, bit 6 = bottom row)
static void ssd1306_get_glyph(char c, uint8_t cols[5]) {
    if (c >= 'a' && c <= 'z') {
        c = (char)(c - ('a' - 'A')); // Fold lowercase to uppercase
    }

    uint8_t g0 = 0x00U, g1 = 0x00U, g2 = 0x00U, g3 = 0x00U, g4 = 0x00U;

    switch (c) {
        case '0': g0=0x3EU; g1=0x41U; g2=0x41U; g3=0x41U; g4=0x3EU; break;
        case '1': g0=0x00U; g1=0x42U; g2=0x7FU; g3=0x40U; g4=0x00U; break;
        case '2': g0=0x62U; g1=0x51U; g2=0x49U; g3=0x49U; g4=0x46U; break;
        case '3': g0=0x22U; g1=0x41U; g2=0x49U; g3=0x49U; g4=0x36U; break;
        case '4': g0=0x18U; g1=0x14U; g2=0x12U; g3=0x7FU; g4=0x10U; break;
        case '5': g0=0x27U; g1=0x45U; g2=0x45U; g3=0x45U; g4=0x39U; break;
        case '6': g0=0x3EU; g1=0x49U; g2=0x49U; g3=0x49U; g4=0x30U; break;
        case '7': g0=0x01U; g1=0x71U; g2=0x09U; g3=0x05U; g4=0x03U; break;
        case '8': g0=0x36U; g1=0x49U; g2=0x49U; g3=0x49U; g4=0x36U; break;
        case '9': g0=0x06U; g1=0x49U; g2=0x49U; g3=0x49U; g4=0x3EU; break;

        case 'A': g0=0x7EU; g1=0x09U; g2=0x09U; g3=0x09U; g4=0x7EU; break;
        case 'B': g0=0x7FU; g1=0x49U; g2=0x49U; g3=0x49U; g4=0x36U; break;
        case 'C': g0=0x3EU; g1=0x41U; g2=0x41U; g3=0x41U; g4=0x22U; break;
        case 'D': g0=0x7FU; g1=0x41U; g2=0x41U; g3=0x41U; g4=0x3EU; break;
        case 'E': g0=0x7FU; g1=0x49U; g2=0x49U; g3=0x49U; g4=0x41U; break;
        case 'F': g0=0x7FU; g1=0x09U; g2=0x09U; g3=0x09U; g4=0x01U; break;
        case 'G': g0=0x3EU; g1=0x41U; g2=0x49U; g3=0x49U; g4=0x3AU; break;
        case 'H': g0=0x7FU; g1=0x08U; g2=0x08U; g3=0x08U; g4=0x7FU; break;
        case 'I': g0=0x00U; g1=0x41U; g2=0x7FU; g3=0x41U; g4=0x00U; break;
        case 'J': g0=0x20U; g1=0x40U; g2=0x41U; g3=0x3FU; g4=0x01U; break;
        case 'K': g0=0x7FU; g1=0x08U; g2=0x14U; g3=0x22U; g4=0x41U; break;
        case 'L': g0=0x7FU; g1=0x40U; g2=0x40U; g3=0x40U; g4=0x40U; break;
        case 'M': g0=0x7FU; g1=0x02U; g2=0x04U; g3=0x02U; g4=0x7FU; break;
        case 'N': g0=0x7FU; g1=0x04U; g2=0x08U; g3=0x10U; g4=0x7FU; break;
        case 'O': g0=0x3EU; g1=0x41U; g2=0x41U; g3=0x41U; g4=0x3EU; break;
        case 'P': g0=0x7FU; g1=0x09U; g2=0x09U; g3=0x09U; g4=0x06U; break;
        case 'Q': g0=0x3EU; g1=0x41U; g2=0x51U; g3=0x21U; g4=0x5EU; break;
        case 'R': g0=0x7FU; g1=0x09U; g2=0x19U; g3=0x29U; g4=0x46U; break;
        case 'S': g0=0x26U; g1=0x49U; g2=0x49U; g3=0x49U; g4=0x32U; break;
        case 'T': g0=0x01U; g1=0x01U; g2=0x7FU; g3=0x01U; g4=0x01U; break;
        case 'U': g0=0x3FU; g1=0x40U; g2=0x40U; g3=0x40U; g4=0x3FU; break;
        case 'V': g0=0x1FU; g1=0x20U; g2=0x40U; g3=0x20U; g4=0x1FU; break;
        case 'W': g0=0x3FU; g1=0x40U; g2=0x38U; g3=0x40U; g4=0x3FU; break;
        case 'X': g0=0x63U; g1=0x14U; g2=0x08U; g3=0x14U; g4=0x63U; break;
        case 'Y': g0=0x07U; g1=0x08U; g2=0x70U; g3=0x08U; g4=0x07U; break;
        case 'Z': g0=0x61U; g1=0x51U; g2=0x49U; g3=0x45U; g4=0x43U; break;

        case ':': g2=0x14U; break;
        case '-': g1=0x08U; g2=0x08U; g3=0x08U; break;
        case '.': g2=0x40U; break;
        case '/': g0=0x20U; g1=0x10U; g2=0x08U; g3=0x04U; g4=0x02U; break;
        case '%': g0=0x23U; g1=0x13U; g2=0x08U; g3=0x64U; g4=0x62U; break;
        default:  break; // Space or unsupported character stays blank
    }

    cols[0] = g0;
    cols[1] = g1;
    cols[2] = g2;
    cols[3] = g3;
    cols[4] = g4;
}

void SSD1306_Init(void) {
    // Standard 128x64 SSD1306 wake-up sequence (Horizontal Addressing Mode)
    static const uint8_t init_cmds[] = {
        0xAEU,        // Display OFF
        0x20U, 0x00U, // Memory Addressing Mode: Horizontal
        0xB0U,        // Page Start Address = 0
        0xC8U,        // COM Output Scan Direction: remapped (top-to-bottom)
        0x00U,        // Low column start = 0
        0x10U,        // High column start = 0
        0x40U,        // Start line = 0
        0x81U, 0x7FU, // Contrast = mid-high (0x7F)
        0xA1U,        // Segment Re-map (left-to-right)
        0xA6U,        // Normal display (1 = pixel ON)
        0xA8U, 0x3FU, // Multiplex ratio = 64 (0x3F)
        0xA4U,        // Display follows RAM content
        0xD3U, 0x00U, // Display offset = 0
        0xD5U, 0x80U, // Display clock divide ratio / oscillator freq
        0xD9U, 0x22U, // Pre-charge period
        0xDAU, 0x12U, // COM pins hardware config for 128x64
        0xDBU, 0x20U, // VCOMH deselect level
        0x8DU, 0x14U, // Enable internal charge pump (required for 3.3V modules)
        0xAFU         // Display ON
    };

    for (uint8_t i = 0U; i < (uint8_t)sizeof(init_cmds); i++) {
        ssd1306_cmd(init_cmds[i]);
    }

    SSD1306_Clear();
    SSD1306_UpdateScreen();
}

void SSD1306_Clear(void) {
    for (uint16_t i = 0U; i < SSD1306_BUF_SIZE; i++) {
        ssd1306_buffer[i] = 0x00U;
    }
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t on) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    uint16_t index = (uint16_t)x + ((uint16_t)(y / 8U) * SSD1306_WIDTH);
    if (on) {
        ssd1306_buffer[index] |= (uint8_t)(1U << (y % 8U));
    } else {
        ssd1306_buffer[index] &= (uint8_t)~(1U << (y % 8U));
    }
}

void SSD1306_DrawChar(uint8_t x, uint8_t y, char c) {
    uint8_t cols[5];
    ssd1306_get_glyph(c, cols);

    for (uint8_t col = 0U; col < 5U; col++) {
        for (uint8_t row = 0U; row < 7U; row++) {
            uint8_t pixel_on = (cols[col] >> row) & 0x01U;
            SSD1306_DrawPixel((uint8_t)(x + col), (uint8_t)(y + row), pixel_on);
        }
    }
}

void SSD1306_DrawString(uint8_t x, uint8_t y, const char *str) {
    uint8_t cursor_x = x;
    while (*str) {
        if ((uint16_t)cursor_x + 5U >= SSD1306_WIDTH) {
            break; // Stop before overflowing right edge
        }
        SSD1306_DrawChar(cursor_x, y, *str++);
        cursor_x = (uint8_t)(cursor_x + 6U); // 5px glyph + 1px spacing
    }
}

void SSD1306_UpdateScreen(void) {
    // Reset hardware write window to cover full 128x64 screen
    ssd1306_cmd(0x21U); ssd1306_cmd(0x00U); ssd1306_cmd(0x7FU); // Cols 0..127
    ssd1306_cmd(0x22U); ssd1306_cmd(0x00U); ssd1306_cmd(0x07U); // Pages 0..7

    // Blast entire 1024-byte framebuffer over I2C1 in one burst
    I2C_WriteBuffer(SSD1306_I2C_ADDR, SSD1306_CTRL_DATA, ssd1306_buffer, SSD1306_BUF_SIZE);
}