#ifndef _DBUS_H
#define _DBUS_H

#include <stddef.h>
#include <systemd/sd-bus-protocol.h>

int make_sd_bus_array_of_struct(
    sd_bus_message *p_msg,
    sd_bus_error *p_reterror,
    void **array_ptr,
    size_t array_size,
    const char *inner_struct_type,
    size_t inner_struct_type_size,
    int (*insertor_func_ptr)(sd_bus_message *, const char *, void *)
);

#endif
