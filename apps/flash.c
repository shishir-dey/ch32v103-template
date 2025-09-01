#include "framework/app_framework.h"
#include "debug.h"
#include "ch32v10x_flash.h"
#include "ch32v10x_rcc.h"

// Flash page size for CH32V103 is typically 1KB (1024 bytes)
#define FLASH_PAGE_SIZE     1024
#define FLASH_TEST_ADDRESS  0x08010000  // Use page at 64KB offset (safe area)
#define FLASH_TEST_DATA_SIZE 256

// Test data pattern
static const uint32_t test_pattern[] = {
    0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0x87654321,
    0xAABBCCDD, 0x11223344, 0x55667788, 0x99AABBCC,
    0xFEDCBA98, 0x13579BDF, 0x2468ACE0, 0x369CF258,
    0x147AD036, 0x258BE147, 0x369CF258, 0x47AE036B
};

void flash_setup(void) {
    printf("Flash Setup\n");
    
    // Flash is always enabled, no need for clock enable
    printf("Flash: Test address = 0x%08X\n", FLASH_TEST_ADDRESS);
    printf("Flash: Test data size = %d bytes\n", FLASH_TEST_DATA_SIZE);
}

FLASH_Status flash_erase_page(uint32_t page_address) {
    FLASH_Status status;
    
    // Unlock flash for erase/program operations
    FLASH_Unlock();
    
    // Erase the page
    status = FLASH_ErasePage(page_address);
    
    // Lock flash
    FLASH_Lock();
    
    return status;
}

FLASH_Status flash_write_data(uint32_t address, uint32_t* data, uint16_t length) {
    FLASH_Status status = FLASH_COMPLETE;
    
    // Unlock flash for program operations
    FLASH_Unlock();
    
    // Write data word by word
    for(uint16_t i = 0; i < length && status == FLASH_COMPLETE; i++) {
        status = FLASH_ProgramWord(address + (i * 4), data[i]);
    }
    
    // Lock flash
    FLASH_Lock();
    
    return status;
}

uint8_t flash_verify_data(uint32_t address, uint32_t* expected_data, uint16_t length) {
    uint32_t* flash_ptr = (uint32_t*)address;
    
    for(uint16_t i = 0; i < length; i++) {
        if(flash_ptr[i] != expected_data[i]) {
            printf("Flash: Verify failed at offset %d: expected 0x%08X, got 0x%08X\n",
                   i, expected_data[i], flash_ptr[i]);
            return 0; // Verification failed
        }
    }
    
    return 1; // Verification successful
}

void flash_read_data(uint32_t address, uint32_t* buffer, uint16_t length) {
    uint32_t* flash_ptr = (uint32_t*)address;
    
    for(uint16_t i = 0; i < length; i++) {
        buffer[i] = flash_ptr[i];
    }
}

void flash_loop(void) {
    static uint32_t loop_counter = 0;
    static uint32_t write_data[64]; // 256 bytes / 4 = 64 words
    static uint32_t read_data[64];
    FLASH_Status flash_status;
    
    printf("Flash: Loop #%d\n", (int)loop_counter);
    
    // Prepare test data (pattern + loop counter for uniqueness)
    for(int i = 0; i < 64; i++) {
        write_data[i] = test_pattern[i % 16] + loop_counter;
    }
    
    // Step 1: Erase the flash page
    printf("Flash: Erasing page at 0x%08X\n", FLASH_TEST_ADDRESS);
    flash_status = flash_erase_page(FLASH_TEST_ADDRESS);
    
    if(flash_status != FLASH_COMPLETE) {
        printf("Flash: Erase failed with status %d\n", flash_status);
        goto next_loop;
    }
    
    printf("Flash: Page erased successfully\n");
    
    // Verify page is erased (should be all 0xFF)
    flash_read_data(FLASH_TEST_ADDRESS, read_data, 64);
    uint8_t erase_ok = 1;
    for(int i = 0; i < 64; i++) {
        if(read_data[i] != 0xFFFFFFFF) {
            printf("Flash: Erase verification failed at word %d: 0x%08X\n", i, read_data[i]);
            erase_ok = 0;
            break;
        }
    }
    
    if(erase_ok) {
        printf("Flash: Erase verification successful\n");
    }
    
    // Step 2: Write test data
    printf("Flash: Writing %d words to flash\n", 64);
    flash_status = flash_write_data(FLASH_TEST_ADDRESS, write_data, 64);
    
    if(flash_status != FLASH_COMPLETE) {
        printf("Flash: Write failed with status %d\n", flash_status);
        goto next_loop;
    }
    
    printf("Flash: Data written successfully\n");
    
    // Step 3: Read back and verify data
    printf("Flash: Reading back data for verification\n");
    flash_read_data(FLASH_TEST_ADDRESS, read_data, 64);
    
    if(flash_verify_data(FLASH_TEST_ADDRESS, write_data, 64)) {
        printf("Flash: Data verification successful!\n");
        
        // Show first few words of data
        printf("Flash: First 8 words: ");
        for(int i = 0; i < 8; i++) {
            printf("0x%08X ", read_data[i]);
        }
        printf("\n");
    } else {
        printf("Flash: Data verification failed!\n");
    }
    
    // Step 4: Test partial read
    printf("Flash: Testing partial read (words 10-15)\n");
    uint32_t partial_data[6];
    flash_read_data(FLASH_TEST_ADDRESS + (10 * 4), partial_data, 6);
    
    printf("Flash: Partial read data: ");
    for(int i = 0; i < 6; i++) {
        printf("0x%08X ", partial_data[i]);
    }
    printf("\n");
    
    // Verify partial read
    uint8_t partial_ok = 1;
    for(int i = 0; i < 6; i++) {
        if(partial_data[i] != write_data[10 + i]) {
            partial_ok = 0;
            break;
        }
    }
    
    printf("Flash: Partial read verification %s\n", partial_ok ? "successful" : "failed");

next_loop:
    loop_counter++;
    
    // Run test every 5 seconds
    Delay_Ms(5000);
}

REGISTER_APP(flash_setup, flash_loop);