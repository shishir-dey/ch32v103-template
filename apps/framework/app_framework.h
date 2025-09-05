#ifndef APP_FRAMEWORK_H
#define APP_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_APPS 32

typedef struct {
    void (*setup)(void);
    void (*loop)(void);
    const char *name;
} App;

extern App apps[MAX_APPS];
extern int num_apps;
extern int current_app_index;

void register_app(const char *name, void (*setup_func)(void), void (*loop_func)(void));
void select_app(int index);
void list_apps(void);
App *get_current_app(void);

#define REGISTER_APP(name, setup_func, loop_func) \
        void __register_app_ ## setup_func(void) __attribute__((constructor)); \
        void __register_app_ ## setup_func(void){ \
            register_app(name, setup_func, loop_func); \
        }

#ifdef __cplusplus
}
#endif

#endif