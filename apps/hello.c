#include "framework/app_framework.h"
#include "debug.h"

void setup(void) {
    printf("Hello setup\n");
}

void loop(void) {
    printf("Hello loop\n");
    Delay_Ms(1000);  // Assuming Delay_Ms is available
}

REGISTER_APP(setup, loop);