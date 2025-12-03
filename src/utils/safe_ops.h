/**
 * @file safe_ops.h
 * @brief Safe arithmetic and conversion operations with overflow checking
 *
 * This header provides MISRA C 2023 compliant utility functions for safe
 * arithmetic operations and type conversions. All functions include overflow
 * detection and proper error handling to prevent undefined behavior.
 *
 * MISRA C 2023 Compliance:
 * - Rule 1.3: Prevents undefined behavior from integer overflow
 * - Rule 21.8: Provides safe alternatives to atoi/atof/atol
 * - Rule 17.7: All functions return status for error checking
 */

#pragma once

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Safely multiply two integers with overflow checking
 *
 * Performs integer multiplication while checking for overflow in all cases
 * (positive * positive, positive * negative, negative * negative).
 * MISRA C 2023 Rule 1.3 compliant.
 *
 * @param[in] a First multiplicand
 * @param[in] b Second multiplicand
 * @param[out] result Pointer to store the multiplication result
 * @return true if multiplication succeeded without overflow, false otherwise
 */
static inline bool SafeMulInt(int a, int b, int* result) {
  if (a > 0) {
    if (b > 0) {
      if (a > (INT_MAX / b)) {
        return false; /* Overflow */
      }
    } else {
      if (b < (INT_MIN / a)) {
        return false; /* Underflow */
      }
    }
  } else {
    if (b > 0) {
      if (a < (INT_MIN / b)) {
        return false; /* Underflow */
      }
    } else {
      if ((a != 0) && (b < (INT_MAX / a))) {
        return false; /* Overflow */
      }
    }
  }
  *result = a * b;
  return true;
}

/**
 * @brief Safely multiply two size_t values with overflow checking
 *
 * Performs size_t multiplication while checking for overflow. Used for
 * calculating buffer sizes and array dimensions safely.
 * MISRA C 2023 Rule 1.3 compliant.
 *
 * @param[in] a First multiplicand
 * @param[in] b Second multiplicand
 * @param[out] result Pointer to store the multiplication result
 * @return true if multiplication succeeded without overflow, false otherwise
 */
static inline bool SafeMulSize(size_t a, size_t b, size_t* result) {
  if ((b != 0U) && (a > (SIZE_MAX / b))) {
    return false; /* Overflow */
  }
  *result = a * b;
  return true;
}

/**
 * @brief Safely add two size_t values with overflow checking
 *
 * Performs size_t addition while checking for overflow. Used for
 * calculating offsets and total sizes safely.
 * MISRA C 2023 Rule 1.3 compliant.
 *
 * @param[in] a First addend
 * @param[in] b Second addend
 * @param[out] result Pointer to store the addition result
 * @return true if addition succeeded without overflow, false otherwise
 */
static inline bool SafeAddSize(size_t a, size_t b, size_t* result) {
  if (a > (SIZE_MAX - b)) {
    return false; /* Overflow */
  }
  *result = a + b;
  return true;
}

/**
 * @brief Safely convert string to long integer
 *
 * Provides a safe alternative to atoi() with comprehensive error checking.
 * Detects invalid input, overflow, and partial conversions.
 * MISRA C 2023 Rule 21.8 compliant (avoids atoi/atof/atol).
 *
 * @param[in] str Input string to convert (must be null-terminated)
 * @param[out] result Pointer to store the converted long value
 * @return true if conversion succeeded, false on error (invalid format,
 * overflow, etc.)
 */
static inline bool SafeStrtol(const char* str, long* result) {
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

/**
 * @brief Safely convert string to size_t (unsigned)
 *
 * Provides a safe conversion from string to size_t with validation.
 * Rejects negative values and performs comprehensive error checking.
 * MISRA C 2023 Rule 21.8 compliant (avoids atoi/atof/atol).
 *
 * @param[in] str Input string to convert (must be null-terminated)
 * @param[out] result Pointer to store the converted size_t value
 * @return true if conversion succeeded, false on error (invalid format,
 * negative, overflow)
 */
static inline bool SafeStrToSize(const char* str, size_t* result) {
  long val;

  if (!SafeStrtol(str, &val)) {
    return false;
  }

  if (val < 0) {
    return false; /* Negative value */
  }

  *result = (size_t)val;
  return true;
}
