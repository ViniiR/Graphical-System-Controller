#include "power_profile.h"
#include "lib/process.h"
#include "lib/result.h"
#include "lib/transform.h"
#include "lib/units.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include <threads.h>

const char POWER_PROFILE_NAME[] = "PowerProfile";
const sd_bus_vtable POWER_PROFILE_VTABLE[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS(
        "GetPowerProfile",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("u", power_profile),
        get_power_profile,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD_WITH_ARGS(
        "SetPowerProfile",
        SD_BUS_ARGS("u", power_profile),
        SD_BUS_NO_RESULT,
        set_power_profile,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

//

typedef enum {
    // When applied, parameters ending in _AC are used
    PERFORMANCE = 0,
    // When applied, parameters ending in _BAT are used
    BALANCED = 1,
    // When applied, parameters ending in _SAV are used, _BAT as fallback
    POWER_SAVER = 2,
} PowerProfile;

static ResultEnum get_profile();
static ResultVoid set_profile(PowerProfile profile);

//

int get_power_profile(
    sd_bus_message *p_msg,
    [[maybe_unused]] void *p_userdata,
    sd_bus_error *p_reterror
) {
    ResultEnum result = get_profile();
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get power profile, Error: %s",
            result.err_msg
        );
    }

    PowerProfile profile = (PowerProfile)result.ok_value;

    return sd_bus_reply_method_return(p_msg, "u", profile);
}
int set_power_profile(
    sd_bus_message *p_msg,
    [[maybe_unused]] void *p_userdata,
    sd_bus_error *p_reterror
) {
    uint32_t value;
    int read_result = sd_bus_message_read(p_msg, "u", &value);
    if (read_result < 0) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_INVALID_ARGS,
            "Expected uint32_t power profile variant"
        );
    }

    switch (value) {
    case PERFORMANCE:
    case BALANCED:
    case POWER_SAVER:
        ResultVoid result = set_profile(value);
        if (result.variant == ERR) {
            return sd_bus_error_setf(
                p_reterror,
                SD_BUS_ERROR_FAILED,
                "Failed to set power profile, Error: %s",
                result.err_msg
            );
        }

        return sd_bus_reply_method_return(p_msg, nullptr);
    }

    return sd_bus_error_setf(
        p_reterror,
        SD_BUS_ERROR_INVALID_ARGS,
        "Invalid input, not within enum bounds: '%u'",
        value
    );
}

//

/// Returns PowerProfiles enum
static ResultEnum get_profile() {
    ResultEnum res = {.variant = ERR, .err_msg = UNKNOWN_ERR, .ok_value = 0};

    PowerProfile profile = PERFORMANCE;

    char output[KIBIBYTE];
    ResultVoid result = exec_command(
        output,
        sizeof(output),
        "tlp-stat -s | grep 'Power profile'"
        "| awk '{split($4, profile, \"/\"); print profile[1]}'",
        "r"
    );
    if (result.variant == ERR) {
        res.err_msg = result.err_msg;
        return res;
    }

    trim_newlines(output);

    if (strcmp(output, "performance") == 0) {
        profile = PERFORMANCE;
    } else if (strcmp(output, "balanced") == 0) {
        profile = BALANCED;
    } else if (strcmp(output, "power-saver") == 0) {
        profile = POWER_SAVER;
    } else {
        res.err_msg = "Unknown power profile";
        return res;
    }

    res.variant = OK;
    res.err_msg = "";
    res.ok_value = profile;
    return res;
}

static ResultVoid set_profile(PowerProfile profile) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    const char *profile_string = nullptr;
    switch (profile) {
    case PERFORMANCE:
        profile_string = "performance";
        break;
    case BALANCED:
        profile_string = "balanced";
        break;
    case POWER_SAVER:
        profile_string = "power-saver";
        break;
    }

    char command[KIBIBYTE];
    snprintf(command, sizeof(command), "tlp %s", profile_string);

    ResultVoid result = exec_command(nullptr, 0, command, "r");
    if (result.variant == ERR) {
        res.err_msg = result.err_msg;
        return res;
    }

    res.variant = OK;
    res.err_msg = "";
    return res;
}
