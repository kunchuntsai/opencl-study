#!/bin/bash
#
# OpenCL Image Processing Framework - Comprehensive Test Runner
#
# This script runs all implemented algorithms and variants, generates
# a comprehensive test report suitable for CI/CD pipelines.
#
# Usage:
#   ./tests/run_tests.sh [options]
#
# Options:
#   --help, -h          Show this help message
#   --verbose, -v       Enable verbose output
#   --json              Output results in JSON format
#   --junit             Output results in JUnit XML format (for CI/CD)
#   --algorithm, -a     Run specific algorithm only (e.g., -a dilate3x3)
#   --variant, -V       Run specific variant only (e.g., -V 0)
#   --report-dir, -r    Directory for reports (default: tests/reports)
#   --no-build          Skip build step
#   --quiet, -q         Minimal output (only show failures)
#

set -e

# =============================================================================
# Configuration
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"
EXECUTABLE="${BUILD_DIR}/opencl_host"
CONFIG_DIR="${PROJECT_ROOT}/config"
REPORT_DIR="${PROJECT_ROOT}/tests/reports"

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Source auto-detection script
source "${SCRIPT_DIR}/detect_tests.sh"

# Test configuration - Auto-detected from config/*.json files
# Format: algorithm:variant1,variant2,...
TEST_MATRIX="$DETECTED_TEST_MATRIX"

# Fallback if detection fails
if [ -z "$TEST_MATRIX" ]; then
    log_error "Failed to detect algorithms from config files"
    exit 1
fi

# Command line options
VERBOSE=0
OUTPUT_FORMAT="text"
FILTER_ALGORITHM=""
FILTER_VARIANT=""
SKIP_BUILD=0
QUIET=0

# Test statistics
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Test results stored in temp file for portability
RESULTS_FILE=""

# =============================================================================
# Helper Functions
# =============================================================================

print_usage() {
    cat << EOF
Usage: $0 [options]

OpenCL Image Processing Framework - Comprehensive Test Runner

Options:
  --help, -h          Show this help message
  --verbose, -v       Enable verbose output
  --json              Output results in JSON format
  --junit             Output results in JUnit XML format (for CI/CD)
  --algorithm, -a     Run specific algorithm only (e.g., -a dilate3x3)
  --variant, -V       Run specific variant only (e.g., -V 0)
  --report-dir, -r    Directory for reports (default: tests/reports)
  --no-build          Skip build step
  --quiet, -q         Minimal output (only show failures)

Examples:
  $0                          # Run all tests
  $0 --json                   # Output JSON report
  $0 -a dilate3x3             # Test only dilate3x3 algorithm
  $0 -a relu -V 0             # Test only relu variant v0
  $0 --junit -r ./ci-reports  # Generate JUnit XML for CI/CD

Algorithms and Variants:
  dilate3x3:   v0, v1
  gaussian5x5: v1f, v1, v2, v3
  relu:        v0, v1, v6, v3
EOF
}

log_info() {
    if [ $QUIET -eq 0 ]; then
        printf "${BLUE}[INFO]${NC} %s\n" "$1"
    fi
}

log_success() {
    if [ $QUIET -eq 0 ]; then
        printf "${GREEN}[PASS]${NC} %s\n" "$1"
    fi
}

log_error() {
    printf "${RED}[FAIL]${NC} %s\n" "$1"
}

log_warning() {
    if [ $QUIET -eq 0 ]; then
        printf "${YELLOW}[WARN]${NC} %s\n" "$1"
    fi
}

log_verbose() {
    if [ $VERBOSE -eq 1 ]; then
        printf "${CYAN}[DEBUG]${NC} %s\n" "$1"
    fi
}

print_header() {
    if [ $QUIET -eq 0 ] && [ "$OUTPUT_FORMAT" = "text" ]; then
        echo ""
        printf "${BOLD}========================================${NC}\n"
        printf "${BOLD}%s${NC}\n" "$1"
        printf "${BOLD}========================================${NC}\n"
        echo ""
    fi
}

print_separator() {
    if [ $QUIET -eq 0 ] && [ "$OUTPUT_FORMAT" = "text" ]; then
        printf "${CYAN}----------------------------------------${NC}\n"
    fi
}

