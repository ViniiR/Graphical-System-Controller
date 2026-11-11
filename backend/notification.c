#include "notification.h"
#include "lib/dbus.h"
#include "lib/process.h"
#include "lib/result.h"
#include "lib/transform.h"
#include "lib/units.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-bus.h>

const char NOTIFICATION_NAME[] = "Notification";

const sd_bus_vtable NOTIFICATION_VTABLE[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS(
        "GetRecentNotifications",
        SD_BUS_NO_ARGS,
        SD_BUS_RESULT("a(ssssx)", array),
        get_recent_notifications,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD_WITH_ARGS(
        "ClearNotifications",
        SD_BUS_NO_ARGS,
        SD_BUS_NO_RESULT,
        clear_notifications,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

//

typedef struct {
    char *body;
    char *app;
    char *icon;
    char *summary;
    int64_t timestamp;
} Notification;

static const unsigned int MAX_NOTIFICATIONS = 20;

//

static ResultVoid get_notifications(
    Notification **array, size_t *size, size_t max_size
);

//

int sd_bus_array_append_notification(
    sd_bus_message *p_reply_msg, const char *struct_types, void *struct_ptr
) {
    Notification *notif = (Notification *)struct_ptr;
    int result = sd_bus_message_append(
        p_reply_msg,
        struct_types,
        notif->body,
        notif->app,
        notif->icon,
        notif->summary,
        notif->timestamp
    );
    return result;
}

static void free_notification_array(Notification *(*array)[MAX_NOTIFICATIONS]) {
    for (size_t i = 0; i < MAX_NOTIFICATIONS; i++) {
        Notification *ptr = (*array)[i];
        free(ptr->app);
        free(ptr->body);
        free(ptr->icon);
        free(ptr->summary);
        // don't forget that timestamp is pass-by-value
        free(ptr);
    }
}

int get_recent_notifications(
    sd_bus_message *p_msg,
    [[maybe_unused]] void *p_userdata,
    sd_bus_error *p_reterror
) {
    Notification *output
        [[gnu::cleanup(free_notification_array)]][MAX_NOTIFICATIONS];
    size_t output_size = 0;

    ResultVoid result = get_notifications(output, &output_size, sizeof(output));
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get notifications, Error: %s",
            result.err_msg
        );
    }

    Notification **notif_arr_decay = output;
    int return_value = make_sd_bus_array_of_struct(
        p_msg,
        p_reterror,
        (void **)(void *)notif_arr_decay,
        output_size,
        "ssssx",
        6,
        &sd_bus_array_append_notification
    );

    return return_value;
}
int clear_notifications(
    sd_bus_message *p_msg,
    [[maybe_unused]] void *p_userdata,
    sd_bus_error *p_reterror
) {
    ResultVoid result =
        exec_command_as_user(nullptr, 0, "dunstctl history-clear", "r", UID);
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to clear notifications, Error: %s",
            result.err_msg
        );
    }

    return sd_bus_reply_method_return(p_msg, nullptr);
}

static void cJSON_Deletep(cJSON **json) {
    if (*json) {
        cJSON_Delete(*json);
    }
}

static void notification_freep(Notification **ptr) {
    if (*ptr) {
        free(*ptr);
    }
}

static ResultVoid get_notifications(
    Notification **array, size_t *size, size_t max_size
) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    static char command[250];
    static bool initialized = false;
    if (!initialized) {
        snprintf(
            command,
            sizeof(command),
            "dunstctl history"
            "| jq '[limit(%u; .data[0][]"
            "| select(.urgency.data == \"NORMAL\" or .urgency.data == "
            "\"CRITICAL\")"
            "| {"
            "    body: .body.data,"
            "    icon: .icon_path.data,"
            "    app: .appname.data,"
            "    summary: .summary.data,"
            "    timestamp: .timestamp.data"
            "})]'",
            MAX_NOTIFICATIONS
        );
        initialized = true;
    }

    char output[KIBIBYTE * 4];
    ResultVoid exec_result =
        exec_command_as_user(output, sizeof(output), command, "r", UID);
    if (exec_result.variant == ERR) {
        res.err_msg = exec_result.err_msg;
        return res;
    }

    cJSON *json [[gnu::cleanup(cJSON_Deletep)]] = cJSON_Parse(output);
    if (json == nullptr) {
        res.err_msg = "Failed to get json object";
        return res;
    }
    if (!cJSON_IsArray(json)) {
        res.err_msg = "Json object is not an array";
        return res;
    }

    size_t index = 0;
    cJSON *item = nullptr;
    cJSON_ArrayForEach(item, json) {
        if (*size >= max_size) {
            res.err_msg = "Received more notifications than can hold";
            return res;
        }
        Notification *notif [[gnu::cleanup(notification_freep)]] =
            malloc(sizeof(Notification));
        if (notif == nullptr) {
            res.err_msg = "Failed to alloc";
            return res;
        }

        cJSON *body = cJSON_GetObjectItemCaseSensitive(item, "body");
        cJSON *app = cJSON_GetObjectItemCaseSensitive(item, "app");
        cJSON *icon = cJSON_GetObjectItemCaseSensitive(item, "icon");
        cJSON *summary = cJSON_GetObjectItemCaseSensitive(item, "summary");
        cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(item, "timestamp");
        if (!cJSON_IsString(body) || !cJSON_IsString(app) ||
            !cJSON_IsString(icon) || !cJSON_IsString(summary) ||
            !cJSON_IsNumber(timestamp)) {
            res.err_msg = "Json item is not the correct type";
            return res;
        }

        notif->body = strdup(body->valuestring);
        notif->app = strdup(app->valuestring);
        notif->icon = strdup(icon->valuestring);
        notif->summary = strdup(summary->valuestring);
        notif->timestamp = (int64_t)metric_conversion(
            timestamp->valuedouble, METRIC_MICRO, METRIC_BASE
        );
        if (notif->body == nullptr || notif->app == nullptr ||
            notif->icon == nullptr) {
            res.err_msg = "Strdup failed";
            return res;
        }
        array[index] = notif;

        index++;
        *size = index;
    }

    res.variant = OK;
    res.err_msg = "";
    return res;
}
