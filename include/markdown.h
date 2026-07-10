#ifndef MARKDOWN_H
#define MARKDOWN_H

#include <ncurses.h>

int markdown_count_wrapped_lines(const char *text, int width);
int markdown_draw_wrapped(WINDOW *win, int start_y, int max_y, const char *text, int width);

#endif