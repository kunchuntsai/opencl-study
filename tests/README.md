# OpenCL Image Processing Framework - Test Documentation

## Overview

This document describes the test framework for the OpenCL Image Processing Framework. The test suite validates all implemented algorithms and their variants, ensuring correctness and tracking performance.

## Portability

The test framework is **fully portable** and auto-detects algorithms and variants from configuration files. When you add a new algorithm or variant:

1. Create the algorithm config file: `config/{algorithm}.json`
2. Add kernel variants under the `kernels` section
3. Tests will automatically detect and run them

**No manual updates to test scripts required!**

```bash
# View auto-detected configuration
./tests/detect_tests.sh

# Example output:
# Detected Algorithms and Variants:
# ==================================
# dilate3x3:
#   - v0
#   - v1
# gaussian5x5:
#   - v1
#   - v2
# ...
```

## Test Matrix

### Algorithms and Variants

| Algorithm | Variant | Description | Host Type | Verification |
|-----------|---------|-------------|-----------|--------------|
| **dilate3x3** | v0 | Standard implementation with struct params | standard | Exact match (tolerance=0) |
| **dilate3x3** | v1 | CL extension with local memory optimization | cl_extension | Exact match (tolerance=0) |
| **gaussian5x5** | v1f | Standard CL with float calculations | standard | Tolerance=1, error_rate=0.1% |
| **gaussian5x5** | v1 | CL extension with temp buffer optimization | cl_extension | Tolerance=1, error_rate=0.1% |
| **gaussian5x5** | v2 | Separable horizontal pass implementation | cl_extension | Tolerance=1, error_rate=0.1% |
| **gaussian5x5** | v3 | Custom sigma/radius parameters | standard | Tolerance=1, error_rate=0.1% |
| **relu** | v0 | Standard 2D work group variant | standard | Exact match (tolerance=0) |
| **relu** | v1 | CL extension with temp buffer | cl_extension | Exact match (tolerance=0) |
| **relu** | v6 | Single workgroup 1D processing | standard | Exact match (tolerance=0) |
| **relu** | v3 | Custom parameters variant | standard | Exact match (tolerance=0) |

**Total: 3 algorithms, 10 variants**

## What We Test

### 1. Functional Correctness

Each test verifies that the GPU kernel output matches the expected reference:

- **Golden Sample Verification**: Compare GPU output against pre-computed golden files or C reference implementation
- **Tolerance Handling**: For floating-point algorithms (gaussian5x5), allow small per-pixel differences
- **Error Rate Threshold**: Maximum percentage of pixels that can differ within tolerance

### 2. Algorithm Categories

#### Dilate3x3 (Morphological Dilation)
- **Purpose**: Find maximum value in 3x3 neighborhood for each pixel
- **Test Data**: 1920x1080 RGB image (3 channels)
- **Golden Source**: Pre-computed file (`test_data/dilate3x3/golden.bin`)
- **Verification**: Exact match required (integer operation)

#### Gaussian5x5 (Gaussian Blur)
- **Purpose**: Apply 5x5 Gaussian blur filter
- **Test Data**: 1920x1080 grayscale image (1 channel)
- **Golden Source**: C reference implementation
- **Verification**: Tolerance of ±1 per pixel (floating-point rounding)
- **Custom Buffers**: Gaussian kernel weights (kernel_x.bin, kernel_y.bin)

#### ReLU (Rectified Linear Unit)
- **Purpose**: Apply activation function: output = max(0, input)
- **Test Data**: 1920x1080 grayscale image (1 channel)
- **Golden Source**: Pre-computed file (`test_data/relu/golden.bin`)
- **Verification**: Exact match required (integer operation)

### 3. Performance Metrics

Each test captures:
- **GPU Execution Time**: Kernel execution time in milliseconds
- **CPU Reference Time**: C reference execution time (when applicable)
- **Speedup Factor**: CPU time / GPU time

## How We Test

### Test Execution Flow

