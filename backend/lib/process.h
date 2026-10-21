#ifndef _PROCESS_H
#define _PROCESS_H

#include "lib/types.h"
#include <sys/types.h>

ResultVoid exec_command(
    char *output,
    unsigned int size,
    const char *command,
    const char *modes
);

ResultVoid exec_command_as_user(
    char *output,
    unsigned int size,
    const char *command,
    const char *modes,
    uid_t target_uid
);

#endif
