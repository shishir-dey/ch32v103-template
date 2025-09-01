#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_i2c.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"

#define I2C_SLAVE_ADDR 0xA0  // Example EEPROM address

void i2c_polling_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    
    printf("I2C Polling Setup\n");
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    
    // Configure I2C pins (PB6 - SCL, PB7 - SDA)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // Configure I2C
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x30;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);
    
    // Enable I2C
    I2C_Cmd(I2C1, ENABLE);
}

uint8_t i2c_write_byte(uint8_t slave_addr, uint8_t reg_addr, uint8_t data) {
    uint32_t timeout = 0x1000;
    
    // Wait until I2C is not busy
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && timeout--);
    if(timeout == 0) return 1;
    
    // Generate start condition
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout--);
    if(timeout == 0) return 1;
    
    // Send slave address for write
    I2C_Send7bitAddress(I2C1, slave_addr, I2C_Direction_Transmitter);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout--);
    if(timeout == 0) return 1;
    
    // Send register address
    I2C_SendData(I2C1, reg_addr);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout--);
    if(timeout == 0) return 1;
    
    // Send data
    I2C_SendData(I2C1, data);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout--);
    if(timeout == 0) return 1;
    
    // Generate stop condition
    I2C_GenerateSTOP(I2C1, ENABLE);
    
    return 0; // Success
}

uint8_t i2c_read_byte(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data) {
    uint32_t timeout = 0x1000;
    
    // Wait until I2C is not busy
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && timeout--);
    if(timeout == 0) return 1;
    
    // Generate start condition
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout--);
    if(timeout == 0) return 1;
    
    // Send slave address for write
    I2C_Send7bitAddress(I2C1, slave_addr, I2C_Direction_Transmitter);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout--);
    if(timeout == 0) return 1;
    
    // Send register address
    I2C_SendData(I2C1, reg_addr);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout--);
    if(timeout == 0) return 1;
    
    // Generate restart condition
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout--);
    if(timeout == 0) return 1;
    
    // Send slave address for read
    I2C_Send7bitAddress(I2C1, slave_addr, I2C_Direction_Receiver);
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && timeout--);
    if(timeout == 0) return 1;
    
    // Disable ACK and generate stop condition
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);
    
    // Wait for data
    timeout = 0x1000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) && timeout--);
    if(timeout == 0) return 1;
    
    // Read data
    *data = I2C_ReceiveData(I2C1);
    
    // Re-enable ACK
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    
    return 0; // Success
}

void i2c_polling_loop(void) {
    static uint8_t test_data = 0x55;
    uint8_t read_data = 0;
    
    // Try to write data
    if(i2c_write_byte(I2C_SLAVE_ADDR, 0x00, test_data) == 0) {
        printf("I2C Polling: Write successful, data = 0x%02X\n", test_data);
        
        Delay_Ms(10); // Small delay for EEPROM write cycle
        
        // Try to read back data
        if(i2c_read_byte(I2C_SLAVE_ADDR, 0x00, &read_data) == 0) {
            printf("I2C Polling: Read successful, data = 0x%02X\n", read_data);
            
            if(read_data == test_data) {
                printf("I2C Polling: Data verification successful!\n");
            } else {
                printf("I2C Polling: Data verification failed!\n");
            }
        } else {
            printf("I2C Polling: Read failed\n");
        }
    } else {
        printf("I2C Polling: Write failed\n");
    }
    
    test_data++;
    Delay_Ms(2000);
}

REGISTER_APP(i2c_polling_setup, i2c_polling_loop);