cleanup() {
    if [ -n "$RESULTS_FILE" ] && [ -f "$RESULTS_FILE" ]; then
        rm -f "$RESULTS_FILE"
    fi
}

trap cleanup EXIT

# =============================================================================
# Parse Command Line Arguments
# =============================================================================

parse_args() {
    while [ $# -gt 0 ]; do
        case $1 in
            --help|-h)
                print_usage
                exit 0
                ;;
            --verbose|-v)
                VERBOSE=1
                shift
                ;;
            --json)
                OUTPUT_FORMAT="json"
                shift
                ;;
            --junit)
                OUTPUT_FORMAT="junit"
                shift
                ;;
            --algorithm|-a)
                FILTER_ALGORITHM="$2"
                shift 2
                ;;
            --variant|-V)
                FILTER_VARIANT="$2"
                shift 2
                ;;
            --report-dir|-r)
                REPORT_DIR="$2"
                shift 2
                ;;
            --no-build)
                SKIP_BUILD=1
                shift
                ;;
            --quiet|-q)
                QUIET=1
                shift
                ;;
            *)
                echo "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done
}

# =============================================================================
# Build Functions
# =============================================================================

build_project() {
    if [ $SKIP_BUILD -eq 1 ]; then
        log_info "Skipping build (--no-build specified)"
        return 0
    fi

    print_header "Building Project"

    log_info "Running build script..."

    if ! "${PROJECT_ROOT}/scripts/build.sh" > /dev/null 2>&1; then
        log_error "Build failed!"
        return 1
    fi

    if [ ! -x "$EXECUTABLE" ]; then
        log_error "Executable not found: $EXECUTABLE"
        return 1
    fi

    log_success "Build completed successfully"
    return 0
}

# =============================================================================
# Test Execution Functions
# =============================================================================

run_single_test() {
    local algorithm="$1"
    local variant="$2"
    local test_id="${algorithm}:v${variant}"

    log_verbose "Running test: $test_id"

    local start_time=$(python3 -c "import time; print(int(time.time() * 1000))" 2>/dev/null || date +%s)
    local output
    local exit_code=0

    # Run the test and capture output
    output=$("$EXECUTABLE" "$algorithm" "$variant" 2>&1) || exit_code=$?

    local end_time=$(python3 -c "import time; print(int(time.time() * 1000))" 2>/dev/null || date +%s)
    local duration=$((end_time - start_time))

    # Parse output for verification status
    local status="UNKNOWN"
    local gpu_time="N/A"
    local cpu_time="N/A"
    local speedup="N/A"
    local error_msg=""

    # Handle both output formats:
    # 1. "Verification:     PASSED" (standard)
    # 2. "Verification PASSED" (cl_extension variants)
    if echo "$output" | grep -qiE "Verification[[:space:]:]+PASSED"; then
        status="PASS"
    elif echo "$output" | grep -qiE "Verification[[:space:]:]+FAIL"; then
        status="FAIL"
        # Get max error value for error message (single line)
        local max_error=$(echo "$output" | grep -E 'Max error:' | head -1 | sed 's/.*Max error:[[:space:]]*//')
        error_msg="Max error: ${max_error}"
    elif echo "$output" | grep -qiE "Failed to run kernel|Error:"; then
        status="ERROR"
        error_msg=$(echo "$output" | grep -iE "Error:|Failed" | head -1)
    elif [ $exit_code -ne 0 ]; then
        status="ERROR"
        error_msg="Exit code: $exit_code"
    fi

    # Extract timing information (format: "OpenCL GPU time:  X.XXX ms")
    gpu_time=$(echo "$output" | grep -E 'GPU time:' | head -1 | sed 's/.*:[[:space:]]*//' | sed 's/[[:space:]]*ms.*//' || echo "N/A")
    cpu_time=$(echo "$output" | grep -E 'CPU time:' | head -1 | sed 's/.*:[[:space:]]*//' | sed 's/[[:space:]]*ms.*//' || echo "N/A")
    speedup=$(echo "$output" | grep -E 'Speedup:' | head -1 | sed 's/.*:[[:space:]]*//' | sed 's/x.*//' || echo "N/A")

    # Store result in temp file
    echo "${test_id}|${status}|${duration}|${gpu_time}|${cpu_time}|${speedup}|${error_msg}" >> "$RESULTS_FILE"

    # Update statistics
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    case $status in
        PASS)
            PASSED_TESTS=$((PASSED_TESTS + 1))
            log_success "$test_id (${duration}ms, GPU: ${gpu_time}ms, Speedup: ${speedup}x)"
            ;;
        FAIL)
            FAILED_TESTS=$((FAILED_TESTS + 1))
            log_error "$test_id - Verification failed"
            if [ $VERBOSE -eq 1 ]; then
                echo "$error_msg" | sed 's/^/    /'
            fi
            ;;
        ERROR)
            FAILED_TESTS=$((FAILED_TESTS + 1))
            log_error "$test_id - Execution error: $error_msg"
            ;;
        *)
            SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
            log_warning "$test_id - Unknown status"
            ;;
    esac

    return 0
}

