#include "cleanup.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void closep(int *file) {
    if (file) {
        close(*file);
    }
}
void fclosep(FILE **file) {
    if (file && *file) {
        fclose(*file);
    }
}
void fdclosep(FILE **file) {
    if (file && *file) {
        fclose(*file);
    }
}

void closedirp(DIR **dir) {
    if (dir && *dir) {
        closedir(*dir);
    }
}

void pclosep(FILE **file) {
    if (file && *file) {
        fclose(*file);
    }
}

void freep(void **ptr) {
    if (ptr && *ptr) {
        free(*ptr);
    }
}
void cfreep(char **ptr) {
    if (ptr && *ptr) {
        free(*ptr);
    }
}

void result_string_freep(ResultHeapString *ptr) {
    if (ptr && ptr->ok_value) {
        free(ptr->ok_value);
    }
}
void result_struct_freep(ResultHeapStruct *ptr) {
    if (ptr && ptr->ok_value) {
        free(ptr->ok_value);
    }
}
