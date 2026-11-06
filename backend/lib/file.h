#ifndef _FILE_H
#define _FILE_H

#include "lib/result.h"

ResultHeapString read_file(const char *path);

ResultVoid write_file(const char *path, const char *content);

#endif
