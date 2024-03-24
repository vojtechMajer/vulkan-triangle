# CC: Program for compiling C programs; default cc
# CFLAGS: Extra flags to give to the C compiler
# CPPFLAGS: Extra flags to give to the C preprocessor
# LDFLAGS: Extra flags to give to compilers when they are supposed to invoke the linker

# $(patsubst pattern,replacement,text)
# $(text:pattern=replacement)

CC := gcc

# Wall - warnings all
# Wpedantic - Issue all the warnings demanded by strict ISO C and ISO C++; diagnose all programs that use forbidden extensions, and some other programs that do not follow ISO C and ISO C++. This follows the version of the ISO C or C++ standard specified by any -std option used.
# O2 - optimization 2 can be set to 1, 2, 3
# std - c standard
CFLAGS := -Wall -Wpedantic -pedantic -O2 -std=c99

LDFLAGS := -lglfw -lvulkan

BINDIR := ./bin
BUILD_DIR := ./obj
SRC_DIRS := ./src

SHADERS_DIR := ./shaders

SHADERS = $(filter-out $(wildcard $(SHADERS_DIR)/*.spv),$(wildcard $(SHADERS_DIR)/*)) 

TARGET_EXEC := $(BINDIR)/vulkanTriangle


all: $(TARGET_EXEC) shader

shader: $(SHADERS:$(SHADERS_DIR)/%=$(SHADERS_DIR)/%.spv)

$(SHADERS_DIR)/%.spv: $(SHADERS_DIR)/%
	glslc $< -o $@

# Find all the C files we want to compile
# Note the single quotes around the * expressions. The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) -name '*.c')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP

# The final build step.
$(TARGET_EXEC): $(OBJS)
	mkdir -p $(BINDIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)

	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/* $(BINDIR)/*
	rm -rf $(SHADERS_DIR)/*.spv

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)