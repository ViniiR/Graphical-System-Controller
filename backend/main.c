#include "audio.h"
#include "battery.h"
#include "boost.h"
#include "brightness.h"
#include "conservation.h"
#include "keyboard.h"
#include "notification.h"
#include "power.h"
#include <systemd/sd-bus.h>

const char DBUS_SERVICE_PATH[] = "/com/vinii/vgsc";
const char DBUS_SERVICE_NAME[] = "com.vinii.vgsc";

//

static int fail_with_msg(const char message[], sd_bus *p, int code);

static int add_controller(const char *name, const sd_bus_vtable *vtable);

//

static sd_bus *p_dbus = nullptr;

int main(int argc, char **argv) {
    int err = 0;

    err = sd_bus_open_system(&p_dbus);
    if (err < 0) return fail_with_msg("Failed to connect to bus", p_dbus, err);
    printf("Connected to bus\n");

    // Brightness Controller
    err = add_controller(BRIGHTNESS_NAME, BRIGHTNESS_VTABLE);
    if (err < 0) return fail_with_msg("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", BRIGHTNESS_NAME);

    // Power controller
    err = add_controller(POWER_NAME, POWER_VTABLE);
    if (err < 0) return fail_with_msg("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", POWER_NAME);

    // Battery controller
    err = add_controller(BATTERY_NAME, BATTERY_VTABLE);
    if (err < 0) return fail_with_msg("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", BATTERY_NAME);

    // Boost controller
    err = add_controller(BOOST_NAME, BOOST_VTABLE);
    if (err < 0) return fail_with_msg("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", BOOST_NAME);

    // Conservation controller
    err = add_controller(CONSERVATION_NAME, CONSERVATION_VTABLE);
    if (err < 0) return fail_with_msg("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", CONSERVATION_NAME);

    // Audio controller
    err = add_controller(AUDIO_NAME, AUDIO_VTABLE);
    if (err < 0) return fail_with_msg("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", AUDIO_NAME);

    // Keyboard controller
    err = add_controller(KEYBOARD_NAME, KEYBOARD_VTABLE);
    if (err < 0) return fail_with_msg("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", KEYBOARD_NAME);

    // Notification controller
    err = add_controller(NOTIFICATION_NAME, NOTIFICATION_VTABLE);
    if (err < 0) return fail_with_msg("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", NOTIFICATION_NAME);

    // Bus
    err = sd_bus_request_name(p_dbus, DBUS_SERVICE_NAME, 0);
    if (err < 0) return fail_with_msg("Failed to request service name", p_dbus, err);
    printf("Started with service name: '%s'\n", DBUS_SERVICE_NAME);

    while (true) {
        err = sd_bus_process(p_dbus, nullptr);
        if (err < 0) return fail_with_msg("Failed to process bus", p_dbus, err);

        err = sd_bus_wait(p_dbus, 100);
        if (err < 0) return fail_with_msg("Failed to wait for bus", p_dbus, err);
    }

    // Finish
    sd_bus_unref(p_dbus);
    return err;
}

//

static int add_controller(const char *name, const sd_bus_vtable *vtable) {
    static const unsigned int MAX_PATH = 100;
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", DBUS_SERVICE_PATH, name);

    char interface[MAX_PATH];
    snprintf(interface, sizeof(interface), "%s.%s", DBUS_SERVICE_NAME, name);

    return sd_bus_add_object_vtable(
        p_dbus, nullptr, path, interface, vtable, nullptr
    );
}

static int fail_with_msg(const char message[], sd_bus *p, int code) {
    fprintf(stderr, "D-Bus Error: %s %s\n", message, strerror(-code));

    sd_bus_unref(p);
    return code;
}
