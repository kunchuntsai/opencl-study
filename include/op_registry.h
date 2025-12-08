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
void RegisterAlgorithm(Algorithm* op);

/**
 * @brief Find algorithm by unique identifier
 *
 * Searches the registry for an algorithm matching the given ID.
 *
 * @param[in] op_id Algorithm identifier (e.g., "dilate3x3")
 * @return Pointer to algorithm, or NULL if not found
 */
Algorithm* FindAlgorithm(const char* op_id);

/**
 * @brief Get algorithm by index
 *
 * Retrieves algorithm at specified index in registry.
 * Used for menu display and iteration.
 *
 * @param[in] index Zero-based index (0 to GetAlgorithmCount()-1)
 * @return Pointer to algorithm, or NULL if index out of range
 */
Algorithm* GetAlgorithmByIndex(int index);

/**
 * @brief Get number of registered algorithms
 *
 * @return Total count of algorithms in registry
 */
int GetAlgorithmCount(void);

/**
 * @brief List all registered algorithms to stdout
 *
 * Prints a formatted list of all available algorithms with
 * their indices, names, and IDs.
 */
void ListAlgorithms(void);
