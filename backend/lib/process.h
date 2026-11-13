#ifndef _PROCESS_H
#define _PROCESS_H

#include "lib/result.h"
#include <stddef.h>
#include <sys/types.h>

extern const unsigned int UID;

ResultVoid exec_command(
    char *output,
    size_t size,
    const char *command,
    const char *modes
);

ResultVoid exec_command_as_user(
    char *output,
    size_t size,
    const char *command,
    const char *modes,
    uid_t target_uid
);

#endif
