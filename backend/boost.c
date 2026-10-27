#include "boost.h"
#include "lib/file.h"
#include "lib/types.h"
#include <stdlib.h>
#include <systemd/sd-bus.h>

const char BOOST_NAME[] = "Boost";

const sd_bus_vtable BOOST_VTABLE[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS(
        "ToggleBoost",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("b", is_active),
        toggle_boost_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD_WITH_ARGS(
        "GetBoost",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("b", is_active),
        get_boost_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

//

static ResultBool get_is_boost_active();
static ResultVoid set_boost_mode(bool value);

//

int toggle_boost_handler(
    sd_bus_message *p_msg,
    [[maybe_unused]] void *p_userdata,
    sd_bus_error *p_reterror
) {
    ResultBool result_is_active = get_is_boost_active();
    if (result_is_active.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get boost, Error: %s",
            result_is_active.err_msg
        );
    }

    ResultVoid result_set_boost = set_boost_mode(!result_is_active.ok_value);
    if (result_set_boost.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to set boost, Error: %s",
            result_set_boost.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, "b", !result_is_active.ok_value);
}

int get_boost_handler(
    sd_bus_message *p_msg,
    [[maybe_unused]] void *p_userdata,
    sd_bus_error *p_reterror
) {
    ResultBool result_is_active = get_is_boost_active();
    if (result_is_active.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get boost, Error: %s",
            result_is_active.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, "b", result_is_active.ok_value);
}

//

static const char boost_filepath[] = "/sys/devices/system/cpu/cpufreq/boost";

static ResultBool get_is_boost_active() {
    ResultBool res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = false
    };

    ResultHeapString result_file = read_file(boost_filepath);
    if (result_file.variant == ERR) {
        res.err_msg = result_file.err_msg;
        return res;
    }

    bool is_active = false;
    if (strcmp(result_file.ok_value, "1") == 0) {
        is_active = true;
    }

    free(result_file.ok_value);

    res.variant = OK;
    res.ok_value = is_active;
    res.err_msg = "";
    return res;
}

static ResultVoid set_boost_mode(bool value) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    ResultVoid result_write = write_file(boost_filepath, value ? "1" : "0");
    if (result_write.variant == ERR) {
        res.err_msg = result_write.err_msg;
        return res;
    }

    res.variant = OK;
    res.err_msg = "";
    return res;
}
