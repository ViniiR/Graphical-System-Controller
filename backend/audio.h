#ifndef _AUDIO_H
#define _AUDIO_H

#include <systemd/sd-bus.h>

extern const char AUDIO_PATH[];
extern const char AUDIO_INTERFACE[];

int get_audio_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int set_audio_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int toggle_audio_muted_handler(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

int get_all_audio_handler_individual(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int set_audio_handler_individual(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);
int toggle_audio_muted_handler_individual(
    sd_bus_message *p_msg, void *p_userdata, sd_bus_error *p_reterror
);

extern const sd_bus_vtable AUDIO_VTABLE[];

#endif
