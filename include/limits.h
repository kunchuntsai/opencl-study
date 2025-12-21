/**
 * @file limits.h
 * @brief Framework-wide limit constants
 *
 * Centralized definitions for all array size limits used across the framework.
 * This ensures consistency between config parsing and runtime storage.
 *
 * Usage: Include this header in any file that needs these limits.
 */

#pragma once

/* =============================================================================
 * Scalar and Struct Limits
 * ===========================================================================*/

/** Maximum number of scalar arguments (shared by config parser and runtime) */
#define MAX_SCALARS 128

/** Maximum number of fields in a single struct argument */
#define MAX_STRUCT_FIELDS 16

/* =============================================================================
 * Buffer Limits
 * ===========================================================================*/

/** Maximum number of custom buffers per algorithm */
#define MAX_CUSTOM_BUFFERS 8

/* =============================================================================
 * Kernel Configuration Limits
 * ===========================================================================*/

/** Maximum number of kernel variants per algorithm */
#define MAX_KERNEL_CONFIGS 32

/** Maximum number of kernel arguments per kernel */
#define MAX_KERNEL_ARGS 32

/* =============================================================================
 * Image Configuration Limits
 * ===========================================================================*/

/** Maximum number of input images */
#define MAX_INPUT_IMAGES 16

/** Maximum number of output images */
#define MAX_OUTPUT_IMAGES 16
