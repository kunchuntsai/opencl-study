#!/bin/bash
#
# CI/CD Test Integration Script
#
# Designed for use in continuous integration pipelines (GitHub Actions, GitLab CI, Jenkins, etc.)
# Provides structured output, proper exit codes, and artifact generation.
#
# Usage:
#   ./tests/ci_test.sh [test_level]
#
# Test Levels:
#   smoke      - Quick smoke tests (one variant per algorithm)
#   regression - Full regression tests (all variants)
#   benchmark  - Performance benchmark tests
#   all        - Complete test suite (default)
#
# Environment Variables:
#   CI_REPORT_DIR     - Directory for reports (default: tests/reports)
#   CI_ARTIFACTS_DIR  - Directory for CI artifacts (default: artifacts)
#   CI_FAIL_FAST      - Exit on first failure (default: false)
#   CI_VERBOSE        - Enable verbose output (default: false)
#

set -eo pipefail

# =============================================================================
# Configuration
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Source auto-detection and test configuration
source "${SCRIPT_DIR}/detect_tests.sh"
source "${SCRIPT_DIR}/test_config.sh"

# CI Environment
TEST_LEVEL="${1:-all}"
REPORT_DIR="${CI_REPORT_DIR:-${PROJECT_ROOT}/tests/reports}"
ARTIFACTS_DIR="${CI_ARTIFACTS_DIR:-${PROJECT_ROOT}/artifacts}"
FAIL_FAST="${CI_FAIL_FAST:-false}"
VERBOSE="${CI_VERBOSE:-false}"

# Executable
BUILD_DIR="${PROJECT_ROOT}/build"
EXECUTABLE="${BUILD_DIR}/opencl_host"

# Test tracking (using temp files for POSIX compatibility)
PASSED_FILE=""
FAILED_FILE=""
SKIPPED_FILE=""
START_TIME=$(date +%s)

# =============================================================================
# CI Output Functions
# =============================================================================

ci_group_start() {
    local name="$1"
    # GitHub Actions
    if [ -n "$GITHUB_ACTIONS" ]; then
        echo "::group::$name"
    # GitLab CI
    elif [ -n "$GITLAB_CI" ]; then
        echo -e "\e[0Ksection_start:$(date +%s):${name//[^a-zA-Z0-9]/_}\r\e[0K$name"
    else
        echo "==> $name"
    fi
}

ci_group_end() {
    local name="$1"
    if [ -n "$GITHUB_ACTIONS" ]; then
        echo "::endgroup::"
    elif [ -n "$GITLAB_CI" ]; then
        echo -e "\e[0Ksection_end:$(date +%s):${name//[^a-zA-Z0-9]/_}\r\e[0K"
    fi
}

ci_error() {
    local message="$1"
    local file="${2:-}"
    local line="${3:-}"

    if [ -n "$GITHUB_ACTIONS" ]; then
        if [ -n "$file" ]; then
            echo "::error file=$file,line=$line::$message"
        else
            echo "::error::$message"
        fi
    else
        echo "ERROR: $message"
    fi
}

ci_warning() {
    local message="$1"

    if [ -n "$GITHUB_ACTIONS" ]; then
        echo "::warning::$message"
    else
        echo "WARNING: $message"
    fi
}

ci_set_output() {
    local name="$1"
    local value="$2"

    if [ -n "$GITHUB_ACTIONS" ] && [ -n "$GITHUB_OUTPUT" ]; then
        echo "$name=$value" >> "$GITHUB_OUTPUT"
    elif [ -n "$GITLAB_CI" ] && [ -n "$CI_PROJECT_DIR" ]; then
        echo "export $name=\"$value\"" >> "${CI_PROJECT_DIR}/.env"
    fi

    echo "OUTPUT: $name=$value"
}

ci_summary() {
    local content="$1"

    if [ -n "$GITHUB_ACTIONS" ] && [ -n "$GITHUB_STEP_SUMMARY" ]; then
        echo "$content" >> "$GITHUB_STEP_SUMMARY"
    fi
}

