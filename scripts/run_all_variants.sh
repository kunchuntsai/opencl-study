#!/bin/bash

# Run All Variants - Simple Benchmark Script
# Executes all algorithm variants and shows results

set -e  # Exit on error

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/out"
EXECUTABLE="$BUILD_DIR/opencl_host"

# Algorithm configurations
# Format: "algorithm:variants" where variants are space-separated
ALGORITHMS=(
    "dilate3x3:0 1"
    "gaussian5x5:0 1 2"
)

# Result storage
declare -a RESULTS

# Check if executable exists
check_executable() {
    if [ ! -f "$EXECUTABLE" ]; then
        echo -e "${RED}Error: Executable not found at $EXECUTABLE${NC}"
        echo "Please run build.sh first."
        exit 1
    fi
}

# Parse timing and verification from output
parse_output() {
    local output="$1"
    local ref_time=""
    local gpu_time=""
    local speedup=""
    local verification=""
    local max_error=""

    # Extract metrics using grep and awk
    ref_time=$(echo "$output" | grep "C Reference time:" | awk '{print $4}')
    gpu_time=$(echo "$output" | grep "OpenCL GPU time:" | awk '{print $4}')
    speedup=$(echo "$output" | grep "Speedup:" | awk '{print $2}')
    verification=$(echo "$output" | grep "Verification:" | awk '{print $2}')
    max_error=$(echo "$output" | grep "Max error:" | awk '{print $3}')

    # Return as semicolon-separated values
    echo "${ref_time};${gpu_time};${speedup};${verification};${max_error}"
}

# Run a single variant
run_variant() {
    local algo="$1"
    local variant="$2"

    echo -ne "${BLUE}Running: ${algo} (variant ${variant})... ${NC}"

    # Run the algorithm in background and show spinner
    local output
    local exit_code=0
    local temp_output=$(mktemp)
    local start_time=$(date +%s)

    # Run in background
    "$EXECUTABLE" "$algo" "$variant" > "$temp_output" 2>&1 &
    local pid=$!

    # Show spinner while running
    local spin='-\|/'
    local i=0
    while kill -0 $pid 2>/dev/null; do
        i=$(( (i+1) %4 ))
        local elapsed=$(($(date +%s) - start_time))
        printf "\r${BLUE}Running: ${algo} (variant ${variant})... ${spin:$i:1} [${elapsed}s]${NC}"
        sleep 0.1
    done

    # Get exit code
    wait $pid
    exit_code=$?
    local end_time=$(date +%s)
    local total_time=$((end_time - start_time))

    # Read output
    output=$(<"$temp_output")
    rm -f "$temp_output"

    # Clear spinner line and rewrite
    printf "\r\033[K"  # Clear entire line

    if [ $exit_code -ne 0 ]; then
        echo -e "${RED}  Failed with exit code $exit_code${NC}"
        RESULTS+=("${algo};v${variant};ERROR;-;-;-;-")
        return 1
    fi

    # Parse results
    local parsed=$(parse_output "$output")
    IFS=';' read -r ref_time gpu_time speedup verification max_error <<< "$parsed"

    # Validate parsed results
    if [ -z "$ref_time" ] || [ -z "$gpu_time" ]; then
        echo -e "${YELLOW}  Warning: Could not parse timing data${NC}"
        RESULTS+=("${algo};v${variant};PARSE_ERROR;-;-;-;-")
        return 1
    fi

    # Display results
    if [ "$verification" = "PASSED" ]; then
        echo -e "${GREEN}  ✓ PASSED${NC} | Ref: ${ref_time}ms | GPU: ${gpu_time}ms | Speedup: ${speedup} | Error: ${max_error}"
    else
        echo -e "${RED}  ✗ FAILED${NC} | Ref: ${ref_time}ms | GPU: ${gpu_time}ms | Speedup: ${speedup} | Error: ${max_error}"
    fi

    # Store results
    RESULTS+=("${algo};v${variant};${verification};${ref_time};${gpu_time};${speedup};${max_error}")

    return 0
}

# Generate summary
generate_summary() {
    echo ""
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN}Summary${NC}"
    echo -e "${CYAN}========================================${NC}"

    local total=0
    local passed=0
    local failed=0
    local errors=0

    for result in "${RESULTS[@]}"; do
        IFS=';' read -r algo variant status ref_time gpu_time speedup max_error <<< "$result"
        ((total++))
        case "$status" in
            PASSED) ((passed++)) ;;
            FAILED) ((failed++)) ;;
            *) ((errors++)) ;;
        esac
    done

    echo -e "Total Variants:    ${total}"
    echo -e "${GREEN}Passed:${NC}            ${passed}"
    if [ $failed -gt 0 ]; then
        echo -e "${RED}Failed:${NC}            ${failed}"
    fi
    if [ $errors -gt 0 ]; then
        echo -e "${YELLOW}Errors:${NC}            ${errors}"
    fi

    echo ""
    echo -e "${CYAN}Performance Rankings (by GPU time)${NC}"
    echo -e "${CYAN}----------------------------------------${NC}"
    printf "%-20s | %-10s | %-12s | %-10s\n" "Algorithm" "Variant" "GPU Time" "Speedup"
    echo "----------------------------------------------------------------"

    for result in "${RESULTS[@]}"; do
        IFS=';' read -r algo variant status ref_time gpu_time speedup max_error <<< "$result"
        if [ "$status" = "PASSED" ] && [ -n "$gpu_time" ] && [ "$gpu_time" != "-" ]; then
            echo "$gpu_time;$algo;$variant;$speedup"
        fi
    done | sort -n -t';' -k1 | while IFS=';' read -r gpu_time algo variant speedup; do
        printf "%-20s | %-10s | %-10s ms | %-10s\n" "$algo" "$variant" "$gpu_time" "$speedup"
    done
    echo ""
}

# Main execution
main() {
    check_executable

    echo -e "${CYAN}=== OpenCL Algorithm Benchmark ===${NC}\n"

    # Count total variants
    local total_variants=0
    for algo_entry in "${ALGORITHMS[@]}"; do
        local variants="${algo_entry#*:}"
        for variant in $variants; do
            ((total_variants++))
        done
    done

    echo -e "${CYAN}Total tests to run: $total_variants${NC}\n"

    local overall_start=$(date +%s)

    # Iterate through all algorithms and variants
    local current_test=0
    for algo_entry in "${ALGORITHMS[@]}"; do
        # Split "algorithm:variants" format
        local algo="${algo_entry%%:*}"
        local variants="${algo_entry#*:}"

        echo -e "${YELLOW}=== Algorithm: $algo ===${NC}"

        for variant in $variants; do
            ((current_test++))
            echo -ne "${CYAN}[Test $current_test/$total_variants] ${NC}"
            run_variant "$algo" "$variant"
            echo ""
        done
        echo ""
    done

    local overall_end=$(date +%s)
    local overall_time=$((overall_end - overall_start))
    local minutes=$((overall_time / 60))
    local seconds=$((overall_time % 60))

    echo -e "${CYAN}All tests completed in ${minutes}m ${seconds}s${NC}"

    generate_summary

    echo -e "${GREEN}✓ Benchmark complete!${NC}\n"
}

# Run main function
main "$@"
