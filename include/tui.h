#ifndef TUI_H
#define TUI_H

#include "app.h"

int tui_init(void);
void tui_shutdown(void);
void tui_draw(const App *app);
int tui_read_key(void);

#endif