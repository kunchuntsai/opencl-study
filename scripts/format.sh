#!/bin/bash

# Format script for OpenCL Image Processing Framework
# Runs clang-format on all C/C++ source files
#
# Usage:
#   ./scripts/format.sh           - Check formatting (dry-run, shows which files need formatting)
#   ./scripts/format.sh --fix     - Apply formatting to all files
#   ./scripts/format.sh --help    - Show this help message

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Find all C/C++ source and header files
EXTENSIONS=(-name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" -o -name "*.cl")

show_help() {
    echo "OpenCL Image Processing Framework - Format Script"
    echo ""
    echo "Usage:"
    echo "  ./scripts/format.sh           - Check formatting (dry-run)"
    echo "  ./scripts/format.sh --fix     - Apply formatting to all files"
    echo "  ./scripts/format.sh --help    - Show this help message"
    echo ""
    echo "Formats all C/C++ source files (.c, .h, .cpp, .hpp, .cl) using .clang-format config"
}

check_clang_format() {
    if ! command -v clang-format &> /dev/null; then
        echo "Error: clang-format is not installed"
        echo "Install with: brew install clang-format"
        exit 1
    fi
}

format_check() {
    echo "=== Checking Code Formatting ==="
    echo ""

    check_clang_format

    local files_need_formatting=0

    # Find all source files and check formatting
    while IFS= read -r -d '' file; do
        if ! clang-format --dry-run -Werror "$file" 2>/dev/null; then
            echo "Needs formatting: $file"
            ((files_need_formatting++))
        fi
    done < <(find "$PROJECT_ROOT/src" "$PROJECT_ROOT/include" \( "${EXTENSIONS[@]}" \) -print0 2>/dev/null)

    echo ""
    if [ $files_need_formatting -eq 0 ]; then
        echo "✓ All files are properly formatted!"
        return 0
    else
        echo "✗ Found $files_need_formatting file(s) that need formatting"
        echo ""
        echo "Run './scripts/format.sh --fix' to format all files"
        return 1
    fi
}

format_fix() {
    echo "=== Applying Code Formatting ==="
    echo ""

    check_clang_format

    local files_formatted=0

    # Find all source files and format them
    while IFS= read -r -d '' file; do
        echo "Formatting: $file"
        clang-format -i "$file"
        ((files_formatted++))
    done < <(find "$PROJECT_ROOT/src" "$PROJECT_ROOT/include" \( "${EXTENSIONS[@]}" \) -print0 2>/dev/null)

    echo ""
    echo "✓ Formatted $files_formatted file(s)"
}

# Main script logic
case "${1:-}" in
    --fix)
        format_fix
        ;;
    --help|-h)
        show_help
        ;;
    "")
        format_check
        ;;
    *)
        echo "Error: Unknown option '$1'"
        echo ""
        show_help
        exit 1
        ;;
esac
