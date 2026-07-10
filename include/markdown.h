#ifndef MARKDOWN_H
#define MARKDOWN_H

#include <ncurses.h>

int markdown_count_lines(const char *text, int width);

int markdown_draw(
    WINDOW *window,
    const char *text,
    int start_y,
    int width,
    int max_y
);

#endif
