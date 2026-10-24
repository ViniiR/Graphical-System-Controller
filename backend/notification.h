#ifndef _NOTIFICATION_H
#define _NOTIFICATION_H

#include <systemd/sd-bus.h>

extern const char NOTIFICATION_NAME[];

int get_recent_notifications(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable NOTIFICATION_VTABLE[];

#endif
