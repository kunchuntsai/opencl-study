/**
 * @file types.h
 * @brief Shared type definitions for OpenCL framework
 *
 * Contains common types used by both config parsing and runtime:
 * - ScalarValue: Named scalar parameters (int/float/size_t)
 * - ImageDimensions: Image width/height/stride/channels
 * - BufferType: Buffer access modes
 * - ScalarType: Scalar value types
 *
 * This eliminates duplication between config.h and op_interface.h,
 * ensuring a single source of truth for shared data structures.
 */

#pragma once

#include <stddef.h>

#include "limits.h"

/* =============================================================================
 * Enumerations
 * ===========================================================================*/

/** Buffer access type enumeration */
typedef enum {
    BUFFER_TYPE_NONE = 0,
    BUFFER_TYPE_READ_ONLY,
    BUFFER_TYPE_WRITE_ONLY,
    BUFFER_TYPE_READ_WRITE
} BufferType;

/** Scalar value type enumeration */
typedef enum {
    SCALAR_TYPE_NONE = 0,
    SCALAR_TYPE_INT,   /**< 32-bit signed integer */
    SCALAR_TYPE_FLOAT, /**< 32-bit floating point */
    SCALAR_TYPE_SIZE   /**< size_t (platform-dependent) */
} ScalarType;

/** Host type enumeration for OpenCL API selection */
typedef enum {
    HOST_TYPE_STANDARD = 0, /**< Standard OpenCL API (default) */
    HOST_TYPE_CL_EXTENSION  /**< Custom CL extension API */
} HostType;

/** Border handling modes */
typedef enum {
    BORDER_CLAMP = 0,     /**< Clamp to edge (replicate edge pixels) */
    BORDER_REPLICATE = 1, /**< Same as clamp */
    BORDER_CONSTANT = 2,  /**< Use constant border value */
    BORDER_REFLECT = 3,   /**< Reflect border pixels */
    BORDER_WRAP = 4       /**< Wrap around (periodic) */
} BorderMode;

/* =============================================================================
 * Shared Structures
 * ===========================================================================*/

/**
 * @brief Image dimensions and layout
 *
 * Common structure for image parameters used in both config and runtime.
 * Eliminates duplication of width/height/stride/channels fields.
 */
typedef struct {
    int width;    /**< Image width in pixels */
    int height;   /**< Image height in pixels */
    int stride;   /**< Stride in bytes (0 = packed, width * channels) */
    int channels; /**< Number of channels (e.g., 1 for gray, 3 for RGB, 4 for RGBA) */
} ImageDimensions;

/**
 * @brief Named scalar value
 *
 * Holds a named scalar parameter that can be passed to kernels.
 * Used by both config parsing and runtime execution.
 * Supports int, float, and size_t types.
 */
typedef struct {
    char name[64];   /**< Scalar name (e.g., "sigma", "threshold") */
    ScalarType type; /**< Value type */
    union {
        int int_value;     /**< Integer value */
        float float_value; /**< Float value */
        size_t size_value; /**< Size value */
    } value;
} ScalarValue;

/**
 * @brief Collection of scalar values
 *
 * Array container for ScalarValue structs.
 * Used by both config and runtime for algorithm-specific parameters.
 */
typedef struct {
    ScalarValue scalars[MAX_SCALARS]; /**< Array of scalar values */
    int count;                        /**< Number of scalars */
} ScalarCollection;

/* Backward compatibility alias */
#define CustomScalars ScalarCollection