run_all_tests() {
    print_header "Running Tests"

    # Initialize results file
    RESULTS_FILE=$(mktemp)

    for entry in $TEST_MATRIX; do
        local algorithm="${entry%%:*}"
        local variants_str="${entry#*:}"

        # Filter by algorithm if specified
        if [ -n "$FILTER_ALGORITHM" ] && [ "$algorithm" != "$FILTER_ALGORITHM" ]; then
            continue
        fi

        print_separator
        log_info "Testing algorithm: $algorithm"

        # Parse variants (comma-separated)
        IFS=',' read -ra variants <<< "$variants_str"
        for variant in "${variants[@]}"; do
            # Filter by variant if specified
            if [ -n "$FILTER_VARIANT" ] && [ "$variant" != "$FILTER_VARIANT" ]; then
                continue
            fi

            run_single_test "$algorithm" "$variant"
        done
    done
}

# =============================================================================
# Report Generation Functions
# =============================================================================

generate_text_report() {
    local report_file="${REPORT_DIR}/test_report_$(date +%Y%m%d_%H%M%S).txt"

    mkdir -p "$REPORT_DIR"

    {
        echo "=============================================="
        echo "OpenCL Image Processing Framework Test Report"
        echo "=============================================="
        echo ""
        echo "Generated: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "Platform: $(uname -s) $(uname -r)"
        echo ""
        echo "----------------------------------------------"
        echo "SUMMARY"
        echo "----------------------------------------------"
        echo "Total Tests:   $TOTAL_TESTS"
        echo "Passed:        $PASSED_TESTS"
        echo "Failed:        $FAILED_TESTS"
        echo "Skipped:       $SKIPPED_TESTS"

        if [ $TOTAL_TESTS -gt 0 ]; then
            local pass_rate=$(awk "BEGIN {printf \"%.1f\", ($PASSED_TESTS/$TOTAL_TESTS)*100}")
            echo "Pass Rate:     ${pass_rate}%"
        fi

        echo ""
        echo "----------------------------------------------"
        echo "DETAILED RESULTS"
        echo "----------------------------------------------"
        printf "%-20s %-8s %-10s %-12s %-12s %-10s\n" "Test" "Status" "Duration" "GPU Time" "CPU Time" "Speedup"
        printf "%-20s %-8s %-10s %-12s %-12s %-10s\n" "----" "------" "--------" "--------" "--------" "-------"

        while IFS='|' read -r test_id status duration gpu_time cpu_time speedup error_msg; do
            printf "%-20s %-8s %-10s %-12s %-12s %-10s\n" \
                "$test_id" "$status" "${duration}ms" "${gpu_time}ms" "${cpu_time}ms" "${speedup}x"
        done < "$RESULTS_FILE"

        echo ""
        echo "----------------------------------------------"
        echo "PERFORMANCE RANKING (by GPU time)"
        echo "----------------------------------------------"

        # Sort by GPU time and display ranking
        local rank=1
        sort -t'|' -k4 -n "$RESULTS_FILE" | while IFS='|' read -r test_id status duration gpu_time cpu_time speedup error_msg; do
            if [ "$status" = "PASS" ] && [ "$gpu_time" != "N/A" ]; then
                printf "%2d. %-20s GPU: %-10s Speedup: %sx\n" "$rank" "$test_id" "${gpu_time}ms" "$speedup"
                rank=$((rank + 1))
            fi
        done

        echo ""
        echo "----------------------------------------------"
        echo "FAILURES"
        echo "----------------------------------------------"

        local has_failures=0
        while IFS='|' read -r test_id status duration gpu_time cpu_time speedup error_msg; do
            if [ "$status" = "FAIL" ] || [ "$status" = "ERROR" ]; then
                has_failures=1
                echo ""
                echo "Test: $test_id"
                echo "Status: $status"
                if [ -n "$error_msg" ]; then
                    echo "Error: $error_msg"
                fi
            fi
        done < "$RESULTS_FILE"

        if [ $has_failures -eq 0 ]; then
            echo "No failures!"
        fi

        echo ""
        echo "=============================================="
        echo "END OF REPORT"
        echo "=============================================="

    } | tee "$report_file"

    log_info "Report saved to: $report_file"
}