cleanup() {
    rm -f "$PASSED_FILE" "$FAILED_FILE" "$SKIPPED_FILE"
}

trap cleanup EXIT

# =============================================================================
# Test Functions
# =============================================================================

get_test_variants() {
    local level="$1"

    case $level in
        smoke)
            echo "$SMOKE_TEST_VARIANTS"
            ;;
        regression)
            echo "$REGRESSION_TEST_VARIANTS"
            ;;
        benchmark)
            echo "$BENCHMARK_VARIANTS"
            ;;
        all)
            echo "$REGRESSION_TEST_VARIANTS"
            ;;
        *)
            echo "Unknown test level: $level" >&2
            exit 1
            ;;
    esac
}

run_test() {
    local test_spec="$1"
    local algorithm="${test_spec%%:*}"
    local variant="${test_spec#*:}"
    local test_id="${algorithm}:v${variant}"

    echo -n "  Testing $test_id... "

    local output
    local exit_code=0
    local start=$(date +%s)

    output=$("$EXECUTABLE" "$algorithm" "$variant" 2>&1) || exit_code=$?

    local end=$(date +%s)
    local duration=$((end - start))

    # Parse results
    local status="UNKNOWN"
    local gpu_time=""

    # Handle both output formats:
    # 1. "Verification:     PASSED" (standard)
    # 2. "Verification PASSED" (cl_extension variants)
    if echo "$output" | grep -qiE "Verification[[:space:]:]+PASSED"; then
        status="PASS"
        gpu_time=$(echo "$output" | grep -E 'GPU time:' | head -1 | sed 's/.*:[[:space:]]*//' | sed 's/[[:space:]]*ms.*//' || echo "")
    elif echo "$output" | grep -qiE "Verification[[:space:]:]+FAIL"; then
        status="FAIL"
    elif echo "$output" | grep -qiE "Failed to run kernel|Error:"; then
        status="ERROR"
    elif [ $exit_code -ne 0 ]; then
        status="ERROR"
    fi

    # Record result
    case $status in
        PASS)
            echo "$test_id" >> "$PASSED_FILE"
            echo "PASS (${duration}s, GPU: ${gpu_time}ms)"
            ;;
        FAIL)
            echo "$test_id" >> "$FAILED_FILE"
            echo "FAIL"
            ci_error "Test $test_id failed verification"

            if [ "$VERBOSE" = "true" ]; then
                echo "$output" | head -20
            fi

            if [ "$FAIL_FAST" = "true" ]; then
                echo "FAIL_FAST enabled, stopping tests"
                return 1
            fi
            ;;
        ERROR)
            echo "$test_id" >> "$FAILED_FILE"
            echo "ERROR (exit code: $exit_code)"
            ci_error "Test $test_id execution failed"

            if [ "$FAIL_FAST" = "true" ]; then
                return 1
            fi
            ;;
        *)
            echo "$test_id" >> "$SKIPPED_FILE"
            echo "UNKNOWN"
            ci_warning "Test $test_id returned unknown status"
            ;;
    esac

    return 0
}

# =============================================================================
# Build
# =============================================================================

build_project() {
    ci_group_start "Build Project"

    echo "Building OpenCL project..."

    if ! "${PROJECT_ROOT}/scripts/build.sh" > /dev/null 2>&1; then
        ci_error "Build failed"
        ci_group_end "Build Project"
        return 1
    fi

    if [ ! -x "$EXECUTABLE" ]; then
        ci_error "Executable not found after build: $EXECUTABLE"
        ci_group_end "Build Project"
        return 1
    fi

    echo "Build successful: $EXECUTABLE"
    ci_group_end "Build Project"
    return 0
}

# =============================================================================
# Run Tests
# =============================================================================

