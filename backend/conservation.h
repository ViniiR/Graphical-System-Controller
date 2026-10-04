#ifndef _CONSERVATION_H
#define _CONSERVATION_H

#include <systemd/sd-bus.h>

extern const char CONSERVATION_PATH[];
extern const char CONSERVATION_INTERFACE[];

int toggle_conservation_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int get_conservation_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable CONSERVATION_VTABLE[];

#endif
