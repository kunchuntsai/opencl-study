#ifndef OP_REGISTRY_H
#define OP_REGISTRY_H

#include "op_interface.h"

// Maximum number of algorithms that can be registered
#define MAX_ALGORITHMS 32

// Registration function (called at startup)
void register_algorithm(Algorithm* op);

// Lookup function by ID
Algorithm* find_algorithm(const char* op_id);

// Get algorithm by index (for menu display)
Algorithm* get_algorithm_by_index(int index);

// Get total number of registered algorithms
int get_algorithm_count(void);

// List all available algorithms
void list_algorithms(void);

#endif // OP_REGISTRY_H
