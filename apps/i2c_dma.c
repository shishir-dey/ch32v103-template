#include "ch32v10x_dma.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_i2c.h"
#include "ch32v10x_misc.h"
#include "ch32v10x_rcc.h"
#include "debug.h"

#include "framework/app_framework.h"

#define I2C_SLAVE_ADDR 0xA0
#define BUFFER_SIZE 8

volatile uint8_t i2c_tx_buffer[BUFFER_SIZE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
volatile uint8_t i2c_rx_buffer[BUFFER_SIZE];
volatile uint8_t i2c_dma_tx_complete = 0;
volatile uint8_t i2c_dma_rx_complete = 0;

void DMA1_Channel6_IRQHandler(void){
    if(DMA_GetITStatus(DMA1_IT_TC6) != RESET) {
        i2c_dma_tx_complete = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC6);
    }
}

void DMA1_Channel7_IRQHandler(void){
    if(DMA_GetITStatus(DMA1_IT_TC7) != RESET) {
        i2c_dma_rx_complete = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC7);
    }
}

void i2c_dma_setup(void){
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    printf("I2C DMA Setup\n");

    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // Configure I2C pins (PB6 - SCL, PB7 - SDA)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure DMA for I2C TX (Channel 6)
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)i2c_tx_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel6, &DMA_InitStructure);

    // Configure DMA for I2C RX (Channel 7)
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)i2c_rx_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE;
    DMA_Init(DMA1_Channel7, &DMA_InitStructure);

    // Enable DMA interrupts
    DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);
    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);

    // Configure NVIC for DMA
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_Init(&NVIC_InitStructure);

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

uint8_t i2c_dma_write(uint8_t slave_addr, uint8_t *data, uint16_t size){
    uint32_t timeout = 0x10000;

    // Wait until I2C is not busy
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && timeout--);

    if(timeout == 0){
        return 1;
    }

    // Reset DMA channel
    DMA_Cmd(DMA1_Channel6, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel6, size);
    i2c_dma_tx_complete = 0;

    // Enable I2C DMA
    I2C_DMACmd(I2C1, ENABLE);

    // Generate start condition
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0x10000;

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout--);

    if(timeout == 0){
        return 1;
    }

    // Send slave address
    I2C_Send7bitAddress(I2C1, slave_addr, I2C_Direction_Transmitter);
    timeout = 0x10000;

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout--);

    if(timeout == 0){
        return 1;
    }

    // Enable DMA
    DMA_Cmd(DMA1_Channel6, ENABLE);

    // Wait for DMA completion
    while(!i2c_dma_tx_complete);

    // Wait for I2C completion
    timeout = 0x10000;

    while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) && timeout--);

    if(timeout == 0){
        return 1;
    }

    // Generate stop condition
    I2C_GenerateSTOP(I2C1, ENABLE);

    // Disable I2C DMA
    I2C_DMACmd(I2C1, DISABLE);

    return 0;
}

uint8_t i2c_dma_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint16_t size){
    uint32_t timeout = 0x10000;

    // First write register address
    if(i2c_dma_write(slave_addr, &reg_addr, 1) != 0) {
        return 1;
    }

    Delay_Ms(1);

    // Wait until I2C is not busy
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && timeout--);

    if(timeout == 0){
        return 1;
    }

    // Reset DMA channel
    DMA_Cmd(DMA1_Channel7, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel7, size);
    i2c_dma_rx_complete = 0;

    // Enable I2C DMA
    I2C_DMACmd(I2C1, ENABLE);

    // Generate start condition
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0x10000;

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout--);

    if(timeout == 0){
        return 1;
    }

    // Send slave address for read
    I2C_Send7bitAddress(I2C1, slave_addr, I2C_Direction_Receiver);
    timeout = 0x10000;

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && timeout--);

    if(timeout == 0){
        return 1;
    }

    // Configure for last byte
    I2C_DMALastTransferCmd(I2C1, ENABLE);

    // Enable DMA
    DMA_Cmd(DMA1_Channel7, ENABLE);

    // Wait for DMA completion
    while(!i2c_dma_rx_complete);

    // Generate stop condition
    I2C_GenerateSTOP(I2C1, ENABLE);

    // Disable I2C DMA
    I2C_DMACmd(I2C1, DISABLE);
    I2C_DMALastTransferCmd(I2C1, DISABLE);

    return 0;
}

void i2c_dma_loop(void){
    static uint8_t operation = 0;

    if(operation == 0) {
        // Write operation
        printf("I2C DMA: Writing %d bytes\n", BUFFER_SIZE);

        if(i2c_dma_write(I2C_SLAVE_ADDR, (uint8_t*)i2c_tx_buffer, BUFFER_SIZE) == 0) {
            printf("I2C DMA: Write successful\n");
        } else {
            printf("I2C DMA: Write failed\n");
        }

        operation = 1;
    } else {
        // Read operation
        printf("I2C DMA: Reading %d bytes\n", BUFFER_SIZE);

        if(i2c_dma_read(I2C_SLAVE_ADDR, 0x00, (uint8_t*)i2c_rx_buffer, BUFFER_SIZE) == 0) {
            printf("I2C DMA: Read successful - ");

            for(int i = 0; i < BUFFER_SIZE; i++) {
                printf("0x%02X ", i2c_rx_buffer[i]);
            }

            printf("\n");
        } else {
            printf("I2C DMA: Read failed\n");
        }

        operation = 0;

        // Update test data
        for(int i = 0; i < BUFFER_SIZE; i++) {
            i2c_tx_buffer[i]++;
        }
    }

    Delay_Ms(2000);
}