generate_json_report() {
    local report_file="${REPORT_DIR}/test_report_$(date +%Y%m%d_%H%M%S).json"

    mkdir -p "$REPORT_DIR"

    local pass_rate=0
    if [ $TOTAL_TESTS -gt 0 ]; then
        pass_rate=$(awk "BEGIN {printf \"%.2f\", ($PASSED_TESTS/$TOTAL_TESTS)*100}")
    fi

    {
        echo "{"
        echo "  \"report\": {"
        echo "    \"generated\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\","
        echo "    \"platform\": \"$(uname -s) $(uname -r)\","
        echo "    \"project\": \"OpenCL Image Processing Framework\""
        echo "  },"
        echo "  \"summary\": {"
        echo "    \"total\": $TOTAL_TESTS,"
        echo "    \"passed\": $PASSED_TESTS,"
        echo "    \"failed\": $FAILED_TESTS,"
        echo "    \"skipped\": $SKIPPED_TESTS,"
        echo "    \"pass_rate\": $pass_rate"
        echo "  },"
        echo "  \"tests\": ["

        local first=1
        while IFS='|' read -r test_id status duration gpu_time cpu_time speedup error_msg; do
            if [ $first -eq 0 ]; then
                echo "    ,"
            fi
            first=0

            # Parse algorithm and variant from test_id
            local algorithm="${test_id%%:*}"
            local variant="${test_id#*:}"

            # Handle N/A and empty values for JSON
            local gpu_json="null"
            local cpu_json="null"
            local speedup_json="null"
            if [ -n "$gpu_time" ] && [ "$gpu_time" != "N/A" ] && [ "$gpu_time" != "" ]; then
                gpu_json="$gpu_time"
            fi
            if [ -n "$cpu_time" ] && [ "$cpu_time" != "N/A" ] && [ "$cpu_time" != "" ]; then
                cpu_json="$cpu_time"
            fi
            if [ -n "$speedup" ] && [ "$speedup" != "N/A" ] && [ "$speedup" != "" ]; then
                speedup_json="$speedup"
            fi

            echo "    {"
            echo "      \"id\": \"$test_id\","
            echo "      \"algorithm\": \"$algorithm\","
            echo "      \"variant\": \"$variant\","
            echo "      \"status\": \"$status\","
            echo "      \"duration_ms\": $duration,"
            echo "      \"gpu_time_ms\": $gpu_json,"
            echo "      \"cpu_time_ms\": $cpu_json,"
            echo "      \"speedup\": $speedup_json"
            echo -n "    }"
        done < "$RESULTS_FILE"

        echo ""
        echo "  ]"
        echo "}"
    } > "$report_file"

    # Also output to stdout
    cat "$report_file"

    log_info "JSON report saved to: $report_file" >&2
}

