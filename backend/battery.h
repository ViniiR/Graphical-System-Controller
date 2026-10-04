#ifndef _BATTERY_H
#define _BATTERY_H

#include <systemd/sd-bus.h>

extern const char BATTERY_PATH[];
extern const char BATTERY_INTERFACE[];

int get_battery_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable BATTERY_VTABLE[];

#endif