```
1. Build Project
   └── Generate algorithm registry
   └── Compile OpenCL host application

2. For Each Algorithm Variant:
   ├── Load Configuration (config/{algorithm}.json)
   ├── Initialize OpenCL (platform, device, context)
   ├── Load Test Input Image
   ├── Execute C Reference (if golden_source=c_ref)
   ├── Compile & Execute OpenCL Kernel
   ├── Read GPU Results
   ├── Verify Output Against Golden Sample
   └── Record Pass/Fail and Timing

3. Generate Report
   └── Summary statistics
   └── Detailed results per test
   └── Performance ranking
   └── Failure details
```

### Verification Logic

```
For each pixel in output:
    difference = |gpu_output[pixel] - golden[pixel]|

    if difference > tolerance:
        error_count++

error_rate = error_count / total_pixels

if error_rate > error_rate_threshold:
    FAIL
else:
    PASS
```

## Running Tests

### Quick Start

```bash
# Build and run all tests
./scripts/build.sh --test

# Run smoke tests only (quick verification)
./scripts/build.sh --test smoke

# Run full regression suite
./scripts/build.sh --test regression
```

### Using Test Scripts Directly

```bash
# Run all tests with text report
./tests/run_tests.sh

# Run with JSON output
./tests/run_tests.sh --json

# Run with JUnit XML (for CI/CD)
./tests/run_tests.sh --junit

# Run specific algorithm
./tests/run_tests.sh -a dilate3x3

# Run specific variant
./tests/run_tests.sh -a relu -V 0

# Skip build step
./tests/run_tests.sh --no-build

# Verbose output
./tests/run_tests.sh --verbose
```

### CI/CD Integration

```bash
# Smoke tests for quick CI feedback
./tests/ci_test.sh smoke

# Full regression for nightly builds
./tests/ci_test.sh regression

# Performance benchmarks
./tests/ci_test.sh benchmark
```

## Test Levels

| Level | Variants Tested | Purpose |
|-------|-----------------|---------|
| **smoke** | dilate3x3:0, gaussian5x5:1f, relu:0 | Quick sanity check |
| **regression** | All 10 variants | Full correctness verification |
| **benchmark** | Performance-critical variants | Performance tracking |
| **all** | All 10 variants | Complete test suite |

## Output Formats

### Text Report (Default)
```
==============================================
OpenCL Image Processing Framework Test Report
==============================================

SUMMARY
----------------------------------------------
Total Tests:   10
Passed:        8
Failed:        2
Pass Rate:     80.0%

DETAILED RESULTS
----------------------------------------------
Test                 Status   GPU Time     Speedup
dilate3x3:v0         PASS     0.004ms      x
dilate3x3:v1         PASS     0.011ms      x
...
```

### JSON Report
```json
{
  "summary": {
    "total": 10,
    "passed": 8,
    "failed": 2,
    "pass_rate": 80.0
  },
  "tests": [
    {
      "id": "dilate3x3:v0",
      "status": "PASS",
      "gpu_time_ms": 0.004
    }
  ]
}
```

### JUnit XML (CI/CD)
```xml
<testsuites name="OpenCL Tests" tests="10" failures="2">
  <testsuite name="dilate3x3" tests="2" failures="0">
    <testcase name="v0" classname="dilate3x3" />
    <testcase name="v1" classname="dilate3x3" />
  </testsuite>
</testsuites>
```

## Report Locations

- **Text Reports**: `tests/reports/test_report_YYYYMMDD_HHMMSS.txt`
- **JSON Reports**: `tests/reports/test_report_YYYYMMDD_HHMMSS.json`
- **JUnit XML**: `tests/reports/test_report_YYYYMMDD_HHMMSS.xml`
- **CI Artifacts**: `artifacts/`

## Test Data

### Input Images

| File | Algorithm | Size | Format |
|------|-----------|------|--------|
| `test_data/dilate3x3/input.bin` | dilate3x3 | 1920x1080x3 | Raw RGB |
| `test_data/gaussian5x5/input.bin` | gaussian5x5 | 1920x1080x1 | Raw Grayscale |
| `test_data/relu/input.bin` | relu | 1920x1080x1 | Raw Grayscale |

### Golden References

| File | Source | Description |
|------|--------|-------------|
| `test_data/dilate3x3/golden.bin` | Pre-computed | Expected output for dilate3x3 |
| `test_data/relu/golden.bin` | Pre-computed | Expected output for relu |
| (generated) | C reference | gaussian5x5 uses C reference at runtime |

