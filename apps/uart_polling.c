#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_usart.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"

void uart_polling_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    
    printf("UART Polling Setup\n");
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    
    // Configure USART1 Tx (PA9) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure USART1 Rx (PA10) as input floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure USART1
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    // Enable USART1
    USART_Cmd(USART1, ENABLE);
    
    printf("UART Polling: USART1 configured at 9600 baud\n");
}

void uart_send_string(const char* str) {
    while(*str) {
        // Wait for transmit data register to be empty
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        
        // Send character
        USART_SendData(USART1, *str++);
    }
    
    // Wait for transmission complete
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

uint8_t uart_receive_char(void) {
    // Wait for data to be received
    while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
    
    // Read and return received data
    return USART_ReceiveData(USART1);
}

uint8_t uart_data_available(void) {
    return USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET;
}

void uart_polling_loop(void) {
    static uint32_t message_counter = 0;
    static char rx_buffer[64];
    static uint8_t rx_index = 0;
    char tx_message[64];
    
    // Check for received data
    if(uart_data_available()) {
        uint8_t received_char = uart_receive_char();
        
        // Echo received character
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, received_char);
        
        // Store in buffer
        if(received_char == '\r' || received_char == '\n') {
            if(rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                printf("UART Polling: Received: '%s'\n", rx_buffer);
                
                // Send response
                uart_send_string("Echo: ");
                uart_send_string(rx_buffer);
                uart_send_string("\r\n");
                
                rx_index = 0;
            }
        } else if(rx_index < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_index++] = received_char;
        }
    }
    
    // Send periodic message every 5 seconds
    static uint32_t last_send_time = 0;
    if((last_send_time == 0) || ((message_counter - last_send_time) >= 50)) { // 50 * 100ms = 5s
        sprintf(tx_message, "UART Polling Message #%d\r\n", (int)message_counter);
        uart_send_string(tx_message);
        
        printf("UART Polling: Sent message #%d\n", (int)message_counter);
        last_send_time = message_counter;
    }
    
    message_counter++;
    Delay_Ms(100);
}

REGISTER_APP(uart_polling_setup, uart_polling_loop);