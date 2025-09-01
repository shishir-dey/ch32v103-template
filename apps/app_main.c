#include "framework/app_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for all app functions
// Basic apps
void setup(void);
void loop(void);

// ADC apps
void adc_polling_setup(void);
void adc_polling_loop(void);
void adc_interrupt_setup(void);
void adc_interrupt_loop(void);
void adc_dma_setup(void);
void adc_dma_loop(void);

// GPIO apps
void gpio_polling_setup(void);
void gpio_polling_loop(void);
void gpio_interrupt_setup(void);
void gpio_interrupt_loop(void);

// I2C apps
void i2c_polling_setup(void);
void i2c_polling_loop(void);
void i2c_interrupt_setup(void);
void i2c_interrupt_loop(void);
void i2c_dma_setup(void);
void i2c_dma_loop(void);

// SPI apps
void spi_polling_setup(void);
void spi_polling_loop(void);
void spi_interrupt_setup(void);
void spi_interrupt_loop(void);
void spi_dma_setup(void);
void spi_dma_loop(void);

// Timer apps
void timer_interrupt_setup(void);
void timer_interrupt_loop(void);
void timer_pwm_setup(void);
void timer_pwm_loop(void);

// UART apps
void uart_polling_setup(void);
void uart_polling_loop(void);
void uart_interrupt_setup(void);
void uart_interrupt_loop(void);
void uart_dma_setup(void);
void uart_dma_loop(void);

// Other apps
void rtc_setup(void);
void rtc_loop(void);
void flash_setup(void);
void flash_loop(void);
void watchdog_setup(void);
void watchdog_loop(void);

// App registration function
// To enable/disable apps, simply comment/uncomment the register_app() lines below
// All apps are compiled but only registered ones will be available at runtime
void register_all_apps(void) {
    // ===========================================
    // BASIC APPS
    // ===========================================
    register_app("Hello World", setup, loop);

    // ===========================================
    // ADC APPS
    // ===========================================
    register_app("ADC Polling", adc_polling_setup, adc_polling_loop);
    register_app("ADC Interrupt", adc_interrupt_setup, adc_interrupt_loop);
    register_app("ADC DMA", adc_dma_setup, adc_dma_loop);

    // ===========================================
    // GPIO APPS
    // ===========================================
    register_app("GPIO Polling", gpio_polling_setup, gpio_polling_loop);
    register_app("GPIO Interrupt", gpio_interrupt_setup, gpio_interrupt_loop);

    // ===========================================
    // I2C APPS
    // ===========================================
    register_app("I2C Polling", i2c_polling_setup, i2c_polling_loop);
    register_app("I2C Interrupt", i2c_interrupt_setup, i2c_interrupt_loop);
    register_app("I2C DMA", i2c_dma_setup, i2c_dma_loop);

    // ===========================================
    // SPI APPS
    // ===========================================
    register_app("SPI Polling", spi_polling_setup, spi_polling_loop);
    register_app("SPI Interrupt", spi_interrupt_setup, spi_interrupt_loop);
    register_app("SPI DMA", spi_dma_setup, spi_dma_loop);

    // ===========================================
    // TIMER APPS
    // ===========================================
    register_app("Timer Interrupt", timer_interrupt_setup, timer_interrupt_loop);
    register_app("Timer PWM", timer_pwm_setup, timer_pwm_loop);

    // ===========================================
    // UART APPS
    // ===========================================
    register_app("UART Polling", uart_polling_setup, uart_polling_loop);
    register_app("UART Interrupt", uart_interrupt_setup, uart_interrupt_loop);
    register_app("UART DMA", uart_dma_setup, uart_dma_loop);

    // ===========================================
    // OTHER APPS
    // ===========================================
    register_app("RTC", rtc_setup, rtc_loop);
    register_app("Flash", flash_setup, flash_loop);
    register_app("Watchdog", watchdog_setup, watchdog_loop);
}

// Main application routine that handles app selection and execution
void app_entry(void) {
    App *current_app;

    // Register all enabled apps
    register_all_apps();

    // List available apps
    list_apps();

    // Select first app by default (index 0)
    select_app(0);
    current_app = get_current_app();

    if (current_app && current_app->setup) {
        current_app->setup();
    }

    // Main application loop
    while(1) {
        if (current_app && current_app->loop) {
            current_app->loop();
        }
    }
}

void app_exit(void) {
    
}

#ifdef __cplusplus
}
#endif