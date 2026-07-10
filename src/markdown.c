#include "markdown.h"
#include <string.h>

#define PAIR_TITLE 2
#define PAIR_BOLD 3
#define PAIR_CODE 4

// 计算文本在给定宽度下的可见宽度
static int visible_width(const char *text) {
    int width = 0;
    size_t i = 0;
    int bold = 0;
    int code = 0;

    while (text[i] != '\0' && text[i] != '\n') {
        if (!code && ((text[i] == '*' && text[i + 1] == '*') || (text[i] == '_' && text[i + 1] == '_'))) {
            bold = !bold;
            (void)bold;
            i += 2;
        } else if (text[i] == '`') {
            code = !code;
            i++;
        } else {
            width++;
            i++;
        }
    }
    return width;
}

// 计算文本在给定宽度下的换行行数
int markdown_count_wrapped_lines(const char *text, int width) {
    int lines = 0;
    const char *line_start = text;
    const char *cursor = text;

    if (width <= 0 || text == NULL || *text == '\0') {
        return 1;
    }

    while (1) {
        if (*cursor == '\n' || *cursor == '\0') {
            int len = (int)(cursor - line_start);
            char buffer[2048];
            int displayed;

            if (len >= (int)sizeof(buffer)) {
                len = (int)sizeof(buffer) - 1;
            }
            memcpy(buffer, line_start, (size_t)len);
            buffer[len] = '\0';
            displayed = visible_width(buffer);
            lines += displayed == 0 ? 1 : (displayed + width - 1) / width;
            
            if (*cursor == '\0') {
                break;
            }
            line_start = cursor + 1;
        }
        cursor++;
    }
    return lines;
}

// 绘制文本，自动换行
static void set_style(WINDOW *win, int title, int bold, int code) {
    wattrset(win, A_NORMAL);
    if (title) {
        wattron(win, COLOR_PAIR(PAIR_TITLE) | A_BOLD);
    } else if (code) {
        wattron(win, COLOR_PAIR(PAIR_CODE));
    } else if (bold) {
        wattron(win, COLOR_PAIR(PAIR_BOLD) | A_BOLD);
    }
}

// 
int markdown_draw_wrapped(WINDOW *win, int start_y, int max_y, const char *text, int width)
{
    int y = start_y;
    int x = 0;
    size_t i = 0;
    int bold = 0;
    int code = 0;
    int line_start = 1;
    int title = 0;

    if (width <= 0) {
        return y;
    }

    while (text[i] != '\0' && y < max_y) {
        if (line_start) {
            title = (text[i] == '#' && (text[i + 1] == ' ' || (text[i + 1] == '#' && text[i + 2] == ' ')) );
            line_start = 0;
            set_style(win, title, bold, code);
        }

        if (text[i] == '\n') {
            y++;
            x = 0;
            i++;
            line_start = 1;
            title = 0;
            continue;
        }

        if (!code && ((text[i] == '*' && text[i + 1] == '*') || (text[i] == '_' && text[i + 1] == '_'))) {
            bold = !bold;
            i += 2;
            set_style(win, title, bold, code);
            continue;
        }

        if (text[i] == '`') {
            code = !code;
            i++;
            set_style(win, title, bold, code);
            continue;
        }

        if (x >= width) {
            y++;
            x = 0;
            if (y >= max_y) {
                break;
            }
        }

        mvwaddch(win, y, x, (unsigned char)text[i]);
        x++;
        i++;
    }

    wattrset(win, A_NORMAL);
    return y + 1;
}