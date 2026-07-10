#include "command.h"
#include "tools.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *skip_spaces(const char *text)
{
    while (text != NULL &&
           *text != '\0' &&
           isspace((unsigned char)*text)) {
        text++;
    }

    return text;
}

static int parse_positive_int(const char *text, int *value)
{
    char *end;
    long parsed;

    if (text == NULL || *text == '\0' || value == NULL) {
        return 0;
    }

    parsed = strtol(text, &end, 10);

    if (*end != '\0' ||
        parsed <= 0 ||
        parsed > INT_MAX) {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

static void add_formatted(
    App *app,
    MessageRole role,
    const char *prefix,
    const char *body
)
{
    size_t prefix_length;
    size_t body_length;
    char *message;

    if (app == NULL || prefix == NULL || body == NULL) {
        return;
    }

    prefix_length = strlen(prefix);
    body_length = strlen(body);

    message = malloc(prefix_length + body_length + 1);
    if (message == NULL) {
        app_add_message(app, ROLE_SYSTEM, "Out of memory");
        return;
    }

    memcpy(message, prefix, prefix_length);
    memcpy(message + prefix_length, body, body_length + 1);

    app_add_message(app, role, message);
    free(message);
}

static void handle_read(App *app, const char *arguments)
{
    char copy[INPUT_CAPACITY];
    char *file;
    char *range;
    char *extra;
    int has_range = 0;
    int start_line = 1;
    int end_line = 0;
    char summary[512];
    char header[512];
    ReadResult result;

    arguments = skip_spaces(arguments);

    if (arguments == NULL || *arguments == '\0') {
        app_add_message(
            app,
            ROLE_SYSTEM,
            "Usage: /read <file> [<lines>|<start>-<end>]"
        );
        return;
    }

    snprintf(copy, sizeof(copy), "%s", arguments);

    file = strtok(copy, " \t");
    range = strtok(NULL, " \t");
    extra = strtok(NULL, " \t");

    if (file == NULL || extra != NULL) {
        app_add_message(
            app,
            ROLE_SYSTEM,
            "Usage: /read <file> [<lines>|<start>-<end>]"
        );
        return;
    }

    if (range != NULL) {
        char *dash = strchr(range, '-');

        has_range = 1;

        if (dash != NULL) {
            *dash = '\0';

            if (!parse_positive_int(range, &start_line) ||
                !parse_positive_int(dash + 1, &end_line) ||
                start_line > end_line) {
                app_add_message(app, ROLE_SYSTEM, "Invalid line range");
                return;
            }
        } else {
            if (!parse_positive_int(range, &end_line)) {
                app_add_message(app, ROLE_SYSTEM, "Invalid line count");
                return;
            }

            start_line = 1;
        }

        if (end_line - start_line + 1 > 500) {
            app_add_message(
                app,
                ROLE_SYSTEM,
                "Read refused: requested range exceeds 500 lines"
            );
            return;
        }
    }

    if (has_range) {
        snprintf(
            summary,
            sizeof(summary),
            "Tool_Use [read_file] file=%s lines=%d-%d",
            file,
            start_line,
            end_line
        );
    } else {
        snprintf(
            summary,
            sizeof(summary),
            "Tool_Use [read_file] file=%s",
            file
        );
    }

    app_add_message(app, ROLE_ASSISTANT, summary);

    result = tool_read_file(
        file,
        has_range,
        start_line,
        end_line
    );

    if (!result.ok) {
        app_add_message(
            app,
            ROLE_SYSTEM,
            result.text != NULL ? result.text : "Read failed"
        );
        tool_free_read_result(&result);
        return;
    }

    snprintf(
        header,
        sizeof(header),
        "File %s Read success, Line %d-%d\n",
        file,
        result.start_line,
        result.end_line
    );

    add_formatted(
        app,
        ROLE_SYSTEM,
        header,
        result.text != NULL ? result.text : ""
    );

    tool_free_read_result(&result);
}

static void handle_write(App *app, const char *arguments)
{
    const char *cursor;
    const char *space;
    const char *content;
    size_t file_length;
    char file[512];
    char summary[640];
    WriteResult result;

    cursor = skip_spaces(arguments);

    if (cursor == NULL || *cursor == '\0') {
        app_add_message(
            app,
            ROLE_SYSTEM,
            "Usage: /write <file> <content>"
        );
        return;
    }

    space = cursor;

    while (*space != '\0' &&
           !isspace((unsigned char)*space)) {
        space++;
    }

    if (*space == '\0') {
        app_add_message(
            app,
            ROLE_SYSTEM,
            "Usage: /write <file> <content>"
        );
        return;
    }

    file_length = (size_t)(space - cursor);

    if (file_length == 0 || file_length >= sizeof(file)) {
        app_add_message(app, ROLE_SYSTEM, "Invalid file name");
        return;
    }

    memcpy(file, cursor, file_length);
    file[file_length] = '\0';

    content = skip_spaces(space);

    if (content == NULL || *content == '\0') {
        app_add_message(
            app,
            ROLE_SYSTEM,
            "Usage: /write <file> <content>"
        );
        return;
    }

    snprintf(
        summary,
        sizeof(summary),
        "Tool_Use [write_file] file=%s",
        file
    );

    app_add_message(app, ROLE_ASSISTANT, summary);

    result = tool_write_file(file, content);

    app_add_message(
        app,
        ROLE_SYSTEM,
        result.message != NULL ? result.message : "Write failed"
    );

    tool_free_write_result(&result);
}

static void handle_exec(App *app, const char *arguments)
{
    const char *command;
    char summary[INPUT_CAPACITY + 64];
    char result_header[128];
    ExecResult result;

    command = skip_spaces(arguments);

    if (command == NULL || *command == '\0') {
        app_add_message(
            app,
            ROLE_SYSTEM,
            "Usage: /exec <command>"
        );
        return;
    }

    snprintf(
        summary,
        sizeof(summary),
        "Tool_Use [exec_cmd] cmd=%s",
        command
    );

    app_add_message(app, ROLE_ASSISTANT, summary);

    result = tool_exec_command(command, 10, 16 * 1024);

    if (result.timed_out) {
        snprintf(
            result_header,
            sizeof(result_header),
            "Command timeout after 10s\n"
        );
    } else {
        snprintf(
            result_header,
            sizeof(result_header),
            "Command exited with code %d%s\n",
            result.exit_code,
            result.truncated ? " (output truncated)" : ""
        );
    }

    add_formatted(
        app,
        ROLE_SYSTEM,
        result_header,
        result.output != NULL ? result.output : ""
    );

    tool_free_exec_result(&result);
}

void command_handle(App *app, const char *input)
{
    if (app == NULL || input == NULL) {
        return;
    }

    if (strncmp(input, "/read", 5) == 0 &&
        (input[5] == '\0' ||
         isspace((unsigned char)input[5]))) {
        handle_read(app, input + 5);
    } else if (strncmp(input, "/write", 6) == 0 &&
               (input[6] == '\0' ||
                isspace((unsigned char)input[6]))) {
        handle_write(app, input + 6);
    } else if (strncmp(input, "/exec", 5) == 0 &&
               (input[5] == '\0' ||
                isspace((unsigned char)input[5]))) {
        handle_exec(app, input + 5);
    } else {
        app_add_message(
            app,
            ROLE_SYSTEM,
            "Unknown local command"
        );
    }
}
