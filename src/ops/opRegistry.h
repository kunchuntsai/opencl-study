/**
 * @file opRegistry.h
 * @brief Header-only operation registry for automatic operation discovery
 *
 * This file provides a registry system that allows operations to
 * self-register at static initialization time. This eliminates the
 * need to manually add operations to main.cpp.
 *
 * @note This is a header-only implementation for simplicity and
 *       consistency with OpBase.
 *
 * @author OpenCL Study Project
 * @date 2025
 */

#ifndef OPREGISTRY_H
#define OPREGISTRY_H

#include "opBase.h"
#include <vector>
#include <memory>
#include <functional>

/**
 * @class OpRegistry
 * @brief Singleton registry for OpenCL operations
 *
 * Provides a central registry where operations can self-register.
 * Operations register themselves by providing a factory function that
 * creates instances on demand.
 *
 * Usage:
 * @code
 * // In operation implementation:
 * OpRegistry::instance().registerOp("Threshold", []() {
 *     return std::unique_ptr<OpBase>(new ThresholdOp());
 * });
 *
 * // In main.cpp:
 * auto ops = OpRegistry::instance().createAllOps();
 * @endcode
 */
class OpRegistry {
public:
    /**
     * @brief Factory function type for creating operations
     *
     * Factory functions take no parameters and return a unique_ptr
     * to a newly created operation instance.
     */
    using OpFactory = std::function<std::unique_ptr<OpBase>()>;

    /**
     * @brief Get the singleton registry instance
     *
     * @return Reference to the global registry instance
     */
    static OpRegistry& instance();

    /**
     * @brief Register an operation factory
     *
     * Adds an operation factory to the registry. The factory will be
     * called when createAllOps() is invoked.
     *
     * @param[in] name Operation name (for display purposes)
     * @param[in] factory Factory function that creates operation instances
     *
     * @note This is typically called at static initialization time
     * @note Duplicate names are allowed (both will be registered)
     */
    void registerOp(const std::string& name, OpFactory factory);

    /**
     * @brief Create instances of all registered operations
     *
     * Calls all registered factory functions to create operation instances.
     *
     * @return Vector of unique_ptrs to operation instances
     *
     * @note Creates fresh instances each time it's called
     * @note Operations are returned in registration order
     */
    std::vector<std::unique_ptr<OpBase>> createAllOps() const;

    /**
     * @brief Get the number of registered operations
     *
     * @return Number of operations in the registry
     */
    size_t count() const;

private:
    /**
     * @brief Private constructor for singleton pattern
     */
    OpRegistry() = default;

    /**
     * @brief Delete copy constructor
     */
    OpRegistry(const OpRegistry&) = delete;

    /**
     * @brief Delete assignment operator
     */
    OpRegistry& operator=(const OpRegistry&) = delete;

    /**
     * @struct OpEntry
     * @brief Registry entry containing operation metadata and factory
     */
    struct OpEntry {
        std::string name;    ///< Operation name
        OpFactory factory;   ///< Factory function to create instances

        OpEntry(const std::string& n, OpFactory f)
            : name(n), factory(f) {}
    };

    std::vector<OpEntry> operations_;  ///< List of registered operations
};

/**
 * @class OpRegistrar
 * @brief Helper class for automatic operation registration
 *
 * This class enables automatic registration at static initialization time.
 * Operations can use this to register themselves without manual registration.
 *
 * Usage:
 * @code
 * // In operation .cpp file, at global scope:
 * static OpRegistrar<ThresholdOp> registrar("Threshold");
 * @endcode
 *
 * @note The template parameter should be the operation class type
 * @note Constructor arguments are forwarded to the operation constructor
 */
template<typename OpType>
class OpRegistrar {
public:
    /**
     * @brief Register an operation with default constructor
     *
     * @param[in] name Operation name for display
     */
    explicit OpRegistrar(const std::string& name) {
        OpRegistry::instance().registerOp(name, []() {
            return std::unique_ptr<OpBase>(new OpType());
        });
    }

    /**
     * @brief Register an operation with custom factory
     *
     * Allows registration with a custom factory function that can
     * pass constructor arguments or perform custom initialization.
     *
     * @param[in] name Operation name for display
     * @param[in] factory Custom factory function
     */
    OpRegistrar(const std::string& name, OpRegistry::OpFactory factory) {
        OpRegistry::instance().registerOp(name, factory);
    }
};

// ============================================================================
// INLINE IMPLEMENTATION
// ============================================================================

inline OpRegistry& OpRegistry::instance() {
    // Singleton instance using Meyer's Singleton pattern
    // Thread-safe in C++11 and later
    static OpRegistry registry;
    return registry;
}

inline void OpRegistry::registerOp(const std::string& name, OpFactory factory) {
    operations_.emplace_back(name, factory);
}

inline std::vector<std::unique_ptr<OpBase>> OpRegistry::createAllOps() const {
    std::vector<std::unique_ptr<OpBase>> ops;
    ops.reserve(operations_.size());

    for (const auto& entry : operations_) {
        ops.push_back(entry.factory());
    }

    return ops;
}

inline size_t OpRegistry::count() const {
    return operations_.size();
}

#endif // OPREGISTRY_H
