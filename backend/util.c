#include "sys/types.h"
#include "sys/wait.h"
#include "types.h"
#include "unistd.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

/// Remove all newlines '\r' '\n' from str
void trim_newlines(char *str) {
    //
    str[strcspn(str, "\r\n")] = 0;
}

ResultInt string_to_int(const char *string) {
    ResultInt res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = 0
    };

    errno = 0;

    char *endptr;
    int value = strtol(string, &endptr, 10);

    if (endptr == string) {
        res.err_msg = "Failed to convert string to int, no numbers found";
        return res;
    }
    if (*endptr != '\0') {
        res.err_msg =
            "Failed to convert string to int, invalid character found";
        return res;
    }
    if (errno == ERANGE) {
        res.err_msg =
            "Failed to convert string to int, overflow or underflow occurred";
        return res;
    }
    if (value < INT_MIN || value > INT_MAX) {
        res.err_msg =
            "Failed to convert string to int, does not fit 'int' type";
        return res;
    }

    res.variant = OK;
    res.err_msg = "";
    res.ok_value = value;
    return res;
}

ResultHeapString read_file(const char *path) {
    ResultHeapString res = {
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = ""
    };

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        res.err_msg = "Failed to open file with 'read' mode";
        return res;
    }

    char *buffer = NULL;
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
        .variant = ERR, .err_msg = RESULT_ERR_MSG_UNKNOWN, .ok_value = NULL
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
    if (f == NULL) {
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

/// WARNING: if programs are not available in root's $PATH
/// must add it flake.nix systemd.services.<...>.path
ResultVoid exec_command(
    char *output,
    const unsigned int size,
    const char *command,
    const char *modes
) {
    static thread_local char error_message[STRING_KB];

    ResultVoid res = RESULT_VOID_DEFAULT;
    FILE *fp;

    fp = popen(command, modes);
    if (fp == NULL) {
        snprintf(
            error_message,
            sizeof(error_message),
            "Failed to execute Command: '%s'",
            command
        );
        res.err_msg = error_message;
        return res;
    }

    //

    char full_output[size];
    full_output[0] = '\0';

    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;

    while ((read_bytes = getline(&line, &len, fp)) != -1) {
        if (read_bytes == 0 && output != NULL) {
            pclose(fp);
            snprintf(
                error_message,
                sizeof(error_message),
                "Could not read file, Command: '%s'",
                command
            );
            res.err_msg = error_message;
            return res;
        }
        strcat(full_output, line);
    }

    snprintf(output, size, "%s", full_output);

    free(line);
    pclose(fp);

    res.variant = OK;
    res.err_msg = "";
    return res;
}

ResultVoid exec_command_as_user(
    char *output,
    const unsigned int size,
    const char *command,
    const char *modes,
    const uid_t target_uid
) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        res.err_msg = "Failed fork pipe";
        return res;
    }

    pid_t pid = fork();
    if (pid < 0) {
        res.err_msg = "Failed to fork process";
        return res;
    }

    // TODO: this is the only part of the application that doesn't propagate
    // errors up.
    // Run as child process
    if (pid == 0) {
        close(pipefd[0]);

        char runtime_dir[64];
        snprintf(runtime_dir, sizeof(runtime_dir), "/run/user/%d", target_uid);
        setenv("XDG_RUNTIME_DIR", runtime_dir, 1);
        if (setgid(target_uid) != 0 || setuid(target_uid) != 0) {
            // res.err_msg = "Failed to drop process privileges";
            _exit(-1);
        }

        ResultVoid result_exec = exec_command(output, size, command, modes);
        if (result_exec.variant == ERR) {
            // res.err_msg = result_exec.err_msg;
            _exit(-2);
        }

        write(pipefd[1], output, size);
        close(pipefd[1]);

        _exit(0);
    }
    // Run as parent process
    else if (pid > 0) {
        close(pipefd[1]);

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                res.err_msg = "Child process exited with failure code";
                fprintf(stderr, "Terminated by code: %i\n", exit_code);
                return res;
            }
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            res.err_msg = "Child process terminated by signal";
            fprintf(stderr, "Terminated by signal: %i\n", sig);
            return res;
        } else {
            res.err_msg = "Process failed or exited abnormally";
            return res;
        }

        read(pipefd[0], output, size);
        close(pipefd[0]);

        res.variant = OK;
        res.err_msg = "";
        return res;
    }

    return res;
}
