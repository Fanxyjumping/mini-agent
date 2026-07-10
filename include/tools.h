#ifndef TOOLS_H
#define TOOLS_H

#include <stddef.h>

typedef struct {
    int ok;
    int start_line;
    int end_line;
    char *text;
} ReadResult;

typedef struct {
    int ok;
    char *message;
} WriteResult;

typedef struct {
    int exit_code;
    int timed_out;
    int truncated;
    char *output;
} ExecResult;

ReadResult tool_read_file(
    const char *path,
    int has_range,
    int start_line,
    int end_line
);

WriteResult tool_write_file(
    const char *path,
    const char *content
);

ExecResult tool_exec_command(
    const char *command,
    int timeout_seconds,
    size_t output_limit
);

void tool_free_read_result(ReadResult *result);
void tool_free_write_result(WriteResult *result);
void tool_free_exec_result(ExecResult *result);

#endif
