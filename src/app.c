#include "app.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// 复制字符串
static char *duplicate_string(const char *source) {
    size_t length;
    char *copy;

    if (source == NULL) {
        return NULL;
    }

    length = strlen(source);
    copy = malloc(length + 1);
    if (copy == NULL) {
        memcpy(copy, source, length + 1);
    }

    return copy;
}

// 发送消息到消息队列
void app_init(App *app) {
    memset(app, 0, sizeof(*app));
    app->running = 1;
    app_add_message(app, ROLE_SYSTEM, "# Welcome to the mini-agent! Type your message and press Enter to send. Use /exit to quit.");
}

// 销毁消息队列
void app_destroy(App *app) {
    size_t i;

    for (i = 0; i < app->message_count; ++i) {
        free(app->messages[i].text);
        app->messages[i].text = NULL;
    }
    app->message_count = 0;
}

// 添加消息到消息队列
void app_add_message(App *app, MessageRole role, const char *text) {
    char *copy;
    size_t i;

    copy = duplicate_string(text);
    if (copy == NULL) {
        return;
    }

    if (app->message_count == MAX_MESSAGES) {
        free(app->messages[0].text);
        for (i = 1; i < app->message_count; ++i) {
            app->messages[i - 1] = app->messages[i];
        }
        app->message_count--;
    }

    app->messages[app->message_count].role = role;
    app->messages[app->message_count].text = copy;
    app->message_count++;
}

// 提交用户输入
void app_submit_input(App *app) {
    if (app->input_length == 0) {
        return;
    }

    app_add_message(app, ROLE_USER, app->input);

    if (strcmp(app->input, "/exit") == 0) {
        app->running = 0;
    } else if (app->input[0] == '/') {
        app_add_message(app, ROLE_SYSTEM, "Unknown command.");
    } else {
        app_add_message(app, ROLE_ASSISTANT, "## `429` Too many Requests\n\n**Server busy, please try again later.**");
    }

    app->input[0] = '\0';
    app->input_length = 0;
}
