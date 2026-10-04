#ifndef _POWER_H
#define _POWER_H

#include <systemd/sd-bus.h>

extern const char POWER_PATH[];
extern const char POWER_INTERFACE[];

int reboot_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int shutdown_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int logout_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable POWER_VTABLE[];

#endif
