#include "lib/result.h"
#include "lib/units.h"
#include "sys/wait.h"
#include "unistd.h"
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const unsigned int UID = 1000;

/// WARNING: if programs are not available in root's $PATH
/// must add it flake.nix systemd.services.<...>.path
ResultVoid exec_command(
    char *output, unsigned int size, const char *command, const char *modes
) {
    static thread_local char error_message[KIBIBYTE];

    ResultVoid res = RESULT_VOID_DEFAULT;
    FILE *fp;

    fp = popen(command, modes);
    if (fp == nullptr) {
        snprintf(
            error_message,
            sizeof(error_message),
            "Failed to execute Command: '%s'",
            command
        );
        res.err_msg = error_message;
        return res;
    }
    if (output == nullptr && size == 0) {
        pclose(fp);
        res.variant = OK;
        res.err_msg = "";
        return res;
    }

    //

    char full_output[size];
    full_output[0] = '\0';

    char *line = nullptr;
    size_t len = 0;
    ssize_t read_bytes;

    while ((read_bytes = getline(&line, &len, fp)) != -1) {
        if (read_bytes == 0 && output != nullptr) {
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
    unsigned int size,
    const char *command,
    const char *modes,
    uid_t target_uid
) {
    ResultVoid res = RESULT_VOID_DEFAULT;

    int output_pipe[2];
    if (pipe(output_pipe) == -1) {
        res.err_msg = "Failed to open pipe to forked process";
        return res;
    }

    pid_t pid = fork();
    if (pid < 0) {
        res.err_msg = "Failed to fork process";
        return res;
    }

    // Run as child process
    if (pid == 0) {
        close(output_pipe[0]);

        char runtime_dir[64];
        snprintf(runtime_dir, sizeof(runtime_dir), "/run/user/%d", target_uid);
        setenv("XDG_RUNTIME_DIR", runtime_dir, 1);

        struct passwd *pw = getpwuid(target_uid);
        char xauth_path[KIBIBYTE];
        snprintf(xauth_path, sizeof(xauth_path), "%s/.Xauthority", pw->pw_dir);
        setenv("XAUTHORITY", xauth_path, 1);
        setenv("DISPLAY", ":0", 1);

        if (setgid(target_uid) != 0 || setuid(target_uid) != 0) {
            ssize_t bytes_written =
                write(output_pipe[1], "Failed to drop process privileges", 34);
            (void)bytes_written;
            close(output_pipe[1]);
            _exit(-1);
        }

        ResultVoid result_exec = exec_command(output, size, command, modes);
        if (result_exec.variant == ERR) {
            ssize_t bytes_written = write(
                output_pipe[1],
                result_exec.err_msg,
                strlen(result_exec.err_msg) + 1
            );
            (void)bytes_written;
            close(output_pipe[1]);
            _exit(-2);
        }

        int bytes_written = write(output_pipe[1], output, size);
        if (bytes_written == -1) {
            ssize_t bytes_written = write(
                output_pipe[1], "Failed to write exec_command output", 36
            );
            (void)bytes_written;
            close(output_pipe[1]);
            _exit(-3);
        }

        close(output_pipe[1]);

        _exit(0);
    }
    // Run as parent process
    else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                // NOTE: I guarantee that err_pipe's stdout
                // is a static lifetime const char *
                // the (char *) cast is intentional, and only meant to
                // satisfy the compiler
                int bytes_read =
                    read(output_pipe[0], (char *)res.err_msg, size);
                if (bytes_read == -1) {
                    res.err_msg = "Failed to read output of exec_command";
                    close(output_pipe[0]);
                    return res;
                }
                res.err_msg = "Child process failed to write output";
                close(output_pipe[0]);
                return res;
            }
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            res.err_msg = "Child process terminated by signal";
            fprintf(stderr, "Terminated by signal: %i\n", sig);
            close(output_pipe[0]);
            return res;
        } else {
            res.err_msg = "Process failed or exited abnormally";
            close(output_pipe[0]);
            return res;
        }

        int bytes_read = read(output_pipe[0], output, size);
        if (bytes_read == -1) {
            res.err_msg = "Failed to read output of exec_command";
            close(output_pipe[0]);
            return res;
        }

        close(output_pipe[0]);

        res.variant = OK;
        res.err_msg = "";
        return res;
    }

    return res;
}
