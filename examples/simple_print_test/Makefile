# Simple OpenCL 1.2 Print Test Makefile
#
# This Makefile builds a standalone test demonstrating:
# - OpenCL 1.2 kernel compilation with include paths
# - Kernel printf functionality
# - Include header (utils.h) in kernel code
#
# REQUIREMENTS:
#   For full OpenCL runtime support, install:
#     Ubuntu/Debian: sudo apt-get install opencl-headers ocl-icd-opencl-dev
#     macOS: OpenCL included in system
#
#   This directory includes a minimal CL/cl.h stub for compilation testing.

CC = gcc
CFLAGS = -Wall -O2

# Include path for local OpenCL headers (stub for offline builds)
# Remove -I. if using system OpenCL headers
INCLUDES = -I.

# Detect OS for OpenCL linking
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    # macOS uses framework
    LDFLAGS = -framework OpenCL
else
    # Linux uses library
    LDFLAGS = -lOpenCL
endif

TARGET = simple_print_test
SOURCES = host.c

.PHONY: all clean run check-syntax info

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS)
	@echo ""
	@echo "Build successful!"
	@echo "Run with: ./$(TARGET)"
	@echo ""
	@echo "Files in this test:"
	@echo "  - host.c    : Host program that loads and runs kernel"
	@echo "  - kernel.cl : OpenCL kernel that includes utils.h"
	@echo "  - utils.h   : Header with utility functions for kernel"

# Compile-only check (no linking) - useful for syntax verification
check-syntax: $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -fsyntax-only $<
	@echo "Syntax check passed!"

run: $(TARGET)
	@echo "Running simple print test..."
	@echo ""
	./$(TARGET)

clean:
	rm -f $(TARGET)
	@echo "Cleaned up"

# Show info about the test
info:
	@echo "=== Simple OpenCL 1.2 Print Test ==="
	@echo ""
	@echo "This test demonstrates:"
	@echo "  1. Including a header file (utils.h) in an OpenCL kernel"
	@echo "  2. Using -I flag to specify include path during kernel compilation"
	@echo "  3. OpenCL 1.2 printf extension for kernel debugging"
	@echo ""
	@echo "Directory structure (flat):"
	@echo "  simple_print_test/"
	@echo "    ├── host.c      - Host program"
	@echo "    ├── kernel.cl   - OpenCL kernel (includes utils.h)"
	@echo "    ├── utils.h     - Kernel utility functions"
	@echo "    ├── CL/cl.h     - Minimal OpenCL header stub"
	@echo "    └── Makefile    - Build script"
	@echo ""
	@echo "Key point for kernel includes:"
	@echo "  The -I. flag in clBuildProgram() tells the OpenCL compiler"
	@echo "  to look in the current directory for #include files."
	@echo "  This allows kernel.cl to use: #include \"utils.h\""
