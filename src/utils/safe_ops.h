#pragma once

#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

/* Safe integer multiplication with overflow checking */
static inline bool safe_mul_int(int a, int b, int* result) {
    if (a > 0) {
        if (b > 0) {
            if (a > (INT_MAX / b)) {
                return false;  /* Overflow */
            }
        } else {
            if (b < (INT_MIN / a)) {
                return false;  /* Underflow */
            }
        }
    } else {
        if (b > 0) {
            if (a < (INT_MIN / b)) {
                return false;  /* Underflow */
            }
        } else {
            if ((a != 0) && (b < (INT_MAX / a))) {
                return false;  /* Overflow */
            }
        }
    }
    *result = a * b;
    return true;
}

/* Safe size_t multiplication with overflow checking */
static inline bool safe_mul_size(size_t a, size_t b, size_t* result) {
    if ((b != 0U) && (a > (SIZE_MAX / b))) {
        return false;  /* Overflow */
    }
    *result = a * b;
    return true;
}

/* Safe addition with overflow checking */
static inline bool safe_add_size(size_t a, size_t b, size_t* result) {
    if (a > (SIZE_MAX - b)) {
        return false;  /* Overflow */
    }
    *result = a + b;
    return true;
}

/* Safe string to long conversion */
static inline bool safe_strtol(const char* str, long* result) {
    char* endptr = NULL;
    long val;

    if ((str == NULL) || (*str == '\0')) {
        return false;
    }

    errno = 0;
    val = strtol(str, &endptr, 10);

    /* Check for various conversion errors */
    if ((errno == ERANGE) || (endptr == str) || (*endptr != '\0')) {
        return false;
    }

    *result = val;
    return true;
}

/* Safe string to size_t conversion */
static inline bool safe_str_to_size(const char* str, size_t* result) {
    long val;

    if (!safe_strtol(str, &val)) {
        return false;
    }

    if (val < 0) {
        return false;  /* Negative value */
    }

    *result = (size_t)val;
    return true;
}
