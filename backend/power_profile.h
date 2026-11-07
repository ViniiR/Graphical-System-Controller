#ifndef _POWER_PROFILE_H
#define _POWER_PROFILE_H

#include <systemd/sd-bus.h>

extern const char POWER_PROFILE_NAME[];

int get_power_profile(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int set_power_profile(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable POWER_PROFILE_VTABLE[];

#endif
