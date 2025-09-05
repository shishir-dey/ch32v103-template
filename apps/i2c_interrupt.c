#include "ch32v10x_gpio.h"
#include "ch32v10x_i2c.h"
#include "ch32v10x_misc.h"
#include "ch32v10x_rcc.h"
#include "debug.h"

#include "framework/app_framework.h"

#define I2C_SLAVE_ADDR 0xA0

volatile uint8_t i2c_state = 0;
volatile uint8_t i2c_data_tx = 0;
volatile uint8_t i2c_data_rx = 0;
volatile uint8_t i2c_operation_complete = 0;
volatile uint8_t i2c_reg_addr = 0;

// I2C states
#define I2C_STATE_IDLE          0
#define I2C_STATE_WRITE_ADDR    1
#define I2C_STATE_WRITE_REG     2
#define I2C_STATE_WRITE_DATA    3
#define I2C_STATE_READ_RESTART  4
#define I2C_STATE_READ_ADDR     5
#define I2C_STATE_READ_DATA     6

void I2C1_EV_IRQHandler(void){
    switch(i2c_state) {
    case I2C_STATE_WRITE_ADDR:

        if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
            I2C_Send7bitAddress(I2C1, I2C_SLAVE_ADDR, I2C_Direction_Transmitter);
            i2c_state = I2C_STATE_WRITE_REG;
        }

        break;

    case I2C_STATE_WRITE_REG:

        if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
            I2C_SendData(I2C1, i2c_reg_addr);
            i2c_state = I2C_STATE_WRITE_DATA;
        }

        break;

    case I2C_STATE_WRITE_DATA:

        if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            I2C_SendData(I2C1, i2c_data_tx);
            i2c_state = I2C_STATE_IDLE;
        }

        break;

    case I2C_STATE_READ_RESTART:

        if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            I2C_GenerateSTART(I2C1, ENABLE);
            i2c_state = I2C_STATE_READ_ADDR;
        }

        break;

    case I2C_STATE_READ_ADDR:

        if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
            I2C_Send7bitAddress(I2C1, I2C_SLAVE_ADDR, I2C_Direction_Receiver);
            i2c_state = I2C_STATE_READ_DATA;
        }

        break;

    case I2C_STATE_READ_DATA:

        if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
            I2C_AcknowledgeConfig(I2C1, DISABLE);
            I2C_GenerateSTOP(I2C1, ENABLE);
        }

        if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
            i2c_data_rx = I2C_ReceiveData(I2C1);
            I2C_AcknowledgeConfig(I2C1, ENABLE);
            i2c_state = I2C_STATE_IDLE;
            i2c_operation_complete = 1;
        }

        break;
    }

    // Check for completion of write operation
    if(i2c_state == I2C_STATE_IDLE && I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        I2C_GenerateSTOP(I2C1, ENABLE);
        i2c_operation_complete = 1;
    }
}

void I2C1_ER_IRQHandler(void){
    if(I2C_GetITStatus(I2C1, I2C_IT_AF) != RESET) {
        I2C_ClearITPendingBit(I2C1, I2C_IT_AF);
        I2C_GenerateSTOP(I2C1, ENABLE);
        i2c_state = I2C_STATE_IDLE;
        i2c_operation_complete = 1;
        printf("I2C Interrupt: NACK received\n");
    }
}

void i2c_interrupt_setup(void){
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    printf("I2C Interrupt Setup\n");

    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    // Configure I2C pins (PB6 - SCL, PB7 - SDA)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure I2C
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x30;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);

    // Enable I2C interrupts
    I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_ERR, ENABLE);

    // Configure NVIC
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&NVIC_InitStructure);

    // Enable I2C
    I2C_Cmd(I2C1, ENABLE);
}

void i2c_write_byte_interrupt(uint8_t reg_addr, uint8_t data){
    i2c_reg_addr = reg_addr;
    i2c_data_tx = data;
    i2c_operation_complete = 0;
    i2c_state = I2C_STATE_WRITE_ADDR;

    I2C_GenerateSTART(I2C1, ENABLE);
}

void i2c_read_byte_interrupt(uint8_t reg_addr){
    i2c_reg_addr = reg_addr;
    i2c_operation_complete = 0;
    i2c_state = I2C_STATE_WRITE_ADDR;

    // First write register address, then restart for read
    I2C_GenerateSTART(I2C1, ENABLE);
}

void i2c_interrupt_loop(void){
    static uint8_t test_data = 0xAA;
    static uint8_t operation = 0; // 0 = write, 1 = read

    if(i2c_state == I2C_STATE_IDLE) {
        if(operation == 0) {
            // Write operation
            printf("I2C Interrupt: Writing data 0x%02X\n", test_data);
            i2c_write_byte_interrupt(0x00, test_data);
            operation = 1;
        } else {
            // Read operation
            printf("I2C Interrupt: Reading data\n");
            i2c_read_byte_interrupt(0x00);
            operation = 0;
            test_data++;
        }
    }

    if(i2c_operation_complete) {
        if(operation == 0) {
            printf("I2C Interrupt: Read complete, data = 0x%02X\n", i2c_data_rx);
        } else {
            printf("I2C Interrupt: Write complete\n");
        }

        i2c_operation_complete = 0;
    }

    Delay_Ms(1000);
}