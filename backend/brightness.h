#ifndef _BRIGHTNESS_H
#define _BRIGHTNESS_H

#include <systemd/sd-bus.h>

extern const char BRIGHTNESS_PATH[];
extern const char BRIGHTNESS_INTERFACE[];

int set_brightness_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int get_brightness_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable BRIGHTNESS_VTABLE[];

#endif
