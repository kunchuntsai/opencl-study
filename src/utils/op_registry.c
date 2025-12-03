#include "op_registry.h"

#include <stdio.h>
#include <string.h>

#include "config.h"

/* MISRA-C:2023 Rule 8.7: File scope variables should have internal linkage */
static Algorithm* registered_algorithms[MAX_ALGORITHMS];
static int algorithm_count = 0;

void RegisterAlgorithm(Algorithm* op) {
  if (op == NULL) {
    (void)fprintf(stderr, "Error: NULL algorithm pointer\n");
    return;
  }

  if (algorithm_count >= MAX_ALGORITHMS) {
    (void)fprintf(stderr, "Error: Maximum number of algorithms (%d) exceeded\n", MAX_ALGORITHMS);
    return;
  }

  registered_algorithms[algorithm_count] = op;
  algorithm_count++;
  (void)printf("Registered algorithm: %s (ID: %s)\n", op->name, op->id);
}

Algorithm* FindAlgorithm(const char* op_id) {
  int i;

  if (op_id == NULL) {
    return NULL;
  }

  for (i = 0; i < algorithm_count; i++) {
    if (registered_algorithms[i] != NULL) {
      if (strcmp(registered_algorithms[i]->id, op_id) == 0) {
        return registered_algorithms[i];
      }
    }
  }
  return NULL;
}

Algorithm* GetAlgorithmByIndex(int index) {
  if ((index < 0) || (index >= algorithm_count)) {
    return NULL;
  }
  return registered_algorithms[index];
}

int GetAlgorithmCount(void) {
  return algorithm_count;
}

void ListAlgorithms(void) {
  int i;
  int j;
  char config_path[256];
  Config config;
  KernelConfig* variants[MAX_KERNEL_CONFIGS];
  int variant_count;
  int parse_result;
  int get_variants_result;

  for (i = 0; i < algorithm_count; i++) {
    if (registered_algorithms[i] != NULL) {
      (void)printf("  %d - %s (ID: %s)\n", i, registered_algorithms[i]->name,
                   registered_algorithms[i]->id);

      /* Construct config path for this algorithm */
      (void)snprintf(config_path, sizeof(config_path), "config/%s.ini",
                     registered_algorithms[i]->id);

      /* Try to load and parse config for this algorithm */
      parse_result = ParseConfig(config_path, &config);
      if (parse_result == 0) {
        /* Set op_id from algorithm ID if not already set */
        if ((config.op_id[0] == '\0') || (strcmp(config.op_id, "config") == 0)) {
          (void)strncpy(config.op_id, registered_algorithms[i]->id, sizeof(config.op_id) - 1U);
          config.op_id[sizeof(config.op_id) - 1U] = '\0';
        }

        /* Get variants for this algorithm */
        get_variants_result =
            GetOpVariants(&config, registered_algorithms[i]->id, variants, &variant_count);
        if ((get_variants_result == 0) && (variant_count > 0)) {
          /* Display variants */
          for (j = 0; j < variant_count; j++) {
            (void)printf("      [%d] %s\n", j, variants[j]->variant_id);
          }
        }
      }
    }
  }
}
