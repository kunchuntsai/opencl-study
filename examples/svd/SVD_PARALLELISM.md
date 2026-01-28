# SVD Parallelism Analysis

## Overview

This SVD implementation uses a **data-parallel (SPMD)** approach with an **embarrassingly parallel** pattern - each pixel's SVD computation is completely independent.

## Work Distribution

| Aspect | CPU Reference | GPU OpenCL |
|--------|---------------|------------|
| Threads | 1 (sequential) | ~2M (1920x1088) |
| Work per thread | All pixels | 1 pixel |
| Workgroup | N/A | 16x16 = 256 threads |

```
GPU Thread Mapping:
+-------------------------------------+
| Image (1920 x 1088 pixels)          |
|  +-----+-----+-----+-----+---       |
|  |WG0  |WG1  |WG2  |WG3  |...       |  Each 16x16 workgroup
|  +-----+-----+-----+-----+---       |  processes 256 pixels
|  |WG16 |WG17 |WG18 |WG19 |...       |  in parallel
|  +-----+-----+-----+-----+---       |
+-------------------------------------+
```

## Why Embarrassingly Parallel

Each thread independently:
1. Reads its own 3x3 neighborhood (READ-ONLY)
2. Computes gradients locally
3. Builds structure tensor locally
4. Computes SVD locally
5. Writes to unique output location (NO CONFLICTS)

**Key properties:**
- **No inter-thread dependencies** - pixel A's SVD doesn't depend on pixel B's result
- **No synchronization needed** - no barriers, mutexes, or atomics
- **No communication** - threads don't share intermediate results

## Memory Access Pattern

### Input (Read-Only, Overlapping)

Each thread reads a 3x3 neighborhood for Scharr gradients:
```
(x-1,y-1)  (x,y-1)  (x+1,y-1)
(x-1,y)    (x,y)    (x+1,y)      <- 9 pixels per thread
(x-1,y+1)  (x,y+1)  (x+1,y+1)
```

Adjacent threads share input data (good for GPU cache).

### Output (Write-Only, No Conflicts)

Each thread writes to a unique index `idx = y * width + x`.

## Computation Pipeline

```
+---------------------------------------------------+
|  STEP 1: Scharr Gradient Computation              |
|  Read 3x3 neighborhood -> Compute Ix, Iy          |
|  Kernel: [-3 0 3; -10 0 10; -3 0 3] / 32          |
+---------------------------------------------------+
                        |
                        v
+---------------------------------------------------+
|  STEP 2: Structure Tensor Construction            |
|  M = [Ix^2    Ix*Iy]  (symmetric 2x2 matrix)      |
|      [Ix*Iy   Iy^2 ]                              |
+---------------------------------------------------+
                        |
                        v
+---------------------------------------------------+
|  STEP 3: Closed-Form 2x2 SVD                      |
|  lambda = (a+c)/2 +/- sqrt((a-c)^2/4 + b^2)       |
|  sigma = sqrt(lambda)                             |
|  theta = 0.5 * atan2(2b, a-c)                     |
+---------------------------------------------------+
```

## Kernel Variants

| Kernel | File | Description |
|--------|------|-------------|
| `svd` | `svd_1.cl` | Per-pixel structure tensor SVD |
| `svd_jacobi` | `svd_1.cl` | Full U, S, V matrix decomposition |
| `svd_patch` | `svd_2.cl` | 3x3 Gaussian-weighted window SVD |

## Performance Characteristics

| Factor | Impact |
|--------|--------|
| Parallelism degree | ~2M independent tasks |
| Load balance | Perfect - identical work per thread |
| Memory coalescing | Good - adjacent threads access adjacent memory |
| Divergence | Minimal - only border pixel branching |
| Arithmetic intensity | Low - memory bound |

## Summary

The SVD achieves parallelism through **spatial decomposition** - each GPU thread computes one pixel's SVD independently. This is a classic **map operation**:

```
Parallel SVD = map(svd_2x2 . build_tensor . scharr_gradients, all_pixels)
```

No reduction, scan, or gather operations are needed because:
1. Each output depends only on a local 3x3 input neighborhood
2. The 2x2 SVD has a closed-form solution (no iteration)
3. Pixels don't communicate results to each other
