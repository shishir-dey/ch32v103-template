#include "ch32v10x_gpio.h"
#include "ch32v10x_rcc.h"
#include "debug.h"

#include "framework/app_framework.h"

void hello_setup(void){
    GPIO_InitTypeDef GPIO_InitStructure;

    printf("Hello setup - GPIO Blinking\n");

    // Enable GPIOA clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // Configure PA1 and PA2 as output push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Initialize LEDs to OFF
    GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2);
}

void hello_loop(void){
    static uint32_t pattern_counter = 0;
    static uint32_t step = 0;

    // Multiple blinking patterns
    switch(pattern_counter % 4) {
    case 0:     // Alternating blink

        if(step % 2 == 0) {
            GPIO_ResetBits(GPIOA, GPIO_Pin_1);     // PA1 ON
            GPIO_SetBits(GPIOA, GPIO_Pin_2);       // PA2 OFF
        } else {
            GPIO_SetBits(GPIOA, GPIO_Pin_1);       // PA1 OFF
            GPIO_ResetBits(GPIOA, GPIO_Pin_2);     // PA2 ON
        }

        break;

    case 1:     // Both on/off together

        if(step % 2 == 0) {
            GPIO_ResetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2);     // Both ON
        } else {
            GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2);       // Both OFF
        }

        break;

    case 2:     // PA1 fast blink, PA2 slow blink

        if(step % 4 == 0 || step % 4 == 1) {
            GPIO_ResetBits(GPIOA, GPIO_Pin_1);     // PA1 ON
        } else {
            GPIO_SetBits(GPIOA, GPIO_Pin_1);       // PA1 OFF
        }

        if(step % 8 < 4) {
            GPIO_ResetBits(GPIOA, GPIO_Pin_2);     // PA2 ON
        } else {
            GPIO_SetBits(GPIOA, GPIO_Pin_2);       // PA2 OFF
        }

        break;

    case 3:     // Chase pattern

        if(step % 4 == 0) {
            GPIO_ResetBits(GPIOA, GPIO_Pin_1);     // PA1 ON
            GPIO_SetBits(GPIOA, GPIO_Pin_2);       // PA2 OFF
        } else if(step % 4 == 1) {
            GPIO_SetBits(GPIOA, GPIO_Pin_1);       // PA1 OFF
            GPIO_SetBits(GPIOA, GPIO_Pin_2);       // PA2 OFF
        } else if(step % 4 == 2) {
            GPIO_SetBits(GPIOA, GPIO_Pin_1);       // PA1 OFF
            GPIO_ResetBits(GPIOA, GPIO_Pin_2);     // PA2 ON
        } else {
            GPIO_SetBits(GPIOA, GPIO_Pin_1);       // PA1 OFF
            GPIO_SetBits(GPIOA, GPIO_Pin_2);       // PA2 OFF
        }

        break;
    }

    step++;

    if(step >= 16) { // Change pattern every 16 steps
        step = 0;
        pattern_counter++;
        printf("Pattern changed to %d\n", pattern_counter % 4);
    }

    Delay_Ms(250); // 250ms delay for visible blinking
}