/**
 * @file android_runner.h
 * @brief Android execution pipeline interface
 *
 * This runner is designed for Android deployment where:
 * - Program binaries are pre-compiled (by desktop opencl-study)
 * - No C reference implementation needed
 * - Uses cl_extension APIs for buffer management
 */

#ifndef ANDROID_RUNNER_H
#define ANDROID_RUNNER_H

#include "utils/config.h"

/**
 * @brief Run algorithm on Android using pre-compiled program binary
 *
 * Unlike the desktop algorithm_runner which compiles kernels from source,
 * this runner:
 * 1. Loads pre-compiled program binary
 * 2. Sets up buffers using cl_extension API
 * 3. Executes kernel
 * 4. Returns results
 *
 * @param[in] argc Argument count from main()
 * @param[in] argv Argument vector from main()
 * @return 0 on success, non-zero on error
 */
int AndroidRunner(int argc, char** argv);

#endif /* ANDROID_RUNNER_H */
