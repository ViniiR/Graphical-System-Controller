#include "notification.h"
#include "lib/process.h"
#include "lib/result.h"
#include "lib/transform.h"
#include "lib/units.h"
#include <cjson/cJSON.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

int get_recent_notifications(
    sd_bus_message *p_msg,
    [[maybe_unused]] void *p_userdata,
    sd_bus_error *p_reterror
) {
    Notification *output[MAX_NOTIFICATIONS];
    size_t size = 0;

    ResultVoid result = get_notifications(output, &size, sizeof(output));
    if (result.variant == ERR) {
        return sd_bus_error_setf(
            p_reterror,
            SD_BUS_ERROR_FAILED,
            "Failed to get notifications, Error: %s",
            result.err_msg
        );
    }

    sd_bus_message *p_reply_msg = nullptr;
    sd_bus_message_new_method_return(p_msg, &p_reply_msg);

    sd_bus_message_open_container(p_reply_msg, SD_BUS_TYPE_ARRAY, "(ssssx)");
    for (size_t i = 0; i < size; i++) {
        sd_bus_message_open_container(p_reply_msg, SD_BUS_TYPE_STRUCT, "ssssx");

        sd_bus_message_append(p_reply_msg, "s", output[i]->body);
        sd_bus_message_append(p_reply_msg, "s", output[i]->app);
        sd_bus_message_append(p_reply_msg, "s", output[i]->icon);
        sd_bus_message_append(p_reply_msg, "s", output[i]->summary);
        sd_bus_message_append(p_reply_msg, "x", output[i]->timestamp);

        sd_bus_message_close_container(p_reply_msg);
    }
    sd_bus_message_close_container(p_reply_msg);

    int return_value = sd_bus_send(nullptr, p_reply_msg, nullptr);

    for (size_t i = 0; i < size; i++) {
        Notification *ptr = output[i];
        free(ptr->app);
        free(ptr->body);
        free(ptr->icon);
        free(ptr->summary);
        // don't forget that timestamp is pass-by-value
        free(ptr);
    }

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

static ResultVoid get_notifications(
    Notification **array, size_t *size, size_t max_size
) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    // TODO: prevent from being recalculated
    static char command[250];
    snprintf(
        command,
        sizeof(command),
        "dunstctl history"
        "| jq '[limit(%u; .data[0][]"
        "| select(.urgency.data == \"NORMAL\" or .urgency.data == \"CRITICAL\")"
        "| {"
        "    body: .body.data,"
        "    icon: .icon_path.data,"
        "    app: .appname.data,"
        "    summary: .summary.data,"
        "    timestamp: .timestamp.data"
        "})]'",
        MAX_NOTIFICATIONS
    );

    char output[KIBIBYTE * 4];
    ResultVoid exec_result =
        exec_command_as_user(output, sizeof(output), command, "r", UID);
    if (exec_result.variant == ERR) {
        res.err_msg = exec_result.err_msg;
        return res;
    }

    cJSON *json = cJSON_Parse(output);
    if (json == nullptr) {
        res.err_msg = "Failed to get json object";
        return res;
    }
    if (!cJSON_IsArray(json)) {
        res.err_msg = "Json object is not an array";
        cJSON_Delete(json);
        return res;
    }

    size_t index = 0;
    cJSON *item = nullptr;
    cJSON_ArrayForEach(item, json) {
        if (*size >= max_size) {
            res.err_msg = "Received more notifications than can hold";
            cJSON_Delete(json);
            return res;
        }
        Notification *notif = malloc(sizeof(Notification));
        if (notif == nullptr) {
            res.err_msg = "Failed to alloc";
            cJSON_Delete(json);
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
            cJSON_Delete(json);
            free(notif);
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

    cJSON_Delete(json);

    res.variant = OK;
    res.err_msg = "";
    return res;
}
