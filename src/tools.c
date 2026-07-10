#define _POSIX_C_SOURCE 200809L

#include "tools.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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

static int append_bytes(
    char **buffer,
    size_t *length,
    size_t *capacity,
    const char *data,
    size_t data_length,
    size_t limit,
    int *truncated
)
{
    size_t available;
    size_t required;
    size_t new_capacity;
    char *new_buffer;

    if (buffer == NULL ||
        length == NULL ||
        capacity == NULL ||
        data == NULL) {
        return 0;
    }

    if (*length >= limit) {
        if (truncated != NULL && data_length > 0) {
            *truncated = 1;
        }
        return 1;
    }

    available = limit - *length;

    if (data_length > available) {
        data_length = available;

        if (truncated != NULL) {
            *truncated = 1;
        }
    }

    required = *length + data_length + 1;

    if (required > *capacity) {
        new_capacity = (*capacity == 0) ? 1024 : *capacity;

        while (new_capacity < required) {
            if (new_capacity > (limit + 1) / 2) {
                new_capacity = limit + 1;
                break;
            }

            new_capacity *= 2;
        }

        if (new_capacity > limit + 1) {
            new_capacity = limit + 1;
        }

        new_buffer = realloc(*buffer, new_capacity);

        if (new_buffer == NULL) {
            return 0;
        }

        *buffer = new_buffer;
        *capacity = new_capacity;
    }

    if (data_length > 0) {
        memcpy(*buffer + *length, data, data_length);
        *length += data_length;
    }

    (*buffer)[*length] = '\0';
    return 1;
}

ReadResult tool_read_file(
    const char *path,
    int has_range,
    int start_line,
    int end_line
)
{
    ReadResult result = {0, 0, 0, NULL};
    FILE *file;
    char line[4096];
    char *buffer = NULL;
    size_t length = 0;
    size_t capacity = 0;
    int current_line = 0;
    int first_saved = 0;
    int last_saved = 0;
    int ignored_truncated = 0;

    if (path == NULL || *path == '\0') {
        result.text = duplicate_string("Read failed: empty path");
        return result;
    }

    file = fopen(path, "r");

    if (file == NULL) {
        char message[512];

        snprintf(
            message,
            sizeof(message),
            "File %s Read failed, %s",
            path,
            strerror(errno)
        );

        result.text = duplicate_string(message);
        return result;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        current_line++;

        if (!has_range && current_line > 100) {
            free(buffer);
            fclose(file);

            result.text = duplicate_string(
                "Read refused: file has more than 100 lines; "
                "specify a line count or range"
            );

            return result;
        }

        if (has_range && current_line < start_line) {
            continue;
        }

        if (has_range && current_line > end_line) {
            break;
        }

        if (first_saved == 0) {
            first_saved = current_line;
        }

        last_saved = current_line;

        if (!append_bytes(
                &buffer,
                &length,
                &capacity,
                line,
                strlen(line),
                1024 * 1024,
                &ignored_truncated)) {
            free(buffer);
            fclose(file);

            result.text = duplicate_string(
                "Read failed: out of memory"
            );

            return result;
        }

        if (ignored_truncated) {
            free(buffer);
            fclose(file);

            result.text = duplicate_string(
                "Read refused: selected content exceeds 1MB"
            );

            return result;
        }
    }

    if (ferror(file)) {
        char message[512];

        snprintf(
            message,
            sizeof(message),
            "File %s Read failed, %s",
            path,
            strerror(errno)
        );

        free(buffer);
        fclose(file);

        result.text = duplicate_string(message);
        return result;
    }

    fclose(file);

    if (buffer == NULL) {
        buffer = duplicate_string("");

        if (buffer == NULL) {
            result.text = duplicate_string(
                "Read failed: out of memory"
            );
            return result;
        }
    }

    result.ok = 1;
    result.start_line = first_saved == 0
        ? (has_range ? start_line : 1)
        : first_saved;
    result.end_line = last_saved == 0
        ? result.start_line
        : last_saved;
    result.text = buffer;

    return result;
}

