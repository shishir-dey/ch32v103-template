#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_rtc.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_pwr.h"
#include "ch32v10x_bkp.h"
#include "ch32v10x_misc.h"

volatile uint8_t rtc_second_flag = 0;
volatile uint8_t rtc_alarm_flag = 0;

void RTC_IRQHandler(void) {
    if(RTC_GetITStatus(RTC_IT_SEC) != RESET) {
        rtc_second_flag = 1;
        RTC_ClearITPendingBit(RTC_IT_SEC);
    }
    
    if(RTC_GetITStatus(RTC_IT_ALR) != RESET) {
        rtc_alarm_flag = 1;
        RTC_ClearITPendingBit(RTC_IT_ALR);
    }
}

void rtc_setup(void) {
    NVIC_InitTypeDef NVIC_InitStructure;
    
    printf("RTC Setup\n");
    
    // Enable PWR and BKP clocks
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    
    // Enable access to backup domain
    PWR_BackupAccessCmd(ENABLE);
    
    // Check if RTC is already configured
    if(BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5) {
        printf("RTC: Configuring for first time\n");
        
        // Reset backup domain
        BKP_DeInit();
        
        // Enable LSE oscillator
        RCC_LSEConfig(RCC_LSE_ON);
        while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
        
        // Select LSE as RTC clock source
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        
        // Enable RTC clock
        RCC_RTCCLKCmd(ENABLE);
        
        // Wait for RTC registers synchronization
        RTC_WaitForSynchro();
        
        // Wait until last write operation on RTC registers has finished
        RTC_WaitForLastTask();
        
        // Enable RTC second interrupt
        RTC_ITConfig(RTC_IT_SEC, ENABLE);
        
        // Wait until last write operation on RTC registers has finished
        RTC_WaitForLastTask();
        
        // Set RTC prescaler: set RTC period to 1sec
        // RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
        RTC_SetPrescaler(32767);
        
        // Wait until last write operation on RTC registers has finished
        RTC_WaitForLastTask();
        
        // Set initial time (e.g., 00:00:00)
        RTC_SetCounter(0);
        
        // Wait until last write operation on RTC registers has finished
        RTC_WaitForLastTask();
        
        // Set alarm for 10 seconds from now
        RTC_SetAlarm(10);
        
        // Wait until last write operation on RTC registers has finished
        RTC_WaitForLastTask();
        
        // Enable alarm interrupt
        RTC_ITConfig(RTC_IT_ALR, ENABLE);
        
        // Wait until last write operation on RTC registers has finished
        RTC_WaitForLastTask();
        
        // Write to backup register to indicate RTC is configured
        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
        
        printf("RTC: Configuration complete\n");
    } else {
        printf("RTC: Already configured, waiting for sync\n");
        
        // Wait for RTC registers synchronization
        RTC_WaitForSynchro();
        
        // Enable RTC interrupts
        RTC_ITConfig(RTC_IT_SEC, ENABLE);
        RTC_ITConfig(RTC_IT_ALR, ENABLE);
        
        // Wait until last write operation on RTC registers has finished
        RTC_WaitForLastTask();
    }
    
    // Configure NVIC
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    printf("RTC: Current time = %d seconds\n", (int)RTC_GetCounter());
}

void format_time(uint32_t seconds, uint8_t *hours, uint8_t *minutes, uint8_t *secs) {
    *hours = (seconds / 3600) % 24;
    *minutes = (seconds / 60) % 60;
    *secs = seconds % 60;
}

void rtc_loop(void) {
    static uint32_t last_time = 0;
    uint32_t current_time;
    uint8_t hours, minutes, seconds;
    
    if(rtc_second_flag) {
        rtc_second_flag = 0;
        
        current_time = RTC_GetCounter();
        format_time(current_time, &hours, &minutes, &seconds);
        
        printf("RTC: %02d:%02d:%02d (%d seconds since start)\n", 
               hours, minutes, seconds, (int)current_time);
        
        last_time = current_time;
    }
    
    if(rtc_alarm_flag) {
        rtc_alarm_flag = 0;
        
        current_time = RTC_GetCounter();
        format_time(current_time, &hours, &minutes, &seconds);
        
        printf("RTC: ALARM! Time is %02d:%02d:%02d\n", hours, minutes, seconds);
        
        // Set next alarm for 30 seconds later
        RTC_WaitForLastTask();
        RTC_SetAlarm(current_time + 30);
        RTC_WaitForLastTask();
        
        printf("RTC: Next alarm set for 30 seconds\n");
    }
    
    Delay_Ms(100);
}
