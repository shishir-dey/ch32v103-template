#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_adc.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_misc.h"

volatile uint16_t adc_value = 0;
volatile uint8_t conversion_complete = 0;

void ADC1_2_IRQHandler(void) {
    if(ADC_GetITStatus(ADC1, ADC_IT_EOC) != RESET) {
        adc_value = ADC_GetConversionValue(ADC1);
        conversion_complete = 1;
        ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
    }
}

void adc_interrupt_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    printf("ADC Interrupt Setup\n");
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    
    // Configure PA1 as analog input
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure ADC
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // Configure channel
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5);
    
    // Enable ADC interrupt
    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
    
    // Configure NVIC
    NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // Enable ADC
    ADC_Cmd(ADC1, ENABLE);
    
    // Calibrate ADC
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

void adc_interrupt_loop(void) {
    // Start conversion
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    // Wait for interrupt to complete conversion
    while(!conversion_complete);
    
    printf("ADC Interrupt Value: %d\n", adc_value);
    
    conversion_complete = 0;
    Delay_Ms(1000);
}
