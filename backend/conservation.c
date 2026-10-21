#include "conservation.h"
#include "lib/types.h"
#include "lib/file.h"
#include <stdlib.h>
#include <systemd/sd-bus.h>

const char CONSERVATION_NAME[] = "Conservation";

const sd_bus_vtable CONSERVATION_VTABLE[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS(
        "ToggleConservation",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("b", is_active),
        toggle_conservation_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD_WITH_ARGS(
        "GetConservation",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("b", is_active),
        get_conservation_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};
//

static ResultBool get_is_conservation_active();
static ResultVoid set_conservation_mode(bool value);

//

int toggle_conservation_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    ResultBool result_is_conservation_active = get_is_conservation_active();
    if (result_is_conservation_active.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get conservation, Error: %s",
            result_is_conservation_active.err_msg
        );
    }

    ResultVoid result_set_conservation =
        set_conservation_mode(!result_is_conservation_active.ok_value);
    if (result_set_conservation.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to set conservation, Error: %s",
            result_set_conservation.err_msg
        );
    }

    return sd_bus_reply_method_return(
        p_msg, "b", !result_is_conservation_active.ok_value
    );
}

int get_conservation_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {

    ResultBool result_is_conservation_active = get_is_conservation_active();
    if (result_is_conservation_active.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get conservation, Error: %s",
            result_is_conservation_active.err_msg
        );
    }

    return sd_bus_reply_method_return(
        p_msg, "b", result_is_conservation_active.ok_value
    );
}

//

static const char conservation_filepath[] =
    "/sys/bus/platform/drivers/ideapad_acpi/VPC2004:00/conservation_mode";

static ResultBool get_is_conservation_active() {
    ResultBool res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = false
    };

    bool is_active = false;

    ResultHeapString result_file = read_file(conservation_filepath);
    if (result_file.variant == ERR) {
        res.err_msg = result_file.err_msg;
        return res;
    }
    if (strcmp(result_file.ok_value, "1") == 0) {
        is_active = true;
    }

    free(result_file.ok_value);

    res.variant = OK;
    res.err_msg = "";
    res.ok_value = is_active;
    return res;
}

static ResultVoid set_conservation_mode(bool value) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    ResultVoid result_write =
        write_file(conservation_filepath, value ? "1" : "0");
    if (result_write.variant == ERR) {
        res.err_msg = result_write.err_msg;
        return res;
    }

    res.variant = OK;
    res.err_msg = "";
    return res;
}
