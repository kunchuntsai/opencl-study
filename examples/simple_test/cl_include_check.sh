#!/bin/bash
# OpenCL Kernel Include Path Diagnostic Tool
# Checks all paths and configurations for kernel #include issues
# Suggests fixes for common misconfigurations

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

ok()   { echo -e "${GREEN}[OK]${NC}   $1"; }
fail() { echo -e "${RED}[FAIL]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
info() { echo -e "${BLUE}[INFO]${NC} $1"; }
fix()  { echo -e "${CYAN}[FIX]${NC}  $1"; }

ERRORS=0
FIXES=""

add_fix() {
    FIXES="${FIXES}\n  - $1"
    ((ERRORS++)) || true
}

echo "=============================================="
echo "  OpenCL Kernel Include Path Diagnostic Tool"
echo "=============================================="
echo ""
echo "Project root: $SCRIPT_DIR"
echo ""

# ============================================
# 1. Find all OpenCL host source files
# ============================================
echo "--- 1. OpenCL Host Source Files ---"
HOST_FILES=$(find . -name "*.c" -o -name "*.cpp" 2>/dev/null | xargs grep -l "clBuildProgram\|clCreateProgramWithSource" 2>/dev/null | grep -v build/ || true)
if [ -z "$HOST_FILES" ]; then
    fail "No OpenCL host source files found"
    add_fix "Create a host .c/.cpp file with clBuildProgram()"
else
    for f in $HOST_FILES; do
        ok "Found: $f"
    done
fi
echo ""

# ============================================
# 2. Find all OpenCL kernel files
# ============================================
echo "--- 2. OpenCL Kernel Files (.cl) ---"
KERNEL_FILES=$(find . -name "*.cl" 2>/dev/null | grep -v build/ || true)
if [ -z "$KERNEL_FILES" ]; then
    fail "No .cl kernel files found"
    add_fix "Create OpenCL kernel .cl files"
else
    for f in $KERNEL_FILES; do
        ok "Found: $f"
    done
fi
echo ""

# ============================================
# 3. Find all header files
# ============================================
echo "--- 3. Header Files (.h) ---"
HEADER_FILES=$(find . -name "*.h" 2>/dev/null | grep -v build/ | grep -v CMakeFiles || true)
if [ -z "$HEADER_FILES" ]; then
    info "No header files found"
else
    for f in $HEADER_FILES; do
        ok "Found: $f"
    done
fi
echo ""

# ============================================
# 4. Extract and validate kernel path from host code
# ============================================
echo "--- 4. Kernel Path Validation ---"
for HOST_C in $HOST_FILES; do
    echo "File: $HOST_C"
    KERNEL_PATHS=$(grep -E 'fopen\(|read_kernel_source\(|load.*kernel|LoadKernel|readFile' "$HOST_C" 2>/dev/null | \
        grep -oE '"[^"]*\.cl"' | tr -d '"' | sort -u || true)

    if [ -z "$KERNEL_PATHS" ]; then
        warn "  Could not extract kernel path (may be dynamic or argv)"
    else
        for kp in $KERNEL_PATHS; do
            if [ -f "$kp" ]; then
                ok "  Kernel path: \"$kp\" exists"
            else
                fail "  Kernel path: \"$kp\" NOT FOUND"
                # Suggest fix - find matching .cl file
                BASENAME=$(basename "$kp")
                FOUND_CL=$(find . -name "$BASENAME" 2>/dev/null | grep -v build/ | head -1 || true)
                if [ -n "$FOUND_CL" ]; then
                    # Remove leading ./
                    FOUND_CL="${FOUND_CL#./}"
                    fix "  In $HOST_C, change kernel path to: \"$FOUND_CL\""
                    add_fix "In $HOST_C: change \"$kp\" to \"$FOUND_CL\""
                else
                    fix "  Check if kernel file exists and path is relative to working directory"
                    add_fix "Kernel \"$kp\" not found - verify file exists"
                fi
            fi
        done
    fi
done
echo ""

# ============================================
# 5. Extract and validate -I include flags
# ============================================
echo "--- 5. Include Flags (-I) Validation ---"
ALL_INCLUDE_DIRS=""
for HOST_C in $HOST_FILES; do
    echo "File: $HOST_C"
    INCLUDE_FLAGS=$(grep -oE '\-I[a-zA-Z0-9_./-]+' "$HOST_C" 2>/dev/null | sort -u || true)

    if [ -z "$INCLUDE_FLAGS" ]; then
        # Check if kernels have includes
        HAS_INCLUDES=0
        for kf in $KERNEL_FILES; do
            if grep -q '#include' "$kf" 2>/dev/null; then
                HAS_INCLUDES=1
                break
            fi
        done
        if [ $HAS_INCLUDES -eq 1 ]; then
            fail "  No -I flags found but kernels have #include"
            # Suggest based on where .h files are
            if [ -n "$HEADER_FILES" ]; then
                HEADER_DIR=$(dirname "$(echo "$HEADER_FILES" | head -1)")
                HEADER_DIR="${HEADER_DIR#./}"
                fix "  Add to build options: \"-I$HEADER_DIR\""
                add_fix "In $HOST_C: add \"-I$HEADER_DIR\" to clBuildProgram options"
            fi
        else
            info "  No -I flags (kernels don't use #include)"
        fi
    else
        for iflag in $INCLUDE_FLAGS; do
            idir="${iflag#-I}"
            if [ -d "$idir" ]; then
                ok "  $iflag -> $idir/ exists"
                ALL_INCLUDE_DIRS="$ALL_INCLUDE_DIRS $idir"
                headers=$(find "$idir" -maxdepth 1 -name "*.h" 2>/dev/null || true)
                for h in $headers; do
                    info "     $(basename $h)"
                done
            else
                fail "  $iflag -> $idir/ NOT FOUND"
                # Suggest fix - find directory containing .h files
                if [ -n "$HEADER_FILES" ]; then
                    for hf in $HEADER_FILES; do
                        HEADER_DIR=$(dirname "$hf")
                        HEADER_DIR="${HEADER_DIR#./}"
                        if [ "$HEADER_DIR" != "." ]; then
                            fix "  Change $iflag to: -I$HEADER_DIR"
                            add_fix "In $HOST_C: change \"$iflag\" to \"-I$HEADER_DIR\""
                            break
                        fi
                    done
                fi
            fi
        done
    fi
done
echo ""

# ============================================
# 6. Kernel #include Resolution
# ============================================
echo "--- 6. Kernel #include Resolution ---"
for KERNEL in $KERNEL_FILES; do
    echo "Kernel: $KERNEL"
    INCLUDES=$(grep '#include' "$KERNEL" 2>/dev/null | grep -v '//' | \
        sed 's/.*#include *["<]\([^">]*\)[">].*/\1/' || true)

    if [ -z "$INCLUDES" ]; then
        info "  No #include directives"
    else
        for inc in $INCLUDES; do
            FOUND=0
            FOUND_PATH=""

            # Check in each include directory
            for idir in $ALL_INCLUDE_DIRS; do
                if [ -f "$idir/$inc" ]; then
                    ok "  #include \"$inc\" -> $idir/$inc"
                    FOUND=1
                    break
                fi
            done

            # Check relative to kernel location
            KERNEL_DIR=$(dirname "$KERNEL")
            if [ $FOUND -eq 0 ] && [ -f "$KERNEL_DIR/$inc" ]; then
                ok "  #include \"$inc\" -> $KERNEL_DIR/$inc (relative to kernel)"
                FOUND=1
            fi

            # Check in project root
            if [ $FOUND -eq 0 ] && [ -f "$inc" ]; then
                ok "  #include \"$inc\" -> $inc (project root)"
                FOUND=1
            fi

            if [ $FOUND -eq 0 ]; then
                fail "  #include \"$inc\" NOT FOUND"

                # Try to find the header and suggest fix
                BASENAME=$(basename "$inc")
                FOUND_H=$(find . -name "$BASENAME" 2>/dev/null | grep -v build/ | head -1 || true)

                if [ -n "$FOUND_H" ]; then
                    FOUND_H="${FOUND_H#./}"
                    FOUND_H_DIR=$(dirname "$FOUND_H")

                    if [ -z "$ALL_INCLUDE_DIRS" ]; then
                        fix "  Add \"-I$FOUND_H_DIR\" to clBuildProgram() in host code"
                        add_fix "Add \"-I$FOUND_H_DIR\" to build options for #include \"$inc\""
                    else
                        # Check if include path matches
                        EXPECTED_IN_IDIR=0
                        for idir in $ALL_INCLUDE_DIRS; do
                            if [ "$idir" = "$FOUND_H_DIR" ]; then
                                EXPECTED_IN_IDIR=1
                            fi
                        done

                        if [ $EXPECTED_IN_IDIR -eq 0 ]; then
                            fix "  Either:"
                            fix "    1. Change -I flag to: -I$FOUND_H_DIR"
                            fix "    2. Or move $FOUND_H to $ALL_INCLUDE_DIRS/$BASENAME"
                            add_fix "Fix include path for \"$inc\": change to -I$FOUND_H_DIR or move header"
                        fi
                    fi
                else
                    fix "  Header \"$inc\" not found in project. Create it or fix the #include path"
                    add_fix "Create missing header: $inc"
                fi
            fi
        done
    fi
done
echo ""

# ============================================
# 7. Nested #include Check
# ============================================
echo "--- 7. Nested #include in Headers ---"
NESTED_ISSUES=0
for idir in $ALL_INCLUDE_DIRS; do
    if [ -d "$idir" ]; then
        headers=$(find "$idir" -name "*.h" 2>/dev/null || true)
        for h in $headers; do
            NESTED=$(grep '#include' "$h" 2>/dev/null | grep -v '//' | \
                sed 's/.*#include *["<]\([^">]*\)[">].*/\1/' || true)
            if [ -n "$NESTED" ]; then
                for inc in $NESTED; do
                    FOUND=0
                    for idir2 in $ALL_INCLUDE_DIRS; do
                        if [ -f "$idir2/$inc" ]; then
                            FOUND=1
                            break
                        fi
                    done
                    if [ $FOUND -eq 0 ] && [ -f "$(dirname $h)/$inc" ]; then
                        FOUND=1
                    fi
                    if [ $FOUND -eq 0 ]; then
                        if [ $NESTED_ISSUES -eq 0 ]; then
                            echo "Header: $h"
                        fi
                        fail "  #include \"$inc\" NOT FOUND"
                        fix "  Add path containing \"$inc\" to -I flags"
                        add_fix "Header $h: nested #include \"$inc\" not found"
                        NESTED_ISSUES=1
                    fi
                done
            fi
        done
    fi
done
if [ $NESTED_ISSUES -eq 0 ]; then
    info "No nested include issues"
fi
echo ""

# ============================================
# 8. CMakeLists.txt Working Directory Check
# ============================================
echo "--- 8. CMakeLists.txt Configuration ---"
CMAKE_FILES=$(find . -name "CMakeLists.txt" -not -path "./build/*" 2>/dev/null || true)
CMAKE_WORK_DIR=""
CMAKE_FILE=""

for cmake in $CMAKE_FILES; do
    echo "File: $cmake"
    CMAKE_FILE="$cmake"

    EXE_NAME=$(grep 'add_executable(' "$cmake" 2>/dev/null | \
        sed 's/.*add_executable(\([^ )]*\).*/\1/' | head -1 || true)
    if [ -n "$EXE_NAME" ]; then
        info "  Executable: $EXE_NAME"
    fi

    WORK_DIR=$(grep "WORKING_DIRECTORY" "$cmake" 2>/dev/null | \
        sed 's/.*WORKING_DIRECTORY *\([^ )]*\).*/\1/' || true)
    if [ -n "$WORK_DIR" ]; then
        info "  WORKING_DIRECTORY: $WORK_DIR"
        CMAKE_WORK_DIR="$WORK_DIR"
    else
        warn "  No WORKING_DIRECTORY set (defaults to build dir - likely wrong!)"
        fix "  Add WORKING_DIRECTORY to custom run target:"
        fix "    add_custom_target(run"
        fix "        COMMAND \$<TARGET_FILE:$EXE_NAME>"
        fix "        WORKING_DIRECTORY \${CMAKE_SOURCE_DIR}"
        fix "        ...)"
        add_fix "Add WORKING_DIRECTORY to CMakeLists.txt run target"
    fi
done
echo ""

# ============================================
# 9. Run Script Check
# ============================================
echo "--- 9. Run Script Configuration ---"
RUN_SCRIPTS=$(find . -name "run*.sh" -o -name "*run.sh" 2>/dev/null | grep -v build/ || true)

for script in $RUN_SCRIPTS; do
    echo "Script: $script"

    CD_CMD=$(grep -E '^[[:space:]]*cd ' "$script" 2>/dev/null | head -1 || true)
    if [ -n "$CD_CMD" ]; then
        info "  $CD_CMD"
    else
        warn "  No 'cd' command - exe will run from current directory"
        fix "  Add at start of script: cd \"\$(dirname \"\$0\")\""
        add_fix "Add 'cd' command to $script"
    fi

    EXE_LINE=$(grep -E '^\./|/[a-zA-Z_]+$' "$script" 2>/dev/null | grep -v '#' | grep -v 'cd' | head -1 || true)
    if [ -n "$EXE_LINE" ]; then
        info "  Run: $EXE_LINE"
    fi
done
echo ""

# ============================================
# 10. Executable Check
# ============================================
echo "--- 10. Executable Check ---"
EXECUTABLES=$(find . -type f -perm +111 -not -path "./build/CMakeFiles/*" -not -name "*.sh" 2>/dev/null | \
    grep -E 'build/|out/|bin/' || true)
if [ -z "$EXECUTABLES" ]; then
    warn "No executables found"
    fix "Run build.sh or: mkdir build && cd build && cmake .. && make"
    add_fix "Build the project first"
else
    for exe in $EXECUTABLES; do
        ok "Found: $exe"
    done
fi
echo ""

# ============================================
# 11. Runtime Path Simulation
# ============================================
echo "--- 11. Runtime Simulation ---"
echo "Testing paths from expected working directory..."

# Resolve working directory
if [ -n "$CMAKE_WORK_DIR" ]; then
    if [[ "$CMAKE_WORK_DIR" == *".."* ]]; then
        RUNTIME_DIR="$SCRIPT_DIR"
    elif [[ "$CMAKE_WORK_DIR" == *"CMAKE_CURRENT_SOURCE_DIR"* ]]; then
        RUNTIME_DIR="$SCRIPT_DIR"
    elif [[ "$CMAKE_WORK_DIR" == *"CMAKE_SOURCE_DIR"* ]]; then
        CMAKE_DIR=$(dirname "$CMAKE_FILE")
        CMAKE_DIR="${CMAKE_DIR#./}"
        if [ "$CMAKE_DIR" = "." ]; then
            RUNTIME_DIR="$SCRIPT_DIR"
        else
            RUNTIME_DIR="$SCRIPT_DIR"  # Assuming parent
        fi
    else
        RUNTIME_DIR="$SCRIPT_DIR"
    fi
else
    RUNTIME_DIR="$SCRIPT_DIR"
fi

info "Working directory: $RUNTIME_DIR"
cd "$RUNTIME_DIR"

RUNTIME_ERRORS=0
for HOST_C in $HOST_FILES; do
    KERNEL_PATH=$(grep -E 'fopen\(|read_kernel_source\(' "$SCRIPT_DIR/$HOST_C" 2>/dev/null | \
        grep -oE '"[^"]*\.cl"' | tr -d '"' | head -1 || true)
    INCLUDE_FLAG=$(grep -oE '\-I[a-zA-Z0-9_./-]+' "$SCRIPT_DIR/$HOST_C" 2>/dev/null | head -1 || true)
    INCLUDE_DIR="${INCLUDE_FLAG#-I}"

    if [ -n "$KERNEL_PATH" ]; then
        if [ -f "$KERNEL_PATH" ]; then
            ok "Kernel \"$KERNEL_PATH\" accessible at runtime"
        else
            fail "Kernel \"$KERNEL_PATH\" NOT accessible at runtime"
            RUNTIME_ERRORS=1
        fi
    fi

    if [ -n "$INCLUDE_DIR" ]; then
        if [ -d "$INCLUDE_DIR" ]; then
            ok "Include \"$INCLUDE_DIR/\" accessible at runtime"
        else
            fail "Include \"$INCLUDE_DIR/\" NOT accessible at runtime"
            RUNTIME_ERRORS=1
        fi
    fi
done

if [ $RUNTIME_ERRORS -eq 1 ]; then
    echo ""
    fix "Paths must be relative to WORKING_DIRECTORY, not to source files"
    fix "Either adjust paths in host.c OR adjust WORKING_DIRECTORY in CMake/run.sh"
fi
echo ""

# ============================================
# Summary with Fixes
# ============================================
echo "=============================================="
if [ $ERRORS -eq 0 ]; then
    echo -e "  ${GREEN}All checks passed!${NC}"
else
    echo -e "  ${RED}Found $ERRORS issue(s)${NC}"
    echo ""
    echo "  Suggested fixes:"
    echo -e "$FIXES"
fi
echo "=============================================="
echo ""
echo "Quick reference:"
echo "  - Kernel path in host.c must be relative to WORKING_DIRECTORY"
echo "  - -I path in clBuildProgram must be relative to WORKING_DIRECTORY"
echo "  - WORKING_DIRECTORY must be set in CMake or 'cd' in run.sh"
echo "  - All paths are resolved from where the exe runs, not where source is"
echo ""
