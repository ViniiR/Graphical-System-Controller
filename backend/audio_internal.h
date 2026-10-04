#ifndef _AUDIO_INTERNAL_H
#define _AUDIO_INTERNAL_H

#include "types.h"

typedef struct {
    bool is_muted;
    int volume;
} VolumeStatus;

typedef struct {
    unsigned int sink_id;
    char *name;
    unsigned int volume;
    bool is_muted;
} AudioStream;

ResultHeapStructPointer external_get_pipewire_volume(const char *sink_id);

#endif
