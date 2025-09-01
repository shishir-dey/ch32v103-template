#include "app_framework.h"
#include <string.h>
#include <stdio.h>

App apps[MAX_APPS];
int num_apps = 0;
int current_app_index = 0;

void register_app(const char *name, void (*setup_func)(void), void (*loop_func)(void)) {
    if (num_apps < MAX_APPS) {
        apps[num_apps].name = name;
        apps[num_apps].setup = setup_func;
        apps[num_apps].loop = loop_func;
        num_apps++;
    }
}

void select_app(int index) {
    if (index >= 0 && index < num_apps) {
        current_app_index = index;
    }
}

void list_apps(void) {
    printf("Available apps:\n");
    for (int i = 0; i < num_apps; i++) {
        printf("%d: %s\n", i, apps[i].name);
    }
    printf("Current app: %d (%s)\n", current_app_index, apps[current_app_index].name);
}

App *get_current_app(void) {
    return &apps[current_app_index];
}