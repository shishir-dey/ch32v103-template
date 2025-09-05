#include "ch32v10x_exti.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_misc.h"
#include "ch32v10x_rcc.h"
#include "debug.h"

#include "framework/app_framework.h"

volatile uint8_t gpio_int_button_pressed = 0;
volatile uint8_t gpio_int_led_state = 0;

void EXTI4_IRQHandler(void){
    if(EXTI_GetITStatus(EXTI_Line4) != RESET) {
        gpio_int_button_pressed = 1;
        EXTI_ClearITPendingBit(EXTI_Line4);
    }
}

void gpio_interrupt_setup(void){
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    printf("GPIO Interrupt Setup\n");

    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

    // Configure PA4 as input with pull-up (button)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure PC13 as output push-pull (LED)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Initialize LED to OFF
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    // Configure EXTI line
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4);

    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // Configure NVIC
    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void gpio_interrupt_loop(void){
    if(gpio_int_button_pressed) {
        gpio_int_button_pressed = 0;
        gpio_int_led_state = !gpio_int_led_state;

        if(gpio_int_led_state) {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
            printf("GPIO Interrupt: Button pressed, LED ON\n");
        } else {
            GPIO_SetBits(GPIOC, GPIO_Pin_13);   // LED OFF
            printf("GPIO Interrupt: Button pressed, LED OFF\n");
        }

        // Simple debounce delay
        Delay_Ms(200);
    }

    Delay_Ms(10);
}