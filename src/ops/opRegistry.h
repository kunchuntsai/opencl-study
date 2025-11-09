/**
 * @file opRegistry.h
 * @brief Header-only registry for automatic operation discovery
 */

#ifndef OPREGISTRY_H
#define OPREGISTRY_H

#include "opBase.h"
#include <vector>
#include <memory>
#include <functional>

/**
 * @class OpRegistry
 * @brief Singleton registry for automatic operation registration
 */
class OpRegistry {
public:
    /** @brief Factory function type */
    using OpFactory = std::function<std::unique_ptr<OpBase>()>;

    /** @brief Get singleton instance */
    static OpRegistry& instance();

    /** @brief Register an operation factory */
    void registerOp(const std::string& name, OpFactory factory);

    /** @brief Create instances of all registered operations */
    std::vector<std::unique_ptr<OpBase>> createAllOps() const;

    /** @brief Get number of registered operations */
    size_t count() const;

private:
    OpRegistry() = default;
    OpRegistry(const OpRegistry&) = delete;
    OpRegistry& operator=(const OpRegistry&) = delete;

    struct OpEntry {
        std::string name;
        OpFactory factory;
        OpEntry(const std::string& n, OpFactory f) : name(n), factory(f) {}
    };

    std::vector<OpEntry> operations_;
};

/**
 * @class OpRegistrar
 * @brief Helper for automatic static registration of operations
 */
template<typename OpType>
class OpRegistrar {
public:
    /** @brief Register operation with default constructor */
    explicit OpRegistrar(const std::string& name) {
        OpRegistry::instance().registerOp(name, []() {
            return std::unique_ptr<OpBase>(new OpType());
        });
    }

    /** @brief Register operation with custom factory */
    OpRegistrar(const std::string& name, OpRegistry::OpFactory factory) {
        OpRegistry::instance().registerOp(name, factory);
    }
};

// Inline implementations

inline OpRegistry& OpRegistry::instance() {
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
