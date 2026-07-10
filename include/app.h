#ifndef APP_H
#define APP_H

#include <stddef.h>

#define INPUT_CAPACITY 1024
#define MAX_MESSAGES 128

typedef enum {
    ROLE_USER,
    ROLE_ASSISTANT,
    ROLE_SYSTEM
} MessageRole;

typedef struct {
    MessageRole role;
    char *text;
} Message;

typedef struct {
    Message messages[MAX_MESSAGES];
    size_t message_count;

    char input[INPUT_CAPACITY];
    size_t input_length;

    int running;
} App;

void app_init(App *app);
void app_destroy(App *app);
void app_add_message(App *app, MessageRole role, const char *text);
void app_submit_input(App *app);

#endif