run_tests() {
    ci_group_start "Run Tests ($TEST_LEVEL)"

    local variants=$(get_test_variants "$TEST_LEVEL")
    local count=0
    for v in $variants; do
        count=$((count + 1))
    done

    echo "Running $count tests (level: $TEST_LEVEL)"
    echo ""

    for test_spec in $variants; do
        if ! run_test "$test_spec"; then
            ci_group_end "Run Tests ($TEST_LEVEL)"
            return 1
        fi
    done

    ci_group_end "Run Tests ($TEST_LEVEL)"
    return 0
}

# =============================================================================
# Generate Reports
# =============================================================================

generate_reports() {
    ci_group_start "Generate Reports"

    mkdir -p "$REPORT_DIR" "$ARTIFACTS_DIR"

    local timestamp=$(date +%Y%m%d_%H%M%S)
    local passed=$(wc -l < "$PASSED_FILE" 2>/dev/null | tr -d ' ' || echo "0")
    local failed=$(wc -l < "$FAILED_FILE" 2>/dev/null | tr -d ' ' || echo "0")
    local skipped=$(wc -l < "$SKIPPED_FILE" 2>/dev/null | tr -d ' ' || echo "0")
    local total=$((passed + failed + skipped))
    local pass_rate=0

    if [ $total -gt 0 ]; then
        pass_rate=$(awk "BEGIN {printf \"%.1f\", ($passed/$total)*100}")
    fi

    # Generate JSON report
    local json_file="${REPORT_DIR}/ci_results_${timestamp}.json"
    {
        echo "{"
        echo "    \"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\","
        echo "    \"test_level\": \"$TEST_LEVEL\","
        echo "    \"platform\": \"$(uname -s) $(uname -r)\","
        echo "    \"commit\": \"$(git -C "$PROJECT_ROOT" rev-parse --short HEAD 2>/dev/null || echo "unknown")\","
        echo "    \"summary\": {"
        echo "        \"total\": $total,"
        echo "        \"passed\": $passed,"
        echo "        \"failed\": $failed,"
        echo "        \"skipped\": $skipped,"
        echo "        \"pass_rate\": $pass_rate"
        echo "    },"
        echo "    \"passed_tests\": ["
        local first=1
        if [ -f "$PASSED_FILE" ]; then
            while read -r test; do
                if [ $first -eq 0 ]; then echo ","; fi
                first=0
                echo -n "        \"$test\""
            done < "$PASSED_FILE"
        fi
        echo ""
        echo "    ],"
        echo "    \"failed_tests\": ["
        first=1
        if [ -f "$FAILED_FILE" ]; then
            while read -r test; do
                if [ $first -eq 0 ]; then echo ","; fi
                first=0
                echo -n "        \"$test\""
            done < "$FAILED_FILE"
        fi
        echo ""
        echo "    ],"
        echo "    \"skipped_tests\": ["
        first=1
        if [ -f "$SKIPPED_FILE" ]; then
            while read -r test; do
                if [ $first -eq 0 ]; then echo ","; fi
                first=0
                echo -n "        \"$test\""
            done < "$SKIPPED_FILE"
        fi
        echo ""
        echo "    ]"
        echo "}"
    } > "$json_file"

    echo "JSON report: $json_file"

    # Generate JUnit XML for CI systems
    local junit_file="${REPORT_DIR}/junit_results_${timestamp}.xml"
    local end_time=$(date +%s)
    local total_time=$((end_time - START_TIME))

    {
        echo '<?xml version="1.0" encoding="UTF-8"?>'
        echo "<testsuites name=\"OpenCL Tests\" tests=\"$total\" failures=\"$failed\" time=\"$total_time\">"
        echo "    <testsuite name=\"$TEST_LEVEL\" tests=\"$total\" failures=\"$failed\" time=\"$total_time\">"

        if [ -f "$PASSED_FILE" ]; then
            while read -r test; do
                local algo="${test%%:*}"
                local var="${test#*:}"
                echo "        <testcase name=\"$var\" classname=\"$algo\" />"
            done < "$PASSED_FILE"
        fi

        if [ -f "$FAILED_FILE" ]; then
            while read -r test; do
                local algo="${test%%:*}"
                local var="${test#*:}"
                echo "        <testcase name=\"$var\" classname=\"$algo\">"
                echo "            <failure message=\"Test verification failed\" />"
                echo "        </testcase>"
            done < "$FAILED_FILE"
        fi

        echo "    </testsuite>"
        echo "</testsuites>"
    } > "$junit_file"

    echo "JUnit report: $junit_file"

    # Copy reports to artifacts directory
    cp "$json_file" "$junit_file" "$ARTIFACTS_DIR/"

    # Generate GitHub Actions summary if available
    if [ -n "$GITHUB_STEP_SUMMARY" ]; then
        ci_summary "## Test Results"
        ci_summary ""
        ci_summary "| Metric | Value |"
        ci_summary "|--------|-------|"
        ci_summary "| Total | $total |"
        ci_summary "| Passed | $passed |"
        ci_summary "| Failed | $failed |"
        ci_summary "| Pass Rate | ${pass_rate}% |"
        ci_summary ""

        if [ "$failed" -gt 0 ] && [ -f "$FAILED_FILE" ]; then
            ci_summary "### Failed Tests"
            while read -r test; do
                ci_summary "- $test"
            done < "$FAILED_FILE"
        fi
    fi

    ci_group_end "Generate Reports"

    # Set CI outputs
    ci_set_output "total_tests" "$total"
    ci_set_output "passed_tests" "$passed"
    ci_set_output "failed_tests" "$failed"
    ci_set_output "pass_rate" "$pass_rate"
    ci_set_output "result" "$([ "$failed" -eq 0 ] && echo 'success' || echo 'failure')"
}

