#include "app.h"
#include "command.h"

#include <stdlib.h>
#include <string.h>

static char *duplicate_string(const char *text)
{
    size_t length;
    char *copy;

    if (text == NULL) {
        return NULL;
    }

    length = strlen(text);
    copy = malloc(length + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length + 1);
    return copy;
}

void app_init(App *app)
{
    if (app == NULL) {
        return;
    }

    memset(app, 0, sizeof(*app));
    app->running = 1;
}

void app_destroy(App *app)
{
    size_t i;

    if (app == NULL) {
        return;
    }

    for (i = 0; i < app->message_count; ++i) {
        free(app->messages[i].text);
        app->messages[i].text = NULL;
    }

    app->message_count = 0;
}

void app_add_message(App *app, MessageRole role, const char *text)
{
    size_t i;
    char *copy;

    if (app == NULL || text == NULL) {
        return;
    }

    copy = duplicate_string(text);
    if (copy == NULL) {
        return;
    }

    if (app->message_count == MAX_MESSAGES) {
        free(app->messages[0].text);

        for (i = 1; i < MAX_MESSAGES; ++i) {
            app->messages[i - 1] = app->messages[i];
        }

        app->message_count--;
    }

    app->messages[app->message_count].role = role;
    app->messages[app->message_count].text = copy;
    app->message_count++;
}

void app_submit_input(App *app)
{
    if (app == NULL || app->input_length == 0) {
        return;
    }

    app_add_message(app, ROLE_USER, app->input);

    if (strcmp(app->input, "/exit") == 0) {
        app->running = 0;
    } else if (app->input[0] == '/') {
        command_handle(app, app->input);
    } else {
        app_add_message(
            app,
            ROLE_ASSISTANT,
            "## `429` Too many Requests\n\n"
            "**Server busy, please try again later.**"
        );
    }

    app->input[0] = '\0';
    app->input_length = 0;
}
