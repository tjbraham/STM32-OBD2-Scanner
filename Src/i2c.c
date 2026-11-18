#include "i2c.h"

void I2C_Init(void) {
    // enable clocks for I2C1 and GPIOB
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // enable I2C1 clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // enable GPIOB clock

    // configure PB6 (SCL) and PB7 (SDA) as alternate function open-drain, 50 MHz
    GPIOB->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6 | GPIO_CRL_MODE7 | GPIO_CRL_CNF7);
    GPIOB->CRL |= (3 << 24) | (3 << 26) | (3 << 28) | (3 << 30);

    // reset I2C1 to clear any stuck bus states
    I2C1->CR1 |= I2C_CR1_SWRST; // set SWRST bit
    I2C1->CR1 &= ~I2C_CR1_SWRST; // clear SWRST bit

    // configure I2C1 timing for 100 kHz
    I2C1->CR2 = 36; // set peripheral clock frequency to 36 MHz
    I2C1->CCR = 180; // 100 kHz standard mode (36 MHz / (2 * 180) = 100 kHz)
    I2C1->TRISE = 37; // standard mode max rise time is 1000ns, (1000ns / (1/36MHz)) + 1 = 37

    // enable I2C1
    I2C1->CR1 |= I2C_CR1_PE; // enable I2C1 peripheral
}

void I2C1_Write(uint8_t device_addr, uint8_t reg_addr, uint8_t data) {
    // Wait until I2C bus is free
    while (I2C1->SR2 & I2C_SR2_BUSY);

    // Generate START condition and wait for completion (SB flag)
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB)); // Wait for Start Bit (SB) flag

    // Send device address (Write mode = address shifted left by 1, LSB is 0)
    I2C1->DR = (device_addr << 1);
    while (!(I2C1->SR1 & I2C_SR1_ADDR)); // Wait for address to be sent (ADDR flag)
    
    // Clear ADDR flag by reading SR1 then SR2
    (void)I2C1->SR1;
    (void)I2C1->SR2; 

    // Send register/control byte
    I2C1->DR = reg_addr;
    while (!(I2C1->SR1 & I2C_SR1_TXE));

    // Send actual data byte
    I2C1->DR = data;
    while (!(I2C1->SR1 & I2C_SR1_TXE));
    
    // Wait for Byte Transfer Finished (BTF)
    while (!(I2C1->SR1 & I2C_SR1_BTF));

    // Generate STOP condition
    I2C1->CR1 |= I2C_CR1_STOP;
}

void I2C1_WriteBuffer(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint16_t size) {
    while (I2C1->SR2 & I2C_SR2_BUSY); // Wait until I2C bus is free

    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB)); // Wait for Start Bit (SB) flag

    I2C1->DR = (device_addr << 1);
    while (!(I2C1->SR1 & I2C_SR1_ADDR)); // Wait for address to be sent (ADDR flag)
    
    (void)I2C1->SR1; // Clear ADDR flag by reading SR1 then SR2
    (void)I2C1->SR2; 

    I2C1->DR = reg_addr;
    while (!(I2C1->SR1 & I2C_SR1_TXE)); // Wait for data register empty (TXE)

    // Send payload array continuously
    for (uint16_t i = 0; i < size; i++) {
        I2C1->DR = data[i];
        while (!(I2C1->SR1 & I2C_SR1_TXE)); // Wait for data register empty (TXE)
    }

    while (!(I2C1->SR1 & I2C_SR1_BTF)); // Wait for Byte Transfer Finished (BTF)
    I2C1->CR1 |= I2C_CR1_STOP;
}