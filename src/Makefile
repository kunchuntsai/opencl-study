CC = gcc
CFLAGS = -Wall -O2 -std=c99 -I.

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS = -framework OpenCL -lm
else ifeq ($(UNAME_S),Linux)
    # Linux
    LDFLAGS = -lOpenCL -lm
else
    # Default to Linux-style flags
    LDFLAGS = -lOpenCL -lm
endif

# Directories
BUILD_DIR = ../out
OBJ_DIR = $(BUILD_DIR)/obj
TARGET = $(BUILD_DIR)/opencl_host

# Source files - automatically find all .c files
# Note: List algorithm directories explicitly to avoid duplicates
ALGO_DIRS = dilate gaussian
SRCS = main.c \
       algorithm_runner.c \
       $(wildcard utils/*.c) \
       $(foreach dir,$(ALGO_DIRS),$(wildcard $(dir)/*.c) $(wildcard $(dir)/c_ref/*.c))

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	@echo "Built executable: $(TARGET)"

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	cd .. && ./out/opencl_host

.PHONY: all clean run