generate_junit_report() {
    local report_file="${REPORT_DIR}/test_report_$(date +%Y%m%d_%H%M%S).xml"

    mkdir -p "$REPORT_DIR"

    local total_time=0
    while IFS='|' read -r test_id status duration gpu_time cpu_time speedup error_msg; do
        total_time=$((total_time + duration))
    done < "$RESULTS_FILE"

    {
        echo '<?xml version="1.0" encoding="UTF-8"?>'
        echo "<testsuites name=\"OpenCL Image Processing Tests\" tests=\"$TOTAL_TESTS\" failures=\"$FAILED_TESTS\" time=\"$(awk "BEGIN {printf \"%.3f\", $total_time/1000}")\">"

        # Group by algorithm
        for entry in $TEST_MATRIX; do
            local algorithm="${entry%%:*}"
            local algo_tests=0
            local algo_failures=0
            local algo_time=0

            # Count tests for this algorithm
            while IFS='|' read -r test_id status duration gpu_time cpu_time speedup error_msg; do
                case "$test_id" in
                    ${algorithm}:*)
                        algo_tests=$((algo_tests + 1))
                        algo_time=$((algo_time + duration))
                        if [ "$status" = "FAIL" ] || [ "$status" = "ERROR" ]; then
                            algo_failures=$((algo_failures + 1))
                        fi
                        ;;
                esac
            done < "$RESULTS_FILE"

            if [ $algo_tests -gt 0 ]; then
                echo "  <testsuite name=\"$algorithm\" tests=\"$algo_tests\" failures=\"$algo_failures\" time=\"$(awk "BEGIN {printf \"%.3f\", $algo_time/1000}")\">"

                while IFS='|' read -r test_id status duration gpu_time cpu_time speedup error_msg; do
                    case "$test_id" in
                        ${algorithm}:*)
                            local variant="${test_id#*:}"
                            local time_sec=$(awk "BEGIN {printf \"%.3f\", $duration/1000}")

                            echo "    <testcase name=\"$variant\" classname=\"$algorithm\" time=\"$time_sec\">"

                            case $status in
                                FAIL)
                                    echo "      <failure message=\"Verification failed\">$error_msg</failure>"
                                    ;;
                                ERROR)
                                    echo "      <error message=\"Execution error\">$error_msg</error>"
                                    ;;
                                PASS)
                                    echo "      <system-out>GPU Time: ${gpu_time}ms, CPU Time: ${cpu_time}ms, Speedup: ${speedup}x</system-out>"
                                    ;;
                            esac

                            echo "    </testcase>"
                            ;;
                    esac
                done < "$RESULTS_FILE"

                echo "  </testsuite>"
            fi
        done

        echo "</testsuites>"
    } > "$report_file"

    # Also output to stdout
    cat "$report_file"

    log_info "JUnit report saved to: $report_file" >&2
}

generate_report() {
    print_header "Generating Report"

    case $OUTPUT_FORMAT in
        json)
            generate_json_report
            ;;
        junit)
            generate_junit_report
            ;;
        *)
            generate_text_report
            ;;
    esac
}

# =============================================================================
# Summary Functions
# =============================================================================

print_summary() {
    if [ "$OUTPUT_FORMAT" != "text" ]; then
        return 0
    fi

    print_header "Test Summary"

    printf "Total Tests:  ${BOLD}%d${NC}\n" "$TOTAL_TESTS"
    printf "Passed:       ${GREEN}%d${NC}\n" "$PASSED_TESTS"
    printf "Failed:       ${RED}%d${NC}\n" "$FAILED_TESTS"
    printf "Skipped:      ${YELLOW}%d${NC}\n" "$SKIPPED_TESTS"
    echo ""

    if [ $TOTAL_TESTS -gt 0 ]; then
        local pass_rate=$(awk "BEGIN {printf \"%.1f\", ($PASSED_TESTS/$TOTAL_TESTS)*100}")

        if [ $FAILED_TESTS -eq 0 ]; then
            printf "${GREEN}${BOLD}All tests passed! (%s%%)${NC}\n" "$pass_rate"
        else
            printf "${RED}${BOLD}Some tests failed. Pass rate: %s%%${NC}\n" "$pass_rate"
        fi
    fi
    echo ""
}

# =============================================================================
# Main Entry Point
# =============================================================================

main() {
    parse_args "$@"

    # Change to project root
    cd "$PROJECT_ROOT"

    if [ "$OUTPUT_FORMAT" = "text" ]; then
        print_header "OpenCL Image Processing Framework - Test Runner"
        log_info "Project root: $PROJECT_ROOT"
        log_info "Report directory: $REPORT_DIR"
    fi

    # Build the project
    if ! build_project; then
        log_error "Build failed, cannot run tests"
        exit 1
    fi

    # Check if executable exists
    if [ ! -x "$EXECUTABLE" ]; then
        log_error "Executable not found: $EXECUTABLE"
        exit 1
    fi

    # Run all tests
    run_all_tests

    # Generate report
    generate_report

    # Print summary
    print_summary

    # Exit with appropriate code for CI/CD
    if [ $FAILED_TESTS -gt 0 ]; then
        exit 1
    fi

    exit 0
}

main "$@"
