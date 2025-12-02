#!/bin/bash

# Benchmark - Build and run all algorithm variants

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Check if build is needed
if [ ! -f "$PROJECT_ROOT/build/opencl_host" ]; then
    echo "Building project..."
    "$SCRIPT_DIR/build.sh"
    echo ""
fi

# Run the benchmark
"$SCRIPT_DIR/run_all_variants.sh" "$@"
