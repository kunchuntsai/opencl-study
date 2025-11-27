/**
 * @file op_registry.h
 * @brief Algorithm registry for dynamic algorithm discovery
 *
 * Provides a central registry for all image processing algorithms.
 * Algorithms register themselves at startup, allowing the framework
 * to discover and execute any algorithm by ID.
 *
 * Registry pattern enables:
 * - Adding new algorithms without modifying main code
 * - Runtime algorithm selection
 * - Algorithm enumeration for UI
 *
 * MISRA C 2023 Compliance:
 * - Rule 8.7: Static storage with file scope
 * - Rule 18.1: Bounds checking on registry access
 */

#pragma once

#include "op_interface.h"

/** Maximum number of algorithms that can be registered */
#define MAX_ALGORITHMS 32

/**
 * @brief Register an algorithm in the global registry
 *
 * Adds an algorithm to the registry, making it available for lookup
 * and execution. Called at startup by each algorithm module.
 *
 * @param[in] op Pointer to algorithm structure (must remain valid)
 *
 * @note Fails silently if MAX_ALGORITHMS is exceeded
 * @note Not thread-safe - should only be called during initialization
 */
void register_algorithm(Algorithm* op);

/**
 * @brief Find algorithm by unique identifier
 *
 * Searches the registry for an algorithm matching the given ID.
 *
 * @param[in] op_id Algorithm identifier (e.g., "dilate3x3")
 * @return Pointer to algorithm, or NULL if not found
 */
Algorithm* find_algorithm(const char* op_id);

/**
 * @brief Get algorithm by index
 *
 * Retrieves algorithm at specified index in registry.
 * Used for menu display and iteration.
 *
 * @param[in] index Zero-based index (0 to get_algorithm_count()-1)
 * @return Pointer to algorithm, or NULL if index out of range
 */
Algorithm* get_algorithm_by_index(int index);

/**
 * @brief Get number of registered algorithms
 *
 * @return Total count of algorithms in registry
 */
int get_algorithm_count(void);

/**
 * @brief List all registered algorithms to stdout
 *
 * Prints a formatted list of all available algorithms with
 * their indices, names, and IDs.
 */
void list_algorithms(void);

/**
 * @brief Macro to define and auto-register an algorithm
 *
 * Convenience macro that eliminates registration boilerplate.
 * Creates an Algorithm struct and registers it before main() runs.
 *
 * Usage: REGISTER_ALGORITHM(op_id, "Display Name", ref_func, verify_func, set_args_func)
 *
 * Example:
 *   static int dilate3x3_set_kernel_args(...) { ... }
 *   REGISTER_ALGORITHM(dilate3x3, "Dilate 3x3", dilate3x3_ref, dilate3x3_verify, dilate3x3_set_kernel_args)
 *
 * To add a new algorithm:
 * 1. Implement the reference function (e.g., myalgo_ref)
 * 2. Implement the verify function (e.g., myalgo_verify)
 * 3. Implement the set_kernel_args function (e.g., myalgo_set_kernel_args)
 * 4. At the end of your .c file, add one line:
 *    REGISTER_ALGORITHM(myalgo, "My Algorithm", myalgo_ref, myalgo_verify, myalgo_set_kernel_args)
 * 5. In config.ini, set: op_id = myalgo
 *
 * The macro generates:
 * - An Algorithm struct with the given parameters
 * - A constructor function that auto-registers before main()
 * - Uses op_id as both the string identifier and variable name
 *
 * For algorithms needing custom buffers:
 * - Implement create_buffers and destroy_buffers functions
 * - Define Algorithm struct manually instead of using this macro
 *
 * IMPORTANT: The op_id parameter must match the op_id value in config.ini
 * Example consistency:
 *   Code:   REGISTER_ALGORITHM(dilate3x3, ...)
 *   Config: op_id = dilate3x3
 *
 * @param op_id Algorithm identifier (must match config.ini op_id field)
 * @param display_name Human-readable name shown in UI
 * @param ref_func C reference implementation: void func(const OpParams*)
 * @param verify_func GPU verification: int func(const OpParams*, float*)
 * @param set_args_func Kernel argument setter: int func(cl_kernel, cl_mem, cl_mem, const OpParams*, void*)
 */
#define REGISTER_ALGORITHM(op_id, display_name, ref_func, verify_func, set_args_func) \
    static Algorithm op_id##_algorithm = { \
        .name = display_name, \
        .id = #op_id, \
        .reference_impl = ref_func, \
        .verify_result = verify_func, \
        .create_buffers = NULL, \
        .destroy_buffers = NULL, \
        .set_kernel_args = set_args_func \
    }; \
    \
    __attribute__((constructor)) \
    static void op_id##_init(void) { \
        register_algorithm(&op_id##_algorithm); \
    }
