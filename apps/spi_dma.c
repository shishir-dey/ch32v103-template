#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_spi.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_dma.h"
#include "ch32v10x_misc.h"

#define SPI_DMA_BUFFER_SIZE 16

volatile uint8_t tx_dma_buffer[SPI_DMA_BUFFER_SIZE];
volatile uint8_t rx_dma_buffer[SPI_DMA_BUFFER_SIZE];
volatile uint8_t dma_tx_complete = 1;
volatile uint8_t dma_rx_complete = 1;

void DMA1_Channel2_IRQHandler(void) {
    if(DMA_GetITStatus(DMA1_IT_TC2) != RESET) {
        dma_rx_complete = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC2);
    }
}

void DMA1_Channel3_IRQHandler(void) {
    if(DMA_GetITStatus(DMA1_IT_TC3) != RESET) {
        dma_tx_complete = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC3);
        
        // Pull CS high to end transaction
        GPIO_SetBits(GPIOA, GPIO_Pin_4);
    }
}

void spi_dma_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    printf("SPI DMA Setup\n");
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // Configure SPI1 pins
    // PA5 - SCK, PA6 - MISO, PA7 - MOSI
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure PA4 as CS (Chip Select) - manual control
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Set CS high (inactive)
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
    
    // Configure DMA for SPI1 RX (Channel 2)
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rx_dma_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = SPI_DMA_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel2, &DMA_InitStructure);
    
    // Configure DMA for SPI1 TX (Channel 3)
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)tx_dma_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = SPI_DMA_BUFFER_SIZE;
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    
    // Enable DMA interrupts
    DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);
    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
    
    // Configure NVIC for DMA
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    
    // Configure SPI1
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64; // ~1.125MHz
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    
    // Enable SPI1
    SPI_Cmd(SPI1, ENABLE);
    
    printf("SPI DMA: SPI1 configured as master with DMA\n");
}

void spi_dma_transfer(uint8_t* tx_data, uint16_t length) {
    // Wait for previous transfer to complete
    while(!dma_tx_complete || !dma_rx_complete);
    
    if(length > SPI_DMA_BUFFER_SIZE) {
        length = SPI_DMA_BUFFER_SIZE;
    }
    
    // Copy data to transmit buffer
    for(uint16_t i = 0; i < length; i++) {
        tx_dma_buffer[i] = tx_data ? tx_data[i] : 0xFF; // Use 0xFF if no data provided
    }
    
    // Reset completion flags
    dma_tx_complete = 0;
    dma_rx_complete = 0;
    
    // Configure DMA transfer lengths
    DMA_Cmd(DMA1_Channel2, DISABLE);
    DMA_Cmd(DMA1_Channel3, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel2, length);
    DMA_SetCurrDataCounter(DMA1_Channel3, length);
    
    // Pull CS low to start transaction
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    
    // Enable SPI DMA
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    
    // Enable DMA channels
    DMA_Cmd(DMA1_Channel2, ENABLE); // RX
    DMA_Cmd(DMA1_Channel3, ENABLE); // TX
}

void spi_dma_write(uint8_t* data, uint16_t length) {
    spi_dma_transfer(data, length);
}

void spi_dma_read(uint16_t length) {
    spi_dma_transfer(NULL, length); // Send dummy bytes
}

uint8_t spi_dma_is_busy(void) {
    return (!dma_tx_complete || !dma_rx_complete);
}

void spi_dma_loop(void) {
    static uint8_t test_data[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 
                                  0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x00};
    static uint32_t loop_counter = 0;
    static uint8_t operation = 0; // 0 = write, 1 = read
    static uint32_t last_operation_time = 0;
    
    if(!spi_dma_is_busy()) {
        if(operation == 0) {
            // Write operation
            printf("SPI DMA: Loop #%d - Writing %d bytes\n", (int)loop_counter, SPI_DMA_BUFFER_SIZE);
            printf("SPI DMA: TX Data: ");
            for(int i = 0; i < SPI_DMA_BUFFER_SIZE; i++) {
                printf("0x%02X ", test_data[i]);
            }
            printf("\n");
            
            spi_dma_write(test_data, SPI_DMA_BUFFER_SIZE);
            operation = 1;
            last_operation_time = loop_counter;
        } else if((loop_counter - last_operation_time) >= 10) { // Wait a bit after write
            // Read operation
            printf("SPI DMA: Reading %d bytes\n", SPI_DMA_BUFFER_SIZE);
            spi_dma_read(SPI_DMA_BUFFER_SIZE);
            operation = 2;
            last_operation_time = loop_counter;
        }
    } else if(operation == 2 && !spi_dma_is_busy()) {
        // Just completed a read operation
        printf("SPI DMA: RX Data: ");
        for(int i = 0; i < SPI_DMA_BUFFER_SIZE; i++) {
            printf("0x%02X ", rx_dma_buffer[i]);
        }
        printf("\n");
        
        operation = 0;
        loop_counter++;
        
        // Update test data for next iteration
        for(int i = 0; i < SPI_DMA_BUFFER_SIZE; i++) {
            test_data[i]++;
        }
        
        // Disable SPI DMA after transfer
        SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);
        SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    }
    
    Delay_Ms(100);
}

REGISTER_APP(spi_dma_setup, spi_dma_loop);