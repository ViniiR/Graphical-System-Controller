#include <stddef.h>
#include <stdio.h>
#include <systemd/sd-bus.h>

int make_sd_bus_array_of_struct(
    sd_bus_message *p_msg,
    sd_bus_error *p_reterror,
    //
    void **array_ptr,
    size_t array_size,
    //
    const char *inner_struct_type,
    size_t inner_struct_type_size,
    //
    int (*insertor_func_ptr)(sd_bus_message *, const char *, void *)
) {
    char array_of_struct_type[inner_struct_type_size + 3];
    snprintf(
        array_of_struct_type,
        sizeof(array_of_struct_type),
        "(%s)",
        inner_struct_type
    );

    int e = 0;

    sd_bus_message *p_reply_msg = nullptr;
    e = sd_bus_message_new_method_return(p_msg, &p_reply_msg);
    if (e < 0) goto failure;

    e = sd_bus_message_open_container(
        p_reply_msg, SD_BUS_TYPE_ARRAY, array_of_struct_type
    );
    if (e < 0) goto failure;
    for (size_t i = 0; i < array_size; i++) {
        e = sd_bus_message_open_container(
            p_reply_msg, SD_BUS_TYPE_STRUCT, inner_struct_type
        );
        if (e < 0) goto failure;

        e = insertor_func_ptr(p_reply_msg, inner_struct_type, array_ptr[i]);
        if (e < 0) goto failure;

        e = sd_bus_message_close_container(p_reply_msg);
        if (e < 0) goto failure;
    }
    e = sd_bus_message_close_container(p_reply_msg);
    if (e < 0) goto failure;

    int return_value = sd_bus_send(nullptr, p_reply_msg, nullptr);
    if (return_value < 0) goto failure;

    sd_bus_message_unref(p_reply_msg);

    return return_value;

failure:
    sd_bus_message_unref(p_reply_msg);

    return sd_bus_error_setf(
        p_reterror,
        SD_BUS_ERROR_FAILED,
        "Internal error while creating array response"
    );
}
