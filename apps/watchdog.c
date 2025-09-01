#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_iwdg.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"

void watchdog_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    printf("Watchdog Setup\n");
    
    // Enable GPIOC clock for LED
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // Configure PC13 as output push-pull (LED)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // Initialize LED to OFF
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
    
    // Configure Independent Watchdog (IWDG)
    // Enable write access to IWDG_PR and IWDG_RLR registers
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    
    // Set IWDG prescaler to 64
    // IWDG counter clock: LSI/64 = 40kHz/64 = 625Hz
    IWDG_SetPrescaler(IWDG_Prescaler_64);
    
    // Set IWDG reload value
    // Timeout = (Reload + 1) / IWDG_CLK
    // For 2 second timeout: Reload = (2 * 625) - 1 = 1249
    IWDG_SetReload(1249);
    
    // Reload IWDG counter
    IWDG_ReloadCounter();
    
    // Enable IWDG (the LSI oscillator will be enabled by hardware)
    IWDG_Enable();
    
    printf("Watchdog: IWDG configured for 2 second timeout\n");
    printf("Watchdog: System will reset if not fed within 2 seconds\n");
}

void watchdog_loop(void) {
    static uint32_t loop_counter = 0;
    static uint8_t led_state = 0;
    static uint32_t last_feed_time = 0;
    static uint8_t simulate_hang = 0;
    
    // Toggle LED to show activity
    led_state = !led_state;
    if(led_state) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
    } else {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);   // LED OFF
    }
    
    printf("Watchdog: Loop #%d, LED = %s\n", 
           (int)loop_counter, led_state ? "ON" : "OFF");
    
    // Simulate a system hang after 20 loops to demonstrate watchdog reset
    if(loop_counter >= 20 && !simulate_hang) {
        printf("Watchdog: Simulating system hang - stopping watchdog feeding\n");
        printf("Watchdog: System should reset in ~2 seconds...\n");
        simulate_hang = 1;
    }
    
    // Feed the watchdog only if not simulating hang
    if(!simulate_hang) {
        // Feed watchdog every loop (every 500ms)
        IWDG_ReloadCounter();
        last_feed_time = loop_counter;
        printf("Watchdog: Fed watchdog (reset timeout)\n");
    } else {
        printf("Watchdog: NOT feeding watchdog (simulating hang)\n");
        printf("Watchdog: Time since last feed: %d loops\n", 
               (int)(loop_counter - last_feed_time));
    }
    
    loop_counter++;
    
    // If we reach here after simulating hang, the watchdog didn't work
    if(loop_counter > 25) {
        printf("Watchdog: ERROR - System should have reset by now!\n");
        simulate_hang = 0; // Reset simulation for next cycle
        loop_counter = 0;
    }
    
    Delay_Ms(500);
}

REGISTER_APP(watchdog_setup, watchdog_loop);