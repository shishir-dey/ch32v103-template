#include "ch32v10x_adc.h"
#include "ch32v10x_gpio.h"
#include "ch32v10x_rcc.h"
#include "debug.h"

#include "framework/app_framework.h"

void adc_polling_setup(void){
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    printf("ADC Polling Setup\n");

    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

    // Configure PA0 as analog input
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
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
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);

    // Enable ADC
    ADC_Cmd(ADC1, ENABLE);

    // Calibrate ADC
    ADC_ResetCalibration(ADC1);

    while(ADC_GetResetCalibrationStatus(ADC1));

    ADC_StartCalibration(ADC1);

    while(ADC_GetCalibrationStatus(ADC1));
}

void adc_polling_loop(void){
    uint16_t adc_value;

    // Start conversion
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    // Wait for conversion to complete
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

    // Read value
    adc_value = ADC_GetConversionValue(ADC1);

    printf("ADC Value: %d\n", adc_value);

    Delay_Ms(1000);
}