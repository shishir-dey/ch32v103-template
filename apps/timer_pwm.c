#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_tim.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"

void timer_pwm_setup(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    printf("Timer PWM Setup\n");
    
    // Enable clocks
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    
    // Configure PA6 (TIM3_CH1) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure PA7 (TIM3_CH2) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure Timer3 for PWM
    // Assuming system clock is 72MHz, APB1 clock is 36MHz
    // PWM frequency = 36MHz / (Prescaler+1) / (Period+1)
    // For 1kHz PWM: Prescaler = 35, Period = 999
    // PWM frequency = 36MHz / 36 / 1000 = 1kHz
    TIM_TimeBaseStructure.TIM_Period = 999;
    TIM_TimeBaseStructure.TIM_Prescaler = 35;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // Configure PWM Channel 1
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 250; // 25% duty cycle initially
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    
    // Configure PWM Channel 2
    TIM_OCInitStructure.TIM_Pulse = 500; // 50% duty cycle initially
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    
    // Enable auto-reload preload
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    
    // Enable Timer3
    TIM_Cmd(TIM3, ENABLE);
    
    printf("Timer PWM: TIM3 configured for 1kHz PWM on PA6 and PA7\n");
    printf("Timer PWM: CH1 = 25%% duty cycle, CH2 = 50%% duty cycle\n");
}

void timer_pwm_loop(void) {
    static uint16_t duty_cycle_ch1 = 0;
    static uint16_t duty_cycle_ch2 = 500;
    static int8_t direction_ch1 = 1;
    static int8_t direction_ch2 = -1;
    static uint32_t update_counter = 0;
    
    // Update PWM duty cycles every 50ms to create breathing effect
    update_counter++;
    if(update_counter >= 5) { // 5 * 10ms = 50ms
        update_counter = 0;
        
        // Update Channel 1 (breathing up and down)
        duty_cycle_ch1 += direction_ch1 * 10;
        if(duty_cycle_ch1 >= 999) {
            duty_cycle_ch1 = 999;
            direction_ch1 = -1;
        } else if(duty_cycle_ch1 <= 0) {
            duty_cycle_ch1 = 0;
            direction_ch1 = 1;
        }
        
        // Update Channel 2 (breathing opposite to Channel 1)
        duty_cycle_ch2 += direction_ch2 * 10;
        if(duty_cycle_ch2 >= 999) {
            duty_cycle_ch2 = 999;
            direction_ch2 = -1;
        } else if(duty_cycle_ch2 <= 0) {
            duty_cycle_ch2 = 0;
            direction_ch2 = 1;
        }
        
        // Update PWM compare values
        TIM_SetCompare1(TIM3, duty_cycle_ch1);
        TIM_SetCompare2(TIM3, duty_cycle_ch2);
        
        printf("Timer PWM: CH1 = %d%%, CH2 = %d%%\n", 
               (duty_cycle_ch1 * 100) / 999, 
               (duty_cycle_ch2 * 100) / 999);
    }
    
    Delay_Ms(10);
}

REGISTER_APP(timer_pwm_setup, timer_pwm_loop);