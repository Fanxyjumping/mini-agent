#define _POSIX_C_SOURCE 200809L

#include "tui.h"
#include "markdown.h"
#include "app.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <time.h>

#define INPUT_HEIGHT 3
#define STATUS_HEIGHT 1 
#define PAIR_STATUS 1
#define PAIR_TITLE 2
#define PAIR_BOLD 3
#define PAIR_CODE 4
#define PAIR_PATH 5

static void abbreviated_cwd(char *output, size_t output_size)
{
    char cwd[PATH_MAX];
    const char *home;
    size_t home_length;

    if (output == NULL || output_size == 0) {
        return;
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        snprintf(output, output_size, "[cwd unavailable]");
        return;
    }

    home = getenv("HOME");

    if (home != NULL && *home != '\0') {
        home_length = strlen(home);

        if (strncmp(cwd, home, home_length) == 0 &&
            (cwd[home_length] == '/' ||
             cwd[home_length] == '\0')) {
            snprintf(
                output,
                output_size,
                "~%s",
                cwd + home_length
            );
            return;
        }
    }

    snprintf(output, output_size, "%s", cwd);
}

static const char *role_name(MessageRole role)
{
    switch (role) {
        case ROLE_USER:
            return "User:";
        case ROLE_ASSISTANT:
            return "Assistant:";
        case ROLE_SYSTEM:
            return "System:";
        default:
            return "Unknown:";
    }
}

static int message_line_count(
    const Message *message,
    int width
)
{
    if (message == NULL || width <= 0) {
        return 0;
    }

    return 1 +
           markdown_count_lines(message->text, width) +
           1;
}

static void draw_dialog(
    const App *app,
    int dialog_height,
    int width
)
{
    size_t first;
    size_t i;
    int used = 0;
    int y = 0;

    if (app == NULL ||
        dialog_height <= 0 ||
        width <= 0) {
        return;
    }

    first = app->message_count;

    while (first > 0) {
        int needed = message_line_count(
            &app->messages[first - 1],
            width
        );

        if (used + needed > dialog_height) {
            break;
        }

        used += needed;
        first--;
    }

    for (i = first;
         i < app->message_count && y < dialog_height;
         ++i) {
        const Message *message = &app->messages[i];

        wattron(stdscr, A_BOLD);
        mvwaddnstr(
            stdscr,
            y,
            0,
            role_name(message->role),
            width
        );
        wattroff(stdscr, A_BOLD);
        y++;

        if (y < dialog_height &&
            message->text != NULL) {
            y = markdown_draw(
                stdscr,
                message->text,
                y,
                width,
                dialog_height
            );
        }

        if (y < dialog_height) {
            y++;
        }
    }
}

static void draw_status(int row, int width)
{
    char cwd[PATH_MAX];
    char time_text[16];
    time_t now;
    struct tm local_time;
    int time_length;
    int prefix_length;
    int path_width;
    int time_x;

    abbreviated_cwd(cwd, sizeof(cwd));

    now = time(NULL);

    if (localtime_r(&now, &local_time) == NULL) {
        snprintf(time_text, sizeof(time_text), "--:--:--");
    } else {
        strftime(
            time_text,
            sizeof(time_text),
            "%H:%M:%S",
            &local_time
        );
    }

    wattron(stdscr, COLOR_PAIR(PAIR_STATUS));
    mvwhline(stdscr, row, 0, ' ', width);
    mvwaddnstr(stdscr, row, 0, "Status: ", width);

    prefix_length = 8;
    time_length = (int)strlen(time_text);
    time_x = width - time_length - 1;
    path_width = time_x - prefix_length - 1;

    if (path_width > 0) {
        wattron(
            stdscr,
            COLOR_PAIR(PAIR_PATH) | A_BOLD
        );

        mvwaddnstr(
            stdscr,
            row,
            prefix_length,
            cwd,
            path_width
        );

        wattroff(
            stdscr,
            COLOR_PAIR(PAIR_PATH) | A_BOLD
        );
    }

    if (time_x >= 0) {
        mvwaddnstr(
            stdscr,
            row,
            time_x,
            time_text,
            time_length
        );
    }

    wattroff(stdscr, COLOR_PAIR(PAIR_STATUS));
}

static void draw_input(
    const App *app,
    int input_row,
    int width
)
{
    int available;
    size_t offset = 0;
    size_t visible_length;
    int cursor_x;

    mvwhline(stdscr, input_row, 0, ' ', width);
    mvwhline(stdscr, input_row + 1, 0, ' ', width);
    mvwhline(stdscr, input_row + 2, 0, ' ', width);

    mvwaddnstr(stdscr, input_row, 0, "> ", width);

    available = width - 2;

    if (available <= 0) {
        return;
    }

    if (app->input_length > (size_t)available) {
        offset = app->input_length - (size_t)available;
    }

    visible_length = app->input_length - offset;

    mvwaddnstr(
        stdscr,
        input_row,
        2,
        app->input + offset,
        (int)visible_length
    );

    cursor_x = 2 + (int)visible_length;

    if (cursor_x >= width) {
        cursor_x = width - 1;
    }

    wmove(stdscr, input_row, cursor_x);
}

int tui_init(void)
{
    if (initscr() == NULL) {
        return -1;
    }

    raw();
    noecho();
    keypad(stdscr, TRUE);
    timeout(200);
    curs_set(1);

    if (has_colors()) {
        start_color();
        use_default_colors();

        init_pair(
            PAIR_STATUS,
            COLOR_BLACK,
            COLOR_WHITE
        );
        init_pair(
            PAIR_TITLE,
            COLOR_GREEN,
            -1
        );
        init_pair(
            PAIR_BOLD,
            COLOR_RED,
            -1
        );
        init_pair(
            PAIR_CODE,
            COLOR_BLUE,
            -1
        );
        init_pair(
            PAIR_PATH,
            COLOR_GREEN,
            COLOR_WHITE
        );
    }

    return 0;
}

void tui_shutdown(void)
{
    endwin();
}

int tui_read_key(void)
{
    return getch();
}

void tui_draw(const App *app)
{
    int rows;
    int cols;
    int dialog_height;
    int status_row;
    int input_row;

    getmaxyx(stdscr, rows, cols);
    erase();

    if (rows < 8 || cols < 30) {
        mvaddstr(
            0,
            0,
            "Terminal too small (minimum 30x8)."
        );
        refresh();
        return;
    }

    dialog_height =
        rows - STATUS_HEIGHT - INPUT_HEIGHT;
    status_row = dialog_height;
    input_row = status_row + STATUS_HEIGHT;

    draw_dialog(app, dialog_height, cols);
    draw_status(status_row, cols);
    draw_input(app, input_row, cols);

    refresh();
}
