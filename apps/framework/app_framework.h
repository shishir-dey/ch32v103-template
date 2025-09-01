#ifndef APP_FRAMEWORK_H
#define APP_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*setup)(void);
    void (*loop)(void);
} App;

extern App current_app;

#define REGISTER_APP(setup_func, loop_func) App current_app = {setup_func, loop_func}

#ifdef __cplusplus
}
#endif

#endif