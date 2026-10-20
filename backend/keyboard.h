#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <systemd/sd-bus.h>

extern const char KEYBOARD_NAME[];

int toggle_keyboard_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int get_keyboard_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable KEYBOARD_VTABLE[];

#endif
