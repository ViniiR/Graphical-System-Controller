#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "lib/types.h"
#include <stdint.h>

typedef enum : int8_t {
    METRIC_YOCTO = -24,
    METRIC_ZEPTO = -21,
    METRIC_ATTO = -18,
    METRIC_FEMTO = -15,
    METRIC_PICO = -12,
    METRIC_NANO = -9,
    METRIC_MICRO = -6,
    METRIC_MILLI = -3,
    METRIC_CENTI = -2,
    METRIC_DECI = -1,
    METRIC_BASE = 0,
    METRIC_DECA = 1,
    METRIC_HECTO = 2,
    METRIC_KILO = 3,
    METRIC_MEGA = 6,
    METRIC_GIGA = 9,
    METRIC_TERA = 12,
    METRIC_PETA = 15,
    METRIC_EXA = 18,
    METRIC_ZETTA = 21,
    METRIC_YOTTA = 24
} MetricExponent;

void trim_newlines(char *str);

ResultInt string_to_int(const char *string);

double metric_conversion(double value, MetricExponent from, MetricExponent to);

#endif
