#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_usart.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_misc.h"

#define RX_BUFFER_SIZE 128
#define TX_BUFFER_SIZE 128

volatile char rx_buffer[RX_BUFFER_SIZE];
volatile char tx_buffer[TX_BUFFER_SIZE];
volatile uint16_t rx_head = 0, rx_tail = 0;
volatile uint16_t tx_head = 0, tx_tail = 0;
volatile uint8_t tx_busy = 0;

void USART1_IRQHandler(void) {
    // Handle receive interrupt
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t received_char = USART_ReceiveData(USART1);
        
        uint16_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
        if(next_head != rx_tail) {
            rx_buffer[rx_head] = received_char;
            rx_head = next_head;
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
    
    // Handle transmit interrupt
    if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
        if(tx_head != tx_tail) {
            USART_SendData(USART1, tx_buffer[tx_tail]);
            tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;
        } else {
            // No more data to send, disable TXE interrupt
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
            tx_busy = 0;
        }
    }
}

void uart_interrupt_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    printf("UART Interrupt Setup\n");
    
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
    
    // Enable USART1 interrupts
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // Configure NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // Enable USART1
    USART_Cmd(USART1, ENABLE);
    
    printf("UART Interrupt: USART1 configured at 9600 baud with interrupts\n");
}

uint8_t uart_rx_available(void) {
    return (rx_head != rx_tail);
}

char uart_read_char(void) {
    if(rx_head == rx_tail) {
        return 0; // No data available
    }
    
    char c = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return c;
}

void uart_send_char(char c) {
    uint16_t next_head = (tx_head + 1) % TX_BUFFER_SIZE;
    
    // Wait if buffer is full
    while(next_head == tx_tail);
    
    tx_buffer[tx_head] = c;
    tx_head = next_head;
    
    // Enable TXE interrupt if not already transmitting
    if(!tx_busy) {
        tx_busy = 1;
        USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    }
}

void uart_send_string(const char* str) {
    while(*str) {
        uart_send_char(*str++);
    }
}

void uart_interrupt_loop(void) {
    static char rx_line_buffer[64];
    static uint8_t rx_line_index = 0;
    static uint32_t message_counter = 0;
    char tx_message[64];
    
    // Process received characters
    while(uart_rx_available()) {
        char received_char = uart_read_char();
        
        // Echo received character
        uart_send_char(received_char);
        
        // Build line buffer
        if(received_char == '\r' || received_char == '\n') {
            if(rx_line_index > 0) {
                rx_line_buffer[rx_line_index] = '\0';
                printf("UART Interrupt: Received: '%s'\n", rx_line_buffer);
                
                // Send response
                uart_send_string("Echo: ");
                uart_send_string(rx_line_buffer);
                uart_send_string("\r\n");
                
                rx_line_index = 0;
            }
        } else if(rx_line_index < sizeof(rx_line_buffer) - 1) {
            rx_line_buffer[rx_line_index++] = received_char;
        }
    }
    
    // Send periodic message every 5 seconds
    static uint32_t last_send_time = 0;
    if((last_send_time == 0) || ((message_counter - last_send_time) >= 50)) { // 50 * 100ms = 5s
        sprintf(tx_message, "UART Interrupt Message #%d\r\n", (int)message_counter);
        uart_send_string(tx_message);
        
        printf("UART Interrupt: Sent message #%d\n", (int)message_counter);
        last_send_time = message_counter;
    }
    
    message_counter++;
    Delay_Ms(100);
}

REGISTER_APP(uart_interrupt_setup, uart_interrupt_loop);