#include "app.h"
#include "tui.h"

#include <ncurses.h>

int main(void)
{
    App app;

    app_init(&app);

    if (tui_init() != 0) {
        app_destroy(&app);
        return 1;
    }

    app_add_message(
        &app,
        ROLE_SYSTEM,
        "Mini Agent Terminal ready. "
        "Commands: /read, /write, /exec, /exit"
    );

    while (app.running) {
        int key;

        tui_draw(&app);
        key = tui_read_key();

        if (key == ERR) {
            continue;
        }

        if (key == 3) {
            app.input[0] = '\0';
            app.input_length = 0;
        } else if (key == '\n' ||
                   key == '\r' ||
                   key == KEY_ENTER) {
            app_submit_input(&app);
        } else if (key == KEY_BACKSPACE ||
                   key == 127 ||
                   key == 8) {
            if (app.input_length > 0) {
                app.input_length--;
                app.input[app.input_length] = '\0';
            }
        } else if (key >= 32 &&
                   key <= 126 &&
                   app.input_length + 1 < INPUT_CAPACITY) {
            app.input[app.input_length] = (char)key;
            app.input_length++;
            app.input[app.input_length] = '\0';
        }
    }

    tui_shutdown();
    app_destroy(&app);

    return 0;
}
