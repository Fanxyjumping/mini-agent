#include "markdown.h"
#include <string.h>

#define PAIR_TITLE 2
#define PAIR_BOLD 3
#define PAIR_CODE 4

// 计算文本在给定宽度下的可见宽度
int markdown_count_lines(const char *text, int width)
{
    int lines = 1;
    int x = 0;
    size_t i = 0;
    int bold = 0;
    int code = 0;

    if (text == NULL || width <= 0) {
        return 1;
    }

    while (text[i] != '\0') {
        int marker_length = 0;

        if (!code &&
            ((text[i] == '*' && text[i + 1] == '*') ||
             (text[i] == '_' && text[i + 1] == '_'))) {
            bold = !bold;
            marker_length = 2;
        } else if (text[i] == '`') {
            code = !code;
            marker_length = 1;
        }

        if (marker_length > 0) {
            i += (size_t)marker_length;
            continue;
        }

        if (text[i] == '\n') {
            lines++;
            x = 0;
            i++;
            continue;
        }

        x++;

        if (x >= width) {
            lines++;
            x = 0;
        }

        i++;
    }

    (void)bold;
    return lines;
}

int markdown_draw(
    WINDOW *window,
    const char *text,
    int start_y,
    int width,
    int max_y
)
{
    int y = start_y;
    int x = 0;
    size_t i = 0;
    int bold = 0;
    int code = 0;
    int title_line = 0;

    if (window == NULL || text == NULL || width <= 0) {
        return y;
    }

    while (text[i] != '\0' && y < max_y) {
        if (x == 0) {
            title_line =
                text[i] == '#' &&
                (text[i + 1] == ' ' ||
                 (text[i + 1] == '#' &&
                  text[i + 2] == ' '));
        }

        if (!code &&
            ((text[i] == '*' && text[i + 1] == '*') ||
             (text[i] == '_' && text[i + 1] == '_'))) {
            bold = !bold;
            i += 2;
            continue;
        }

        if (text[i] == '`') {
            code = !code;
            i++;
            continue;
        }

        if (text[i] == '\n') {
            y++;
            x = 0;
            title_line = 0;
            i++;
            continue;
        }

        if (x >= width) {
            y++;
            x = 0;
            title_line = 0;

            if (y >= max_y) {
                break;
            }
        }

        if (title_line) {
            wattron(window, COLOR_PAIR(PAIR_TITLE) | A_BOLD);
        } else if (code) {
            wattron(window, COLOR_PAIR(PAIR_CODE));
        } else if (bold) {
            wattron(window, COLOR_PAIR(PAIR_BOLD) | A_BOLD);
        }

        mvwaddch(window, y, x, (unsigned char)text[i]);

        if (title_line) {
            wattroff(window, COLOR_PAIR(PAIR_TITLE) | A_BOLD);
        } else if (code) {
            wattroff(window, COLOR_PAIR(PAIR_CODE));
        } else if (bold) {
            wattroff(window, COLOR_PAIR(PAIR_BOLD) | A_BOLD);
        }

        x++;
        i++;
    }

    if (x > 0 && y < max_y) {
        y++;
    }

    return y;
}
