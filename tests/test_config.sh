#!/bin/bash
#
# Test Configuration File
#
# This file defines test configuration for the OpenCL Image Processing Framework.
# Algorithms and variants are auto-detected from config/*.json files.
#
# Source this file to access test configuration and helper functions.
#

# Determine script location
_CONFIG_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_CONFIG_PROJECT_ROOT="$(dirname "$_CONFIG_SCRIPT_DIR")"

# =============================================================================
# Auto-Detection (from detect_tests.sh)
# =============================================================================

# Source detection script if not already sourced
if [ -z "$DETECTED_ALGORITHMS" ]; then
    source "${_CONFIG_SCRIPT_DIR}/detect_tests.sh"
fi

# All implemented algorithms (auto-detected)
ALGORITHMS="$DETECTED_ALGORITHMS"

# =============================================================================
# Test Categories (auto-generated)
# =============================================================================

# Quick smoke test - first variant of each algorithm
SMOKE_TEST_VARIANTS=$(get_smoke_tests)

# Full regression test - all variants
REGRESSION_TEST_VARIANTS=$(get_all_tests)

# Performance-critical variants for benchmarking
# Auto-select variants with "optimized" or specific patterns, or use all
BENCHMARK_VARIANTS=$(get_all_tests)

# =============================================================================
# Variant Descriptions (from config files)
# =============================================================================

get_variant_description() {
    local test_id="$1"
    local algorithm="${test_id%%:*}"
    local variant="${test_id#*:}"

    # Remove 'v' prefix if present
    variant="${variant#v}"

    local config_file="${_CONFIG_PROJECT_ROOT}/config/${algorithm}.json"

    if [ ! -f "$config_file" ]; then
        echo "No description available"
        return
    fi

    # Extract description from config
    python3 -c "
import json
import sys

try:
    with open('$config_file') as f:
        config = json.load(f)

    kernels = config.get('kernels', {})

    # Try with and without 'v' prefix
    for key in ['v$variant', '$variant']:
        if key in kernels:
            desc = kernels[key].get('description', 'No description')
            print(desc)
            sys.exit(0)

    print('No description available')
except Exception:
    print('No description available')
" 2>/dev/null
}

# =============================================================================
# Verification Thresholds (from config files)
# =============================================================================

get_verification_tolerance() {
    local algorithm="$1"
    local config_file="${_CONFIG_PROJECT_ROOT}/config/${algorithm}.json"

    if [ ! -f "$config_file" ]; then
        echo "0"
        return
    fi

    python3 -c "
import json
try:
    with open('$config_file') as f:
        config = json.load(f)
    tolerance = config.get('verification', {}).get('tolerance', 0)
    print(tolerance)
except:
    print('0')
" 2>/dev/null
}

get_error_rate_threshold() {
    local algorithm="$1"
    local config_file="${_CONFIG_PROJECT_ROOT}/config/${algorithm}.json"

    if [ ! -f "$config_file" ]; then
        echo "0"
        return
    fi

    python3 -c "
import json
try:
    with open('$config_file') as f:
        config = json.load(f)
    threshold = config.get('verification', {}).get('error_rate_threshold', 0)
    print(threshold)
except:
    print('0')
" 2>/dev/null
}

# =============================================================================
# Performance Baselines
# =============================================================================

# Default performance regression threshold (percentage)
PERFORMANCE_REGRESSION_THRESHOLD=20

# Get performance baseline for a test (can be overridden per-project)
get_performance_baseline() {
    local test_id="$1"
    # Default baseline - can be customized per project
    echo "10.0"
}

# =============================================================================
# Helper Functions
# =============================================================================

# Get variants for a specific algorithm (wrapper for detect_tests.sh)
get_algorithm_variants() {
    local algorithm="$1"
    get_variants "$algorithm"
}

# Get total test count
get_total_test_count() {
    count_tests
}

# Check if a test is in the smoke test set
is_smoke_test() {
    local test_id="$1"
    for t in $SMOKE_TEST_VARIANTS; do
        if [ "$t" = "$test_id" ]; then
            return 0
        fi
    done
    return 1
}

# =============================================================================
# Display Functions
# =============================================================================

# Print detected configuration
print_test_config() {
    echo "Test Configuration (Auto-Detected)"
    echo "===================================="
    echo ""
    echo "Algorithms: $ALGORITHMS"
    echo ""
    echo "Smoke Tests: $SMOKE_TEST_VARIANTS"
    echo ""
    echo "All Tests: $REGRESSION_TEST_VARIANTS"
    echo ""
    echo "Total: $(count_tests) test cases"
}

# =============================================================================
# Run if executed directly
# =============================================================================

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    print_test_config
fi
