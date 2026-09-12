#include "battery.h"
#include "dirent.h"
#include "string.h"
#include "types.h"
#include "util.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

const char BATTERY_PATH[] = "/com/vinii/vgsc/Battery";
const char BATTERY_INTERFACE[] = "com.vinii.vgsc.Battery";

const sd_bus_vtable BATTERY_VTABLE[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS(
        "GetBattery",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("s", icon, "u", percentage),
        get_battery_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

//

static ResultHeapString get_battery_icon();
static ResultInt get_battery_percentage();

//

int get_battery_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    ResultHeapString result_icon = get_battery_icon();
    if (result_icon.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_INVALID_ARGS,
            "Failed to get battery, Error %s",
            result_icon.err_msg
        );
    }
    ResultInt result_percent = get_battery_percentage();
    if (result_percent.variant == ERR) {
        free(result_icon.ok_value);
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_INVALID_ARGS,
            "Failed to get battery, Error %s",
            result_percent.err_msg
        );
    }

    int return_value = sd_bus_reply_method_return(
        p_msg, "su", result_icon.ok_value, result_percent.ok_value
    );

    free(result_icon.ok_value);
    return return_value;
}

//

static const char power_supply[] = "/sys/class/power_supply";
static const char energy_full[] = "/energy_full";
static const char energy_now[] = "/energy_now";
static const char status[] = "/status";

typedef enum { CHARGING, DISCHARGING, PLUGGED_IN } ChargingStatus;

//

static ResultHeapString get_battery_directory() {
    DIR *dir;
    struct dirent *entry;
    ResultHeapString res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = ""
    };

    dir = opendir(power_supply);
    if (dir == NULL) {
        res.err_msg = "Failed to open directory";
        return res;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char start[3 + 1];
        snprintf(start, sizeof(start), "%.3s", entry->d_name);

        if (strcmp(start, "BAT") == 0) {
            closedir(dir);
            char *buf;
            int size = asprintf(&buf, "%s/%s", power_supply, entry->d_name);
            if (size == -1) {
                res.err_msg = "Failed to alloc";
                return res;
            }

            res.variant = OK;
            res.ok_value = buf;
            res.err_msg = "";
            return res;
        }
    }

    closedir(dir);

    res.err_msg = "Failed to read directory";
    return res;
}

static ResultInt get_battery_percentage() {
    ResultInt res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = 0
    };

    ResultHeapString result_directory = get_battery_directory();
    if (result_directory.variant == ERR) {
        res.err_msg = result_directory.err_msg;
        return res;
    }

    char full_capacity[STRING_KB];
    snprintf(
        full_capacity,
        sizeof(full_capacity),
        "%s%s",
        result_directory.ok_value,
        energy_full
    );

    char now_capacity[STRING_KB];
    snprintf(
        now_capacity,
        sizeof(now_capacity),
        "%s%s",
        result_directory.ok_value,
        energy_now
    );
    free(result_directory.ok_value);

    ResultHeapString result_full = read_file(full_capacity);
    if (result_full.variant == ERR) {
        res.err_msg = result_full.err_msg;
        return res;
    }
    ResultInt full_number_result = string_to_int(result_full.ok_value);
    free(result_full.ok_value);
    if (full_number_result.variant == ERR) {
        res.err_msg = full_number_result.err_msg;
        return res;
    }

    ResultHeapString result_now = read_file(now_capacity);
    if (result_now.variant == ERR) {
        res.err_msg = result_now.err_msg;
        return res;
    }
    ResultInt now_number_result = string_to_int(result_now.ok_value);
    free(result_now.ok_value);
    if (now_number_result.variant == ERR) {
        res.err_msg = now_number_result.err_msg;
        return res;
    }

    int value = (int)round(
        ((double)now_number_result.ok_value /
         (double)full_number_result.ok_value) *
        100.0
    );

    res.variant = OK;
    res.ok_value = value;
    res.err_msg = "";
    return res;
}