# =============================================================================
# Print Summary
# =============================================================================

print_summary() {
    local passed=$(wc -l < "$PASSED_FILE" 2>/dev/null | tr -d ' ' || echo "0")
    local failed=$(wc -l < "$FAILED_FILE" 2>/dev/null | tr -d ' ' || echo "0")
    local skipped=$(wc -l < "$SKIPPED_FILE" 2>/dev/null | tr -d ' ' || echo "0")
    local total=$((passed + failed + skipped))

    echo ""
    echo "========================================"
    echo "TEST SUMMARY"
    echo "========================================"
    echo "Level:     $TEST_LEVEL"
    echo "Total:     $total"
    echo "Passed:    $passed"
    echo "Failed:    $failed"
    echo "Skipped:   $skipped"
    echo "Duration:  $(($(date +%s) - START_TIME))s"
    echo "========================================"

    if [ "$failed" -gt 0 ]; then
        echo ""
        echo "FAILED TESTS:"
        if [ -f "$FAILED_FILE" ]; then
            while read -r test; do
                echo "  - $test"
            done < "$FAILED_FILE"
        fi
        echo ""
        echo "RESULT: FAILED"
        return 1
    else
        echo ""
        echo "RESULT: PASSED"
        return 0
    fi
}

# =============================================================================
# Main
# =============================================================================

main() {
    echo "========================================"
    echo "OpenCL CI Test Runner"
    echo "========================================"
    echo "Test Level: $TEST_LEVEL"
    echo "Report Dir: $REPORT_DIR"
    echo "Fail Fast:  $FAIL_FAST"
    echo "========================================"
    echo ""

    # Initialize temp files
    PASSED_FILE=$(mktemp)
    FAILED_FILE=$(mktemp)
    SKIPPED_FILE=$(mktemp)

    # Validate test level
    case $TEST_LEVEL in
        smoke|regression|benchmark|all)
            ;;
        *)
            ci_error "Invalid test level: $TEST_LEVEL"
            echo "Valid levels: smoke, regression, benchmark, all"
            exit 1
            ;;
    esac

    # Build project
    if ! build_project; then
        exit 1
    fi

    # Run tests
    run_tests || true  # Continue to generate reports even if tests fail

    # Generate reports
    generate_reports

    # Print summary and exit with appropriate code
    if ! print_summary; then
        exit 1
    fi

    exit 0
}

main "$@"
