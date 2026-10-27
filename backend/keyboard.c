#include "keyboard.h"
#include "lib/types.h"
#include "lib/process.h"
#include "lib/transform.h"
#include <string.h>
#include <systemd/sd-bus.h>

const char KEYBOARD_NAME[] = "Keyboard";

const sd_bus_vtable KEYBOARD_VTABLE[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS(
        "ToggleKeyboard",
        SD_BUS_NO_ARGS,
        SD_BUS_NO_RESULT,
        toggle_keyboard_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD_WITH_ARGS(
        "GetKeyboard",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("b", is_standard),
        get_keyboard_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

//

static ResultBool get_is_standard_keyboard();
static ResultVoid toggle_keyboard(bool is_standard);

//

int toggle_keyboard_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    ResultBool result = get_is_standard_keyboard();
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to toggle keyboard, Error: %s",
            result.err_msg
        );
    }

    ResultVoid result_toggle = toggle_keyboard(result.ok_value);
    if (result_toggle.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to toggle keyboard, Error: %s",
            result_toggle.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, nullptr);
}
int get_keyboard_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    ResultBool result = get_is_standard_keyboard();
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get keyboard, Error: %s",
            result.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, "b", result.ok_value);
}

static ResultBool get_is_standard_keyboard() {
    ResultBool res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = false
    };

    bool is_standard = true;

    char output[KIBIBYTE];
    ResultVoid result = exec_command_as_user(
        output,
        sizeof(output),
        "setxkbmap -query | grep \"variant\" | sed \"s/variant:\\s*//\"",
        "r",
        UID
    );
    if (result.variant == ERR) {
        res.err_msg = result.err_msg;
        return res;
    }
    if (strnlen(output, sizeof(output)) == sizeof(output)) {
        res.err_msg = "Failed to get setxkbmap exec_command output";
        return res;
    }
    trim_newlines(output);
    if (strcmp(output, "nodeadkeys") == 0) {
        is_standard = false;
    }

    res.variant = OK;
    res.ok_value = is_standard;
    res.err_msg = "";
    return res;
}

static ResultVoid toggle_keyboard(bool is_standard) {
    static const char *standard_cmd = "setxkbmap -variant \"\"";
    static const char *dev_cmd = "setxkbmap -variant \"nodeadkeys\"";

    ResultVoid res = RESULT_VOID_DEFAULT;

    ResultVoid result = exec_command_as_user(
        nullptr, 0, is_standard ? dev_cmd : standard_cmd, "r", UID
    );
    if (result.variant == ERR) {
        res.err_msg = result.err_msg;
        return res;
    }

    res.variant = OK;
    res.err_msg = "";
    return res;
}
