#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_adc.h"
#include "ch32v10x_rcc.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_dma.h"
#include "ch32v10x_misc.h"

#define ADC_BUFFER_SIZE 10
volatile uint16_t adc_buffer[ADC_BUFFER_SIZE];
volatile uint8_t dma_complete = 0;

void DMA1_Channel1_IRQHandler(void) {
    if(DMA_GetITStatus(DMA1_IT_TC1) != RESET) {
        dma_complete = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC1);
    }
}

void adc_dma_setup(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    printf("ADC DMA Setup\n");
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // Configure PA2 as analog input
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure DMA
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->RDATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = ADC_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    
    // Enable DMA interrupt
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
    
    // Configure NVIC for DMA
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // Configure ADC
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // Configure channel
    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_239Cycles5);
    
    // Enable ADC DMA
    ADC_DMACmd(ADC1, ENABLE);
    
    // Enable DMA
    DMA_Cmd(DMA1_Channel1, ENABLE);
    
    // Enable ADC
    ADC_Cmd(ADC1, ENABLE);
    
    // Calibrate ADC
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    
    // Start continuous conversion
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void adc_dma_loop(void) {
    uint32_t sum = 0;
    uint16_t average;
    
    if(dma_complete) {
        // Calculate average of buffer
        for(int i = 0; i < ADC_BUFFER_SIZE; i++) {
            sum += adc_buffer[i];
        }
        average = sum / ADC_BUFFER_SIZE;
        
        printf("ADC DMA Average: %d\n", average);
        dma_complete = 0;
    }
    
    Delay_Ms(1000);
}

REGISTER_APP(adc_dma_setup, adc_dma_loop);