#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_rcc.h"

void gpio_polling_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    printf("GPIO Polling Setup\n");
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);
    
    // Configure PA3 as input with pull-up (button)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure PC13 as output push-pull (LED)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // Initialize LED to OFF
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void gpio_polling_loop(void) {
    static uint8_t button_state = 1;
    static uint8_t last_button_state = 1;
    static uint8_t led_state = 0;
    
    // Read button state (active low)
    button_state = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3);
    
    // Detect button press (falling edge)
    if(last_button_state == 1 && button_state == 0) {
        led_state = !led_state;
        
        if(led_state) {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
            printf("GPIO Polling: Button pressed, LED ON\n");
        } else {
            GPIO_SetBits(GPIOC, GPIO_Pin_13);   // LED OFF
            printf("GPIO Polling: Button pressed, LED OFF\n");
        }
    }
    
    last_button_state = button_state;
    Delay_Ms(50); // Debounce delay
}

REGISTER_APP(gpio_polling_setup, gpio_polling_loop);