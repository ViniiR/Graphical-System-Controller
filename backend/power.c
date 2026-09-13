#include "power.h"
#include "systemd/sd-bus-vtable.h"
#include "types.h"
#include "util.h"
#include <systemd/sd-bus.h>

const char POWER_PATH[] = "/com/vinii/vgsc/Power";
const char POWER_INTERFACE[] = "com.vinii.vgsc.Power";

const sd_bus_vtable POWER_VTABLE[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS(
        "Shutdown",
        SD_BUS_NO_ARGS,
        SD_BUS_NO_RESULT,
        shutdown_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD_WITH_ARGS(
        "Reboot",
        SD_BUS_NO_ARGS,
        SD_BUS_NO_RESULT,
        reboot_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD_WITH_ARGS(
        "Logout",
        SD_BUS_NO_ARGS,
        SD_BUS_NO_RESULT,
        logout_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

//
//

int reboot_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    ResultVoid result = exec_command(nullptr, 0, "systemctl reboot", "r");
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to reboot, Error: %s",
            result.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, nullptr);
}
int shutdown_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    ResultVoid result = exec_command(nullptr, 0, "systemctl shutdown", "r");
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to shutdown, Error: %s",
            result.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, nullptr);
}
int logout_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    ResultVoid result =
        exec_command(nullptr, 0, "loginctl terminate-session self", "r");
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to terminate session, Error: %s",
            result.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, nullptr);
}
