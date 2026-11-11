#ifndef _CLEANUP_H
#define _CLEANUP_H

#include "lib/result.h"
#include <dirent.h>
#include <stdio.h>

void closep(int *file);
void fclosep(FILE **file);
void fdclosep(FILE **file);

void closedirp(DIR **dir);

void pclosep(FILE **file);

void freep(void **ptr);
void cfreep(char **ptr);

void result_string_freep(ResultHeapString *ptr);
void result_struct_freep(ResultHeapStruct *ptr);

#endif
