#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_spi.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_misc.h"

#define SPI_BUFFER_SIZE 16

volatile uint8_t tx_buffer[SPI_BUFFER_SIZE];
volatile uint8_t rx_buffer[SPI_BUFFER_SIZE];
volatile uint16_t tx_index = 0;
volatile uint16_t rx_index = 0;
volatile uint16_t transfer_length = 0;
volatile uint8_t spi_transfer_complete = 1;

void SPI1_IRQHandler(void) {
    // Handle receive interrupt
    if(SPI_I2S_GetITStatus(SPI1, SPI_I2S_IT_RXNE) != RESET) {
        rx_buffer[rx_index++] = SPI_I2S_ReceiveData(SPI1);
        
        if(rx_index >= transfer_length) {
            // Transfer complete
            SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, DISABLE);
            SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, DISABLE);
            
            // Pull CS high to end transaction
            GPIO_SetBits(GPIOA, GPIO_Pin_4);
            
            spi_transfer_complete = 1;
        }
    }
    
    // Handle transmit interrupt
    if(SPI_I2S_GetITStatus(SPI1, SPI_I2S_IT_TXE) != RESET) {
        if(tx_index < transfer_length) {
            SPI_I2S_SendData(SPI1, tx_buffer[tx_index++]);
        } else {
            // No more data to send, disable TXE interrupt
            SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, DISABLE);
        }
    }
}

void spi_interrupt_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    printf("SPI Interrupt Setup\n");
    
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
    
    // Configure NVIC
    NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // Enable SPI1
    SPI_Cmd(SPI1, ENABLE);
    
    printf("SPI Interrupt: SPI1 configured as master with interrupts\n");
}

void spi_interrupt_transfer(uint8_t* tx_data, uint16_t length) {
    // Wait for previous transfer to complete
    while(!spi_transfer_complete);
    
    if(length > SPI_BUFFER_SIZE) {
        length = SPI_BUFFER_SIZE;
    }
    
    // Copy data to transmit buffer
    for(uint16_t i = 0; i < length; i++) {
        tx_buffer[i] = tx_data ? tx_data[i] : 0xFF; // Use 0xFF if no data provided
    }
    
    // Reset indices and set transfer length
    tx_index = 0;
    rx_index = 0;
    transfer_length = length;
    spi_transfer_complete = 0;
    
    // Pull CS low to start transaction
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    
    // Enable interrupts
    SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, ENABLE);
    SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, ENABLE);
}

void spi_interrupt_write(uint8_t* data, uint16_t length) {
    spi_interrupt_transfer(data, length);
}

void spi_interrupt_read(uint16_t length) {
    spi_interrupt_transfer(NULL, length); // Send dummy bytes
}

void spi_interrupt_loop(void) {
    static uint8_t test_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    static uint32_t loop_counter = 0;
    static uint8_t operation = 0; // 0 = write, 1 = read
    
    if(spi_transfer_complete) {
        printf("SPI Interrupt: Loop #%d\n", (int)loop_counter);
        
        if(operation == 0) {
            // Write operation
            printf("SPI Interrupt: Writing data: ");
            for(int i = 0; i < 8; i++) {
                printf("0x%02X ", test_data[i]);
            }
            printf("\n");
            
            spi_interrupt_write(test_data, 8);
            operation = 1;
        } else {
            // Read operation (from previous write)
            printf("SPI Interrupt: Reading 8 bytes\n");
            spi_interrupt_read(8);
            operation = 0;
            loop_counter++;
            
            // Update test data for next iteration
            for(int i = 0; i < 8; i++) {
                test_data[i]++;
            }
        }
    } else {
        // Check if transfer completed
        if(spi_transfer_complete) {
            if(operation == 0) {
                // Just completed a read operation
                printf("SPI Interrupt: Received data: ");
                for(int i = 0; i < transfer_length; i++) {
                    printf("0x%02X ", rx_buffer[i]);
                }
                printf("\n");
            } else {
                // Just completed a write operation
                printf("SPI Interrupt: Write completed\n");
            }
        }
    }
    
    Delay_Ms(100);
}

REGISTER_APP(spi_interrupt_setup, spi_interrupt_loop);