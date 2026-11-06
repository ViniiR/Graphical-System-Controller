#include "lib/transform.h"
#include "lib/result.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// Removes all newlines '\r' '\n' from string
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

double metric_conversion(double value, MetricExponent from, MetricExponent to) {
    int8_t difference = (int8_t)from - (int8_t)to;
    double converted = value * pow(10.0, difference);
    return converted;
}
