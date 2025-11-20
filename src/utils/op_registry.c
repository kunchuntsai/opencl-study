#include "op_registry.h"
#include <stdio.h>
#include <string.h>

static Algorithm* registered_algorithms[MAX_ALGORITHMS];
static int algorithm_count = 0;

void register_algorithm(Algorithm* op) {
    if (algorithm_count >= MAX_ALGORITHMS) {
        fprintf(stderr, "Error: Maximum number of algorithms (%d) exceeded\n", MAX_ALGORITHMS);
        return;
    }

    registered_algorithms[algorithm_count++] = op;
    printf("Registered algorithm: %s (ID: %s)\n", op->name, op->id);
}

Algorithm* find_algorithm(const char* op_id) {
    for (int i = 0; i < algorithm_count; i++) {
        if (strcmp(registered_algorithms[i]->id, op_id) == 0) {
            return registered_algorithms[i];
        }
    }
    return NULL;
}

Algorithm* get_algorithm_by_index(int index) {
    if (index < 0 || index >= algorithm_count) {
        return NULL;
    }
    return registered_algorithms[index];
}

int get_algorithm_count(void) {
    return algorithm_count;
}

void list_algorithms(void) {
    for (int i = 0; i < algorithm_count; i++) {
        printf("%d. %s\n", i + 1, registered_algorithms[i]->name);
    }
}
