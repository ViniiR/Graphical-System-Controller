#ifndef _BOOST_H
#define _BOOST_H

#include <systemd/sd-bus.h>

extern const char BOOST_PATH[];
extern const char BOOST_INTERFACE[];

int toggle_boost_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int get_boost_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable BOOST_VTABLE[];

#endif
