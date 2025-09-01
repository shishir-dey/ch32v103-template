#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_usart.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_dma.h"
#include "ch32v10x_misc.h"

#define DMA_BUFFER_SIZE 64

volatile char uart_rx_dma_buffer[DMA_BUFFER_SIZE];
volatile char uart_tx_dma_buffer[DMA_BUFFER_SIZE];
volatile uint8_t uart_dma_rx_complete = 0;
volatile uint8_t uart_dma_tx_complete = 1; // Initially ready to transmit

void DMA1_Channel4_IRQHandler(void) {
    if(DMA_GetITStatus(DMA1_IT_TC4) != RESET) {
        uart_dma_tx_complete = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC4);
    }
}

void DMA1_Channel5_IRQHandler(void) {
    if(DMA_GetITStatus(DMA1_IT_TC5) != RESET) {
        uart_dma_rx_complete = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC5);
    }
}

void uart_dma_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    printf("UART DMA Setup\n");
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // Configure USART1 Tx (PA9) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure USART1 Rx (PA10) as input floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure DMA for USART1 TX (Channel 4)
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uart_tx_dma_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = DMA_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);

    // Configure DMA for USART1 RX (Channel 5)
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uart_rx_dma_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = DMA_BUFFER_SIZE;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    
    // Enable DMA interrupts
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
    
    // Configure NVIC for DMA
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    
    // Configure USART1
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    // Enable USART1 DMA
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
    
    // Enable DMA channels
    DMA_Cmd(DMA1_Channel5, ENABLE); // RX always enabled for circular mode
    
    // Enable USART1
    USART_Cmd(USART1, ENABLE);
    
    printf("UART DMA: USART1 configured at 9600 baud with DMA\n");
}

void uart_dma_send(const char* data, uint16_t length) {
    // Wait for previous transmission to complete
    while(!uart_dma_tx_complete);

    if(length > DMA_BUFFER_SIZE) {
        length = DMA_BUFFER_SIZE;
    }

    // Copy data to DMA buffer
    for(uint16_t i = 0; i < length; i++) {
        uart_tx_dma_buffer[i] = data[i];
    }

    // Configure and start DMA transmission
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel4, length);
    uart_dma_tx_complete = 0;
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

void uart_dma_send_string(const char* str) {
    uint16_t length = 0;
    
    // Calculate string length
    while(str[length] && length < DMA_BUFFER_SIZE - 1) {
        length++;
    }
    
    uart_dma_send(str, length);
}

uint16_t uart_dma_get_received_count(void) {
    return DMA_BUFFER_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5);
}

void uart_dma_loop(void) {
    static uint16_t last_rx_count = 0;
    static uint32_t message_counter = 0;
    char tx_message[64];

    // Check for received data
    uint16_t current_rx_count = uart_dma_get_received_count();

    if(current_rx_count != last_rx_count) {
        // Process new received data
        for(uint16_t i = last_rx_count; i < current_rx_count; i++) {
            char received_char = uart_rx_dma_buffer[i % DMA_BUFFER_SIZE];
            printf("UART DMA: Received char: '%c' (0x%02X)\n",
                   (received_char >= 32 && received_char <= 126) ? received_char : '?',
                   received_char);
        }

        // Echo received data
        if(uart_dma_tx_complete) {
            sprintf(tx_message, "Echo: Received %d chars\r\n",
                    current_rx_count - last_rx_count);
            uart_dma_send_string(tx_message);
        }

        last_rx_count = current_rx_count;

        // Reset if buffer is full
        if(current_rx_count >= DMA_BUFFER_SIZE) {
            DMA_Cmd(DMA1_Channel5, DISABLE);
            DMA_SetCurrDataCounter(DMA1_Channel5, DMA_BUFFER_SIZE);
            DMA_Cmd(DMA1_Channel5, ENABLE);
            last_rx_count = 0;
            printf("UART DMA: RX buffer reset\n");
        }
    }

    // Send periodic message every 5 seconds
    static uint32_t last_send_time = 0;
    if((last_send_time == 0) || ((message_counter - last_send_time) >= 50)) { // 50 * 100ms = 5s
        if(uart_dma_tx_complete) {
            sprintf(tx_message, "UART DMA Message #%d\r\n", (int)message_counter);
            uart_dma_send_string(tx_message);

            printf("UART DMA: Sent message #%d\n", (int)message_counter);
            last_send_time = message_counter;
        }
    }

    message_counter++;
    Delay_Ms(100);
}
