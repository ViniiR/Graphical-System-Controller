#include "lib/transform.h"
#include "lib/types.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

ResultHeapString read_file(const char *path) {
    ResultHeapString res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = ""
    };

    FILE *f = fopen(path, "r");
    if (f == nullptr) {
        res.err_msg = "Failed to open file with 'read' mode";
        return res;
    }

    char *buffer = nullptr;
    size_t len = 0;

    ssize_t bytes_read = getdelim(&buffer, &len, '\0', f); // malloc
    fclose(f);
    if (bytes_read == -1) {
        free(buffer);
        res.err_msg = "Failed to read file content";
        return res;
    }
    trim_newlines(buffer);

    res.variant = OK;
    res.ok_value = buffer;
    res.err_msg = "";
    return res;
}

ResultVoid write_file(const char *path, const char *content) {
    ResultVoid res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = nullptr
    };

    errno = 0;

    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            res.err_msg = "Failed to write to file, does not exist";
        } else {
            res.err_msg = "Failed to write to file, failed to open";
        }
        return res;
    }

    FILE *f = fdopen(fd, "w");
    if (f == nullptr) {
        res.err_msg = "Failed to open file with 'write' mode";
        return res;
    }

    int chars_written = fprintf(f, "%s", content);
    fclose(f);
    if (chars_written == -1) {
        res.err_msg = "Failed to write to file";
        return res;
    }

    res.variant = OK;
    res.err_msg = "";
    return res;
}
