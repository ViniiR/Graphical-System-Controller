#include "brightness.h"
#include "systemd/sd-bus-protocol.h"
#include "types.h"
#include "util.h"
#include <dirent.h>
#include <math.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

const char BRIGHTNESS_PATH[] = "/com/vinii/vgsc/Brightness";
const char BRIGHTNESS_INTERFACE[] = "com.vinii.vgsc.Brightness";

const sd_bus_vtable BRIGHTNESS_VTABLE[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS(
        "SetBrightness",
        SD_BUS_ARGS("u", percentage),
        SD_BUS_NO_RESULT,
        set_brightness_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD_WITH_ARGS(
        "GetBrightness",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("u", percentage),
        get_brightness_handler,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

//

static ResultInt get_brightness();

static ResultVoid set_brightness_all(const unsigned int percent);

//

int set_brightness_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    uint32_t value;
    int res;

    res = sd_bus_message_read(p_msg, "u", &value);
    if (res < 0) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_INVALID_ARGS,
            "Expected uint32_t percentage"
        );
    }

    if (value > 100) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_INVALID_ARGS,
            "Percentage out of bounds '0..=100'"
        );
    }

    ResultVoid result = set_brightness_all(value);
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to set brightness, Error: %s",
            result.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, nullptr);
}

int get_brightness_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
) {
    ResultInt result = get_brightness();
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get brightness, Error: %s",
            result.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, "u", result.ok_value);
}

//

static const char BACKLIGHT_PATH[] = "/sys/class/backlight";
static const char MAX_BRIGHTNESS_PATH[] = "/max_brightness";
static const char CURRENT_BRIGHTNESS_PATH[] = "/brightness";

static ResultHeapString read_brightness_from(
    const char *dir_name, const char *file_name
) {
    ResultHeapString res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = ""
    };

    char filepath[STRING_KB];
    snprintf(
        filepath,
        sizeof(filepath),
        "%s/%s%s",
        BACKLIGHT_PATH,
        dir_name,
        file_name
    );

    ResultHeapString file_result = read_file(filepath);
    if (file_result.variant == ERR) {
        res.err_msg = file_result.err_msg;
        return res;
    }

    char *value = file_result.ok_value;

    res.variant = OK;
    res.ok_value = value;
    res.err_msg = "";
    return res;
}

static ResultInt get_brightness() {
    DIR *dir;
    struct dirent *entry;
    ResultInt res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = 0
    };

    dir = opendir(BACKLIGHT_PATH);
    if (dir == nullptr) {
        res.err_msg = "Failed to open directory";
        return res;
    }

    // Operates on the first file it finds that doesn't start with '.'
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        ResultHeapString result_max_brightness =
            read_brightness_from(entry->d_name, MAX_BRIGHTNESS_PATH);
        if (result_max_brightness.variant == ERR) {
            closedir(dir);
            res.err_msg = result_max_brightness.err_msg;
            return res;
        }
        ResultHeapString result_current_brightness =
            read_brightness_from(entry->d_name, CURRENT_BRIGHTNESS_PATH);
        if (result_current_brightness.variant == ERR) {
            free(result_max_brightness.ok_value);
            closedir(dir);
            res.err_msg = result_current_brightness.err_msg;
            return res;
        }

        ResultInt result_current_brightness_int =
            string_to_int(result_current_brightness.ok_value);
        free(result_current_brightness.ok_value);
        if (result_current_brightness_int.variant == ERR) {
            free(result_max_brightness.ok_value);
            closedir(dir);
            res.err_msg = result_current_brightness_int.err_msg;
            return res;
        }
        ResultInt result_max_brightness_int =
            string_to_int(result_max_brightness.ok_value);
        free(result_max_brightness.ok_value);
        if (result_max_brightness_int.variant == ERR) {
            closedir(dir);
            res.err_msg = result_max_brightness_int.err_msg;
            return res;
        }

        int percent = (result_current_brightness_int.ok_value * 100) /
                      result_max_brightness_int.ok_value;

        closedir(dir);

        res.variant = OK;
        res.ok_value = percent;
        res.err_msg = "";
        return res;
    }
    closedir(dir);

    res.err_msg = "Failed to read backlight directory";
    return res;
}

static ResultVoid set_brightness(
    const unsigned int percent, const char *filepath, const char *max_filepath
) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    ResultHeapString result_max = read_file(max_filepath);
    if (result_max.variant == ERR) {
        res.err_msg = result_max.err_msg;
        return res;
    }

    ResultInt result_max_int = string_to_int(result_max.ok_value);
    free(result_max.ok_value);
    if (result_max_int.variant == ERR) {
        res.err_msg = result_max_int.err_msg;
        return res;
    }

    int calculated_value = ceil((float)percent * result_max_int.ok_value / 100);

    char str[STRING_KB];
    snprintf(str, sizeof(str), "%i", calculated_value);

    ResultVoid result = write_file(filepath, str);
    if (result.variant == ERR) {
        res.err_msg = result.err_msg;
        return res;
    }

    res.variant = OK;
    res.err_msg = "";
    return res;
}
static ResultVoid set_brightness_all(const unsigned int percent) {
    DIR *dir;
    struct dirent *entry;
    ResultVoid res = RESULT_VOID_DEFAULT;

    dir = opendir(BACKLIGHT_PATH);
    if (dir == nullptr) {
        res.err_msg = "Failed to open directory";
        return res;
    }

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char filepath[STRING_KB];
        snprintf(
            filepath,
            sizeof(filepath),
            "%s/%s%s",
            BACKLIGHT_PATH,
            entry->d_name,
            CURRENT_BRIGHTNESS_PATH
        );

        char max_filepath[STRING_KB];
        snprintf(
            max_filepath,
            sizeof(max_filepath),
            "%s/%s%s",
            BACKLIGHT_PATH,
            entry->d_name,
            MAX_BRIGHTNESS_PATH
        );

        ResultVoid result = set_brightness(percent, filepath, max_filepath);
        if (result.variant == ERR) {
            closedir(dir);
            res.err_msg = result.err_msg;
            return res;
        }
    }

    closedir(dir);

    res.variant = OK;
    res.err_msg = "";
    return res;
}
