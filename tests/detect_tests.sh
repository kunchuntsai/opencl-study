#!/bin/bash
#
# Auto-detect algorithms and variants from config files
#
# This script scans the config/ directory to find all algorithm configurations
# and extracts available variants from each algorithm's JSON config.
#
# Usage:
#   source tests/detect_tests.sh
#   # Then use: DETECTED_ALGORITHMS, get_variants(), get_all_tests()
#
# Or run directly:
#   ./tests/detect_tests.sh              # List all algorithms and variants
#   ./tests/detect_tests.sh --matrix     # Output TEST_MATRIX format
#   ./tests/detect_tests.sh --count      # Count total tests
#

# Determine project root
if [ -n "$PROJECT_ROOT" ]; then
    _DETECT_PROJECT_ROOT="$PROJECT_ROOT"
else
    _DETECT_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    _DETECT_PROJECT_ROOT="$(dirname "$_DETECT_SCRIPT_DIR")"
fi

CONFIG_DIR="${_DETECT_PROJECT_ROOT}/config"

# Files to exclude from algorithm detection
EXCLUDED_CONFIGS="inputs.json outputs.json template.json"

# =============================================================================
# Detection Functions
# =============================================================================

# Detect all algorithm names from config files
detect_algorithms() {
    local algorithms=""

    for config_file in "$CONFIG_DIR"/*.json; do
        if [ ! -f "$config_file" ]; then
            continue
        fi

        local filename=$(basename "$config_file")

        # Skip excluded files
        local skip=0
        for excluded in $EXCLUDED_CONFIGS; do
            if [ "$filename" = "$excluded" ]; then
                skip=1
                break
            fi
        done

        if [ $skip -eq 1 ]; then
            continue
        fi

        # Extract algorithm name (remove .json extension)
        local algo_name="${filename%.json}"

        # Verify it has a kernels section (is a valid algorithm config)
        if python3 -c "import json; d=json.load(open('$config_file')); exit(0 if 'kernels' in d else 1)" 2>/dev/null; then
            if [ -n "$algorithms" ]; then
                algorithms="$algorithms $algo_name"
            else
                algorithms="$algo_name"
            fi
        fi
    done

    echo "$algorithms"
}

# Get variants for a specific algorithm
get_variants() {
    local algorithm="$1"
    local config_file="${CONFIG_DIR}/${algorithm}.json"

    if [ ! -f "$config_file" ]; then
        echo ""
        return 1
    fi

    # Extract variant keys from kernels section, remove 'v' prefix for CLI
    python3 -c "
import json
import sys

try:
    with open('$config_file') as f:
        config = json.load(f)

    kernels = config.get('kernels', {})
    variants = []

    for key in kernels.keys():
        # Remove 'v' prefix if present
        if key.startswith('v'):
            variants.append(key[1:])
        else:
            variants.append(key)

    # Sort variants (handle mixed numeric and alphanumeric)
    def sort_key(v):
        try:
            return (0, int(v))
        except ValueError:
            return (1, v)

    variants.sort(key=sort_key)
    print(' '.join(variants))
except Exception as e:
    sys.exit(1)
" 2>/dev/null
}

# Get all tests in "algorithm:variant" format
get_all_tests() {
    local algorithms=$(detect_algorithms)
    local all_tests=""

    for algo in $algorithms; do
        local variants=$(get_variants "$algo")
        for var in $variants; do
            local test_id="${algo}:${var}"
            if [ -n "$all_tests" ]; then
                all_tests="$all_tests $test_id"
            else
                all_tests="$test_id"
            fi
        done
    done

    echo "$all_tests"
}

# Generate TEST_MATRIX format (algorithm:var1,var2,...)
generate_test_matrix() {
    local algorithms=$(detect_algorithms)
    local matrix=""

    for algo in $algorithms; do
        local variants=$(get_variants "$algo")
        # Convert space-separated to comma-separated
        local variants_csv=$(echo "$variants" | tr ' ' ',')

        if [ -n "$variants_csv" ]; then
            local entry="${algo}:${variants_csv}"
            if [ -n "$matrix" ]; then
                matrix="$matrix $entry"
            else
                matrix="$entry"
            fi
        fi
    done

    echo "$matrix"
}

# Count total number of tests
count_tests() {
    local count=0
    local algorithms=$(detect_algorithms)

    for algo in $algorithms; do
        local variants=$(get_variants "$algo")
        for var in $variants; do
            count=$((count + 1))
        done
    done

    echo "$count"
}

# Get first variant for each algorithm (for smoke tests)
get_smoke_tests() {
    local algorithms=$(detect_algorithms)
    local smoke_tests=""

    for algo in $algorithms; do
        local variants=$(get_variants "$algo")
        local first_var=$(echo "$variants" | awk '{print $1}')

        if [ -n "$first_var" ]; then
            local test_id="${algo}:${first_var}"
            if [ -n "$smoke_tests" ]; then
                smoke_tests="$smoke_tests $test_id"
            else
                smoke_tests="$test_id"
            fi
        fi
    done

    echo "$smoke_tests"
}

# =============================================================================
# Export for sourcing
# =============================================================================

# Auto-detect and export when sourced
DETECTED_ALGORITHMS=$(detect_algorithms)
DETECTED_TEST_MATRIX=$(generate_test_matrix)
DETECTED_TEST_COUNT=$(count_tests)

export DETECTED_ALGORITHMS
export DETECTED_TEST_MATRIX
export DETECTED_TEST_COUNT

# =============================================================================
# CLI Mode
# =============================================================================

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    # Script is being run directly (not sourced)
    case "${1:-}" in
        --matrix)
            generate_test_matrix
            ;;
        --count)
            count_tests
            ;;
        --smoke)
            get_smoke_tests
            ;;
        --all)
            get_all_tests
            ;;
        --help|-h)
            echo "Usage: $0 [option]"
            echo ""
            echo "Options:"
            echo "  (none)     List all algorithms and their variants"
            echo "  --matrix   Output TEST_MATRIX format"
            echo "  --count    Count total number of tests"
            echo "  --smoke    List smoke test variants (first of each algo)"
            echo "  --all      List all tests in algorithm:variant format"
            echo "  --help     Show this help"
            ;;
        *)
            echo "Detected Algorithms and Variants:"
            echo "=================================="

            algorithms=$(detect_algorithms)
            total=0

            for algo in $algorithms; do
                variants=$(get_variants "$algo")
                var_count=$(echo "$variants" | wc -w | tr -d ' ')
                total=$((total + var_count))

                echo ""
                echo "$algo:"
                for var in $variants; do
                    echo "  - v$var"
                done
            done

            echo ""
            echo "=================================="
            echo "Total: $(echo "$algorithms" | wc -w | tr -d ' ') algorithms, $total variants"
            ;;
    esac
fi
