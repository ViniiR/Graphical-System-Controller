#include "audio.h"
#include "battery.h"
#include "boost.h"
#include "brightness.h"
#include "conservation.h"
#include "power.h"
#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include <stdbool.h>

const char DBUS_SERVICE_NAME[] = "com.vinii.vgsc";
sd_bus *p_dbus = nullptr;

static int fail_with_message(const char message[], sd_bus *p, const int code);

// TODO: OPTIONAL: add cpu freq governor option
int main(int argc, char **argv) {
    int err = 0;

    err = sd_bus_open_system(&p_dbus);
    if (err < 0)
        return fail_with_message("Failed to connect to bus", p_dbus, err);
    printf("Connected to bus\n");

    // Brightness Controller
    err = sd_bus_add_object_vtable(
        p_dbus,
        nullptr,
        BRIGHTNESS_PATH,
        BRIGHTNESS_INTERFACE,
        BRIGHTNESS_VTABLE,
        nullptr
    );
    if (err < 0)
        return fail_with_message("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", BRIGHTNESS_PATH);

    // Power controller
    err = sd_bus_add_object_vtable(
        p_dbus,
        nullptr,
        POWER_PATH,
        POWER_INTERFACE,
        POWER_VTABLE,
        nullptr
    );
    if (err < 0)
        return fail_with_message("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", POWER_PATH);

    // Battery controller
    err = sd_bus_add_object_vtable(
        p_dbus,
        nullptr,
        BATTERY_PATH,
        BATTERY_INTERFACE,
        BATTERY_VTABLE,
        nullptr
    );
    if (err < 0)
        return fail_with_message("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", BATTERY_PATH);

    // Boost controller
    err = sd_bus_add_object_vtable(
        p_dbus,
        nullptr,
        BOOST_PATH,
        BOOST_INTERFACE,
        BOOST_VTABLE,
        nullptr
    );
    if (err < 0)
        return fail_with_message("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", BOOST_PATH);

    // Conservation controller
    err = sd_bus_add_object_vtable(
        p_dbus,
        nullptr,
        CONSERVATION_PATH,
        CONSERVATION_INTERFACE,
        CONSERVATION_VTABLE,
        nullptr
    );
    if (err < 0)
        return fail_with_message("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", CONSERVATION_PATH);

    // Audio controller
    err = sd_bus_add_object_vtable(
        p_dbus,
        nullptr,
        AUDIO_PATH,
        AUDIO_INTERFACE,
        AUDIO_VTABLE,
        nullptr
    );
    if (err < 0)
        return fail_with_message("Failed to add object vtable", p_dbus, err);
    printf("Added '%s' object vtable\n", AUDIO_PATH);

    err = sd_bus_request_name(p_dbus, DBUS_SERVICE_NAME, 0);
    if (err < 0)
        return fail_with_message("Failed to request service name", p_dbus, err);
    printf("Started with service name: '%s'\n", DBUS_SERVICE_NAME);

    while (true) {
        err = sd_bus_process(p_dbus, nullptr);
        if (err < 0)
            return fail_with_message("Failed to process bus", p_dbus, err);

        err = sd_bus_wait(p_dbus, 100);
        if (err < 0)
            return fail_with_message("Failed to wait for bus", p_dbus, err);
    }

    // Finish
    sd_bus_unref(p_dbus);
    return err;
}

static int fail_with_message(const char message[], sd_bus *p, const int code) {
    fprintf(stderr,"D-Bus Error: %s %s\n", message, strerror(-code));

    sd_bus_unref(p);
    return code;
}