### Kernel Weights (Gaussian only)

| File | Description |
|------|-------------|
| `test_data/gaussian5x5/kernel_x.bin` | 5-element horizontal Gaussian weights |
| `test_data/gaussian5x5/kernel_y.bin` | 5-element vertical Gaussian weights |

## Configuration

### Test Configuration File: `tests/test_config.sh`

Defines:
- Algorithm and variant lists
- Verification thresholds
- Performance baselines
- Test categories (smoke, regression, benchmark)

### Algorithm Configuration: `config/{algorithm}.json`

Each algorithm has a JSON config specifying:
- Input/output image references
- Verification settings (tolerance, error_rate_threshold, golden_source)
- Custom buffers and scalars
- Kernel variants with work sizes

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | All tests passed |
| 1 | One or more tests failed |

## CI/CD Integration Examples

### GitHub Actions

```yaml
jobs:
  test:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3

      - name: Build and Test
        run: ./scripts/build.sh --test

      - name: Upload Test Reports
        uses: actions/upload-artifact@v3
        with:
          name: test-reports
          path: tests/reports/
```

### GitLab CI

```yaml
test:
  script:
    - ./scripts/build.sh --test
  artifacts:
    reports:
      junit: tests/reports/*.xml
    paths:
      - tests/reports/
```

## Troubleshooting

### Common Issues

1. **Test shows UNKNOWN status**
   - The executable output format may have changed
   - Check `./build/opencl_host <algo> <variant>` output manually

2. **Configuration errors**
   - Verify `config/{algorithm}.json` has required custom buffers
   - Check that test data files exist in `test_data/`

3. **Verification failures**
   - Check tolerance settings in config
   - Regenerate golden samples if algorithm changed

### Debug Mode

```bash
# Run with verbose output
./tests/run_tests.sh --verbose

# Run single test manually
./build/opencl_host dilate3x3 0
```

## Adding New Tests

When adding a new algorithm, the test framework **auto-detects** it:

1. Create algorithm config: `config/{algorithm}.json`
   - Must have a `kernels` section with variant definitions
2. Add C reference: `examples/{algorithm}/c_ref/{algorithm}_ref.c`
3. Add OpenCL kernels: `examples/{algorithm}/cl/*.cl`
4. Generate test data: `test_data/{algorithm}/input.bin`
5. **Done!** Tests will automatically detect and run the new algorithm

**No manual updates to test scripts required!**

To verify detection:
```bash
./tests/detect_tests.sh
```

## File Structure

```
tests/
├── README.md           # This documentation
├── detect_tests.sh     # Auto-detection of algorithms/variants
├── run_tests.sh        # Main test runner
├── ci_test.sh          # CI/CD integration script
├── test_config.sh      # Test configuration
├── generate_report.sh  # Report generator
└── reports/            # Generated reports
    ├── test_report_*.txt
    ├── test_report_*.json
    └── test_report_*.xml
```

## Auto-Detection Script

The `detect_tests.sh` script scans `config/*.json` to find all algorithms and their variants:

```bash
# List all detected tests
./tests/detect_tests.sh

# Get TEST_MATRIX format
./tests/detect_tests.sh --matrix

# Count total tests
./tests/detect_tests.sh --count

# Get smoke test variants (first of each algorithm)
./tests/detect_tests.sh --smoke

# Get all tests in algorithm:variant format
./tests/detect_tests.sh --all
```

### How Auto-Detection Works

1. Scans `config/*.json` files (excludes inputs.json, outputs.json, template.json)
2. For each valid algorithm config (has `kernels` section):
   - Extracts algorithm name from filename
   - Extracts variant IDs from `kernels` keys
3. Exports `DETECTED_ALGORITHMS`, `DETECTED_TEST_MATRIX`, `DETECTED_TEST_COUNT`

### Required Config Structure

For auto-detection to work, algorithm configs must have this structure:

```json
{
  "kernels": {
    "v0": { "description": "...", ... },
    "v1": { "description": "...", ... }
  }
}
```
