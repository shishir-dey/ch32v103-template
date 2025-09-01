#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_tim.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_misc.h"

volatile uint32_t timer_int_counter = 0;
volatile uint8_t timer_int_led_state = 0;

void TIM2_IRQHandler(void) {
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        timer_int_counter++;

        // Toggle LED every second (assuming 1Hz timer)
        timer_int_led_state = !timer_int_led_state;
        if(timer_int_led_state) {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
        } else {
            GPIO_SetBits(GPIOC, GPIO_Pin_13);   // LED OFF
        }

        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

void timer_interrupt_setup(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    printf("Timer Interrupt Setup\n");
    
    // Enable clocks
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // Configure LED pin (PC13)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // Initialize LED to OFF
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
    
    // Configure Timer2 for 1Hz interrupt
    // Assuming system clock is 72MHz, APB1 clock is 36MHz
    // Timer clock = 36MHz
    // For 1Hz: Prescaler = 36000-1, Period = 1000-1
    // Timer frequency = 36MHz / (36000 * 1000) = 1Hz
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 36000 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // Enable Timer2 update interrupt
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // Configure NVIC
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // Enable Timer2
    TIM_Cmd(TIM2, ENABLE);
    
    printf("Timer Interrupt: Timer2 configured for 1Hz interrupt\n");
}

void timer_interrupt_loop(void) {
    static uint32_t last_counter = 0;

    if(timer_int_counter != last_counter) {
        printf("Timer Interrupt: Count = %d, LED = %s\n",
               (int)timer_int_counter, timer_int_led_state ? "ON" : "OFF");
        last_counter = timer_int_counter;
    }

    Delay_Ms(100);
}
