#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_spi.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"

void spi_polling_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;
    
    printf("SPI Polling Setup\n");
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
    
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
    
    printf("SPI Polling: SPI1 configured as master\n");
}

uint8_t spi_transfer_byte(uint8_t data) {
    // Wait for transmit buffer to be empty
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    
    // Send data
    SPI_I2S_SendData(SPI1, data);
    
    // Wait for receive buffer to have data
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    
    // Return received data
    return SPI_I2S_ReceiveData(SPI1);
}

void spi_write_bytes(uint8_t* data, uint16_t length) {
    // Pull CS low to start transaction
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    
    for(uint16_t i = 0; i < length; i++) {
        spi_transfer_byte(data[i]);
    }
    
    // Pull CS high to end transaction
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
}

void spi_read_bytes(uint8_t* buffer, uint16_t length) {
    // Pull CS low to start transaction
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    
    for(uint16_t i = 0; i < length; i++) {
        buffer[i] = spi_transfer_byte(0xFF); // Send dummy byte to generate clock
    }
    
    // Pull CS high to end transaction
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
}

void spi_write_read_bytes(uint8_t* tx_data, uint8_t* rx_data, uint16_t length) {
    // Pull CS low to start transaction
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    
    for(uint16_t i = 0; i < length; i++) {
        rx_data[i] = spi_transfer_byte(tx_data[i]);
    }
    
    // Pull CS high to end transaction
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
}

void spi_polling_loop(void) {
    static uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    static uint8_t rx_buffer[5];
    static uint32_t loop_counter = 0;
    
    printf("SPI Polling: Loop #%d\n", (int)loop_counter);
    
    // Test 1: Write only
    printf("SPI Polling: Writing data: ");
    for(int i = 0; i < 5; i++) {
        printf("0x%02X ", test_data[i]);
    }
    printf("\n");
    
    spi_write_bytes(test_data, 5);
    
    Delay_Ms(100);
    
    // Test 2: Read only (sending dummy bytes)
    printf("SPI Polling: Reading data: ");
    spi_read_bytes(rx_buffer, 5);
    
    for(int i = 0; i < 5; i++) {
        printf("0x%02X ", rx_buffer[i]);
    }
    printf("\n");
    
    Delay_Ms(100);
    
    // Test 3: Write and read simultaneously
    printf("SPI Polling: Write/Read simultaneously\n");
    spi_write_read_bytes(test_data, rx_buffer, 5);
    
    printf("SPI Polling: Sent: ");
    for(int i = 0; i < 5; i++) {
        printf("0x%02X ", test_data[i]);
    }
    printf("\n");
    
    printf("SPI Polling: Received: ");
    for(int i = 0; i < 5; i++) {
        printf("0x%02X ", rx_buffer[i]);
    }
    printf("\n");
    
    // Update test data for next iteration
    for(int i = 0; i < 5; i++) {
        test_data[i]++;
    }
    
    loop_counter++;
    Delay_Ms(2000);
}
