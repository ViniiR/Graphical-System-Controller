#include "audio_internal.h"
#include "types.h"
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char get_ids_command[] =
    "pw-dump | jq -r '.[] | select(.type == \"PipeWire:Interface:Node\" and "
    "(.info.props[\"media.class\"] | strings | startswith(\"Stream/\") and "
    "contains(\"Audio\"))) | .id '";

ResultHeapStructPointer get_stream(unsigned int id);

static const unsigned int UID = 1000;

/// stream_array is an array of allocated structs(must free individually)
/// array_size is a pointer to keep track of array_stream's size
ResultVoid get_all_streams(AudioStream *stream_array[], size_t *array_size) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    char output_id[STRING_KB];
    ResultVoid result_ids = exec_command_as_user(
        output_id, sizeof(output_id), get_ids_command, "r", UID
    );
    if (result_ids.variant == ERR) {
        res.err_msg = result_ids.err_msg;
        return res;
    }

    FILE *f = fmemopen(output_id, strlen(output_id), "r");
    if (f == nullptr) {
        res.err_msg = "fmemopen failed";
        return res;
    }

    //

    char *line = nullptr;
    size_t len = 0;
    ssize_t read_bytes;

    while ((read_bytes = getline(&line, &len, f)) != -1) {
        trim_newlines(line);

        // Avoids final \n
        if (strcmp(line, "") == 0) {
            continue;
        }

        ResultInt line_int_result = string_to_int(line);
        if (line_int_result.variant==ERR) {
            free(line);
            fclose(f);
            res.err_msg = line_int_result.err_msg;
            return res;
        }
        ResultHeapStructPointer result = get_stream(line_int_result.ok_value);
        if (result.variant == ERR) {
            free(line);
            fclose(f);
            res.err_msg = result.err_msg;
            return res;
        }
        AudioStream *stream = result.ok_value;

        stream_array[(*array_size)++] = stream;
    }

    free(line);
    fclose(f);

    res.variant = OK;
    res.err_msg = "";
    return res;
}

ResultHeapStructPointer get_stream(unsigned int id) {
    ResultHeapStructPointer res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = nullptr
    };

    char id_char[STRING_KB];
    snprintf(id_char, sizeof(id_char), "%i", id);

    //

    char name_command[STRING_KB];
    snprintf(
        name_command,
        sizeof(name_command),
        "wpctl inspect %i | grep media.name | sed -E "
        "'s/\\s*media\\.name\\s=\\s\"(.*)\"$/\\1/'",
        id
    );
    char output_name[STRING_KB];
    ResultVoid result_name = exec_command_as_user(
        output_name, sizeof(output_name), name_command, "r", UID
    );
    if (result_name.variant == ERR) {
        res.err_msg = result_name.err_msg;
        return res;
    }
    trim_newlines(output_name);

    //

    ResultHeapStructPointer result_volume =
        external_get_pipewire_volume(id_char);
    if (result_volume.variant == ERR) {
        res.err_msg = result_volume.err_msg;
        return res;
    }
    VolumeStatus *status = result_volume.ok_value;

    AudioStream *audio = malloc(sizeof(AudioStream));
    audio->sink_id = id;
    audio->name = strdup(output_name);
    audio->volume = status->volume;
    audio->is_muted = status->is_muted;

    free(result_volume.ok_value);

    res.variant = OK;
    res.err_msg = "";
    res.ok_value = audio;
    return res;
}