/// Returns integer of enum ChargingStatus,
/// it is safe to cast ok value to ChargingStatus, unless it is ERR
static ResultInt get_charging_status() {
    ResultInt res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = 0
    };

    ResultHeapString result_dir = get_battery_directory();
    if (result_dir.variant == ERR) {
        res.err_msg = result_dir.err_msg;
        return res;
    }

    char filepath[STRING_KB];
    snprintf(filepath, sizeof(filepath), "%s%s", result_dir.ok_value, status);

    ResultHeapString result = read_file(filepath);
    if (result.variant == ERR) {
        free(result_dir.ok_value);
        res.err_msg = result.err_msg;
        return res;
    }

    ChargingStatus status = CHARGING;
    if (strcmp(result.ok_value, "Not charging") == 0) {
        status = PLUGGED_IN;
    } else if (strcmp(result.ok_value, "Discharging") == 0) {
        status = DISCHARGING;
    }

    free(result_dir.ok_value);
    free(result.ok_value);

    res.variant = OK;
    res.ok_value = status;
    res.err_msg = "";
    return res;
}

static ResultInt format_percentage(const int percentage) {
    ResultInt res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = 0
    };
    int battery_level_formatted = 0;

    if (percentage <= 0) {
        battery_level_formatted = 0;
    } else if (percentage <= 9) {
        battery_level_formatted = percentage;
    } else if (percentage >= 100) {
        battery_level_formatted = 100;
    } else {
        char first[1 + 1];
        snprintf(first, sizeof(first), "%1.0f", (float)percentage);
        ResultInt result_int = string_to_int(first);
        if (result_int.variant == ERR) {
            res.err_msg = result_int.err_msg;
            return res;
        }
        battery_level_formatted = result_int.ok_value * 10;
    }

    res.variant = OK;
    res.err_msg = "";
    res.ok_value = battery_level_formatted;
    return res;
}

static ResultHeapString allocate_suffix(const ChargingStatus status) {
    static const char plugged[] = "-plugged-in";
    static const char charging[] = "-charging";

    ResultHeapString res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = ""
    };

    char *suffix = NULL;
    switch (status) {
    case CHARGING:
        suffix = strdup(charging);
        break;
    case PLUGGED_IN:
        suffix = strdup(plugged);
        break;
    case DISCHARGING:
        suffix = strdup("");
        break;
    }

    if (suffix == NULL) {
        res.err_msg = "Failed to alloc";
        return res;
    }

    res.variant = OK;
    res.ok_value = suffix;
    res.err_msg = "";
    return res;
}

static ResultHeapString get_battery_icon() {
    static const char battery_prefix[] = "battery-level-";

    ResultHeapString res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = ""
    };

    ResultInt charging_status_result = get_charging_status();
    if (charging_status_result.variant == ERR) {
        res.err_msg = charging_status_result.err_msg;
        return res;
    }

    ResultHeapString result_suffix =
        allocate_suffix((ChargingStatus)charging_status_result.ok_value);
    if (result_suffix.variant == ERR) {
        res.err_msg = result_suffix.err_msg;
        return res;
    }

    ResultInt result_percentage = get_battery_percentage();
    if (result_percentage.variant == ERR) {
        free(result_suffix.ok_value);
        res.err_msg = result_percentage.err_msg;
        return res;
    }

    ResultInt battery_level_formatted_result =
        format_percentage(result_percentage.ok_value);
    if (battery_level_formatted_result.variant == ERR) {
        free(result_suffix.ok_value);
        res.err_msg = battery_level_formatted_result.err_msg;
        return res;
    }

    char *buf;
    int size = asprintf(
        &buf,
        "%s%i%s",
        battery_prefix,
        battery_level_formatted_result.ok_value,
        result_suffix.ok_value
    );
    free(result_suffix.ok_value);
    if (size == -1) {
        res.err_msg = "Failed to alloc";
        return res;
    }

    res.variant = OK;
    res.ok_value = buf;
    res.err_msg = "";
    return res;
}
