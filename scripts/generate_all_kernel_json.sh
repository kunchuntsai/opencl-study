#!/bin/bash
# Generate JSON files for all .ini configuration files in config/

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CONFIG_DIR="$PROJECT_ROOT/config"
OUTPUT_DIR="${1:-$PROJECT_ROOT/batch}"

echo "Generating kernel JSON files..."
echo "Config directory: $CONFIG_DIR"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Find all .ini files in config directory
ini_count=0
for ini_file in "$CONFIG_DIR"/*.ini; do
    if [ -f "$ini_file" ]; then
        echo "Processing: $ini_file"
        python3 "$SCRIPT_DIR/generate_kernel_json.py" "$ini_file" "$OUTPUT_DIR"
        ((ini_count++))
        echo ""
    fi
done

if [ $ini_count -eq 0 ]; then
    echo "No .ini files found in $CONFIG_DIR"
    exit 1
fi

echo "Done! Generated $ini_count JSON file(s) in $OUTPUT_DIR"
