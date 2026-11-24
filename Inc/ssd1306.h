#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SSD1306_I2C_ADDR    0x3CU
#define SSD1306_WIDTH       128U
#define SSD1306_HEIGHT      64U

void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t on);
void SSD1306_DrawChar(uint8_t x, uint8_t y, char c);
void SSD1306_DrawString(uint8_t x, uint8_t y, const char *str);
void SSD1306_UpdateScreen(void);

#endif // SSD1306_H