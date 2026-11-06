#ifndef _UNITS_H
#define _UNITS_H

#include <stdint.h>

constexpr uint64_t BYTE = 1;
constexpr uint64_t KIBIBYTE = BYTE * 1024;
constexpr uint64_t MEBIBYTE = KIBIBYTE * 1024;
constexpr uint64_t GIBIBYTE = MEBIBYTE * 1024;
constexpr uint64_t TEBIBYTE = GIBIBYTE * 1024;

#endif