WriteResult tool_write_file(
    const char *path,
    const char *content
)
{
    WriteResult result = {0, NULL};
    FILE *file;
    char message[512];

    if (path == NULL || *path == '\0' || content == NULL) {
        result.message = duplicate_string(
            "Write failed: invalid arguments"
        );
        return result;
    }

    file = fopen(path, "a");

    if (file == NULL) {
        snprintf(
            message,
            sizeof(message),
            "File %s Write failed, %s",
            path,
            strerror(errno)
        );

        result.message = duplicate_string(message);
        return result;
    }

    if (fprintf(file, "%s\n", content) < 0) {
        int saved_errno = errno;

        fclose(file);
        errno = saved_errno;

        snprintf(
            message,
            sizeof(message),
            "File %s Write failed, %s",
            path,
            strerror(errno)
        );

        result.message = duplicate_string(message);
        return result;
    }

    if (fclose(file) != 0) {
        snprintf(
            message,
            sizeof(message),
            "File %s Write failed, %s",
            path,
            strerror(errno)
        );

        result.message = duplicate_string(message);
        return result;
    }

    result.ok = 1;

    snprintf(
        message,
        sizeof(message),
        "File %s Write success",
        path
    );

    result.message = duplicate_string(message);
    return result;
}

static double elapsed_seconds(
    const struct timespec *start,
    const struct timespec *end
)
{
    double seconds;
    double nanoseconds;

    seconds = (double)(end->tv_sec - start->tv_sec);
    nanoseconds =
        (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;

    return seconds + nanoseconds;
}

ExecResult tool_exec_command(
    const char *command,
    int timeout_seconds,
    size_t output_limit
)
{
    ExecResult result = {-1, 0, 0, NULL};
    int pipefd[2];
    pid_t pid;
    int status = 0;
    int child_done = 0;
    int pipe_eof = 0;
    char *output = NULL;
    size_t length = 0;
    size_t capacity = 0;
    struct timespec start_time;

    if (command == NULL || *command == '\0') {
        result.output = duplicate_string("Empty command");
        return result;
    }

    if (pipe(pipefd) == -1) {
        result.output = duplicate_string(strerror(errno));
        return result;
    }

    pid = fork();

    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);

        result.output = duplicate_string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        close(pipefd[0]);

        if (dup2(pipefd[1], STDOUT_FILENO) == -1 ||
            dup2(pipefd[1], STDERR_FILENO) == -1) {
            _exit(126);
        }

        close(pipefd[1]);

        execl(
            "/bin/sh",
            "sh",
            "-c",
            command,
            (char *)NULL
        );

        dprintf(
            STDERR_FILENO,
            "exec failed: %s\n",
            strerror(errno)
        );

        _exit(127);
    }

    close(pipefd[1]);

    {
        int flags = fcntl(pipefd[0], F_GETFL, 0);

        if (flags != -1) {
            (void)fcntl(
                pipefd[0],
                F_SETFL,
                flags | O_NONBLOCK
            );
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0) {
        start_time.tv_sec = 0;
        start_time.tv_nsec = 0;
    }

    while (!child_done || !pipe_eof) {
        char chunk[1024];
        ssize_t count;

        do {
            count = read(pipefd[0], chunk, sizeof(chunk));

            if (count > 0) {
                if (!append_bytes(
                        &output,
                        &length,
                        &capacity,
                        chunk,
                        (size_t)count,
                        output_limit,
                        &result.truncated)) {
                    free(output);
                    output = duplicate_string(
                        "Failed to store command output"
                    );
                    length = output != NULL ? strlen(output) : 0;
                    capacity = length + 1;
                }
            } else if (count == 0) {
                pipe_eof = 1;
            }
        } while (count > 0);

        if (!child_done) {
            pid_t wait_result =
                waitpid(pid, &status, WNOHANG);

            if (wait_result == pid) {
                child_done = 1;
            } else if (wait_result == -1) {
                child_done = 1;
            }
        }

        if (!child_done) {
            struct timespec now;

            if (clock_gettime(CLOCK_MONOTONIC, &now) == 0 &&
                elapsed_seconds(&start_time, &now) >= timeout_seconds) {
                result.timed_out = 1;

                (void)kill(pid, SIGKILL);
                (void)waitpid(pid, &status, 0);

                child_done = 1;
            }
        }

        if ((!child_done || !pipe_eof)) {
            struct timespec pause_time = {
                .tv_sec = 0,
                .tv_nsec = 20 * 1000 * 1000
            };

            nanosleep(&pause_time, NULL);
        }
    }

    close(pipefd[0]);

    if (!result.timed_out) {
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            result.exit_code = 128 + WTERMSIG(status);
        }
    }

    if (output == NULL) {
        output = duplicate_string("");
    }

    result.output = output;
    return result;
}

void tool_free_read_result(ReadResult *result)
{
    if (result == NULL) {
        return;
    }

    free(result->text);
    result->text = NULL;
}

void tool_free_write_result(WriteResult *result)
{
    if (result == NULL) {
        return;
    }

    free(result->message);
    result->message = NULL;
}

void tool_free_exec_result(ExecResult *result)
{
    if (result == NULL) {
        return;
    }

    free(result->output);
    result->output = NULL;
}
