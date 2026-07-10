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

// 1. 获取当前工作目录
static void abbreviated_cwd(char *output, size_t output_size)
{
    char cwd[PATH_MAX];
    const char *home = getenv("HOME");
    size_t home_length;
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        snprintf(output, output_size, "[cwd unavailable]");
        return;
    }

    if (home != NULL) {
        home_length = strlen(home);
        if (strncmp(cwd, home, home_length) == 0 && (cwd[home_length] == '/' || cwd[home_length] == '\0')) 
        {
            snprintf(output, output_size, "~%s", cwd + home_length);
            return;
        }   
    }

    snprintf(output, output_size, "%s", cwd);  
}

// 2. 绘制状态栏
int tui_init(void) {
    if (initscr() == NULL) {
        return -1; // 初始化失败
    }

    raw();
    noecho();
    keypad(stdscr, TRUE);
    timeout(200);
    curs_set(1);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(PAIR_STATUS, COLOR_BLACK, COLOR_WHITE);
        init_pair(PAIR_TITLE, COLOR_GREEN, -1);
        init_pair(PAIR_BOLD, COLOR_RED, -1);
        init_pair(PAIR_CODE, COLOR_BLUE, -1);
        init_pair(PAIR_PATH, COLOR_GREEN, COLOR_WHITE);
    }
    return 0;
}

// 
void tui_shutdown(void) {
    endwin();
}

//
static const char *role_label(MessageRole role) {
    switch (role) {
        case ROLE_USER:
            return "User";
        case ROLE_ASSISTANT:
            return "Assistant";
        case ROLE_SYSTEM:
        default:
            return "System";
    }
}

static int message_line_count(const Message *message, int width) {
    return 1 + markdown_count_wrapped_lines(message->text, width) + 1;
}

static void draw_dialog(const App *app, int dialog_height, int width) {
    size_t first = app->message_count;
    int used = 0;
    int y = 0;
    size_t i;

    while (first > 0) {
        int needed = message_line_count(&app->messages[first - 1], width);
        if (used + needed > dialog_height) {
            break;
        }
        used += needed;
        first--;
    }

    for (i = first; i < app->message_count && y <  dialog_height; ++i) {
        mvaddnstr(y, 0, role_label(app->messages[i].role), width);
        y++;
        y = markdown_draw_wrapped(stdscr, y, dialog_height, app->messages[i].text, width);
        if (y < dialog_height) {
            y++;
        }
    }

}


static void draw_status(int row, int width)
{
    // 获取当前时间
    char cwd[PATH_MAX];
    char time_text[16];
    time_t now = time(NULL);
    struct tm local_tm;
    int time_x;

    abbreviated_cwd(cwd, sizeof(cwd));
    localtime_r(&now, &local_tm);
    strftime(time_text, sizeof(time_text), "%H:%M:%S", &local_tm);

    attron(COLOR_PAIR(PAIR_STATUS));
    mvhline(row, 0, ' ', width);
    mvaddnstr(row, 0, "Status: ", width);
    attron(COLOR_PAIR(PAIR_PATH) | A_BOLD);
    mvaddnstr(row, 8, cwd, width > 8 ? width - 8 : 0);
    attroff(COLOR_PAIR(PAIR_PATH) | A_BOLD);
    attroff(COLOR_PAIR(PAIR_STATUS));

    time_x = width - (int)strlen(time_text) - 1;
    if (time_x > 0) {
        mvaddstr(row, time_x, time_text);
    }
    attroff(COLOR_PAIR(PAIR_STATUS));
}

// 3. 绘制输入框
static void draw_input(const App *app, int row, int width)
{
    int available = width - 2;
    size_t offset = 0;

    mvhline(row, 0, ' ', width);
    mvhline(row + 1, 0, ' ', width);
    mvhline(row + 2, 0, ' ', width);
    mvaddstr(row, 0, "> ");

    if (available > 0 && app->input_length > (size_t)available) {
        offset = app->input_length - (size_t)available;
    }    
    mvaddnstr(row, 2, app->input + offset, available);
    move(row, 2 + (int)app->input_length - (int)offset);
}

void tui_draw(const App *app) {
    int rows;
    int cols;
    int dialog_height;
    int status_row;
    int input_row;

    getmaxyx(stdscr, rows, cols);
    erase();

    if (rows < 8 || cols < 30) {
        mvaddstr(0, 0, "Terminal too small (minimum 30*8).");
        refresh();
        return;
    }

    dialog_height = rows - STATUS_HEIGHT - INPUT_HEIGHT;
    status_row = dialog_height;
    input_row = status_row + STATUS_HEIGHT;

    draw_dialog(app, dialog_height, cols);
    draw_status(status_row, cols);
    draw_input(app, input_row, cols);
    refresh();
}

// 4. 读取用户输入
int tui_read_key(void)
{
    return getch();
}