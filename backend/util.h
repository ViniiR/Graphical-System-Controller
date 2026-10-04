#ifndef _UTIL_H
#define _UTIL_H

#include "types.h"
#include <sys/types.h>

void trim_newlines(char *str);

ResultInt string_to_int(const char *string);

ResultHeapString read_file(const char *path);

ResultVoid write_file(const char *path, const char *content);

ResultVoid exec_command(
    char *output,
    const unsigned int size,
    const char *command,
    const char *modes
);

ResultVoid exec_command_as_user(
    char *output,
    const unsigned int size,
    const char *command,
    const char *modes,
    const uid_t target_uid
);

#endif
