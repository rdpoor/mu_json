# TODO:
# Note the following way to test different compiler flags.  Use for mu_log test:
# coverage:
# 	# Clean and rebuild everything with coverage flags
# 	$(MAKE) clean
# 	$(MAKE) all CFLAGS="$(CFLAGS) $(GCOVFLAGS)"
# 	# Run tests to generate coverage data
# 	@for test in $(EXECUTABLES) ; do \
# 		./$$test; \
# 	done
#
# Or from command line:
# make CFLAGS="-Wall -g -DUSE_STRING_LIB" tests

# Compile and run unit tests
SRC_DIR := ../src
TEST_DIR := ../test
TEST_SUPPORT_DIR := ../../test_support
OBJ_DIR := $(TEST_DIR)/obj
BIN_DIR := $(TEST_DIR)/bin
COVERAGE_DIR := $(TEST_DIR)/coverage

SRC_FILES := \
	$(SRC_DIR)/mu_json.c \
	$(SRC_DIR)/mu_log.c \
	$(SRC_DIR)/mu_str.c

TEST_FILES := \
	$(TEST_DIR)/test_mu_json.c \
	$(TEST_DIR)/test_mu_str.c

# Note: everything below this line is common to all modules.  Consider
# splitting into shared makefile.

CC := gcc
CFLAGS := -Wall -g
DEPFLAGS := -MMD -MP
GCOVFLAGS := -fprofile-arcs -ftest-coverage
# Add coverage flags also to the linker flags
LFLAGS := $(GCOVFLAGS)

TEST_SUPPORT_FILES := \
	$(TEST_SUPPORT_DIR)/unity.c

SRC_OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES) $(CORE_FILES))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c, $(OBJ_DIR)/%.o, $(TEST_FILES))
TEST_SUPPORT_OBJS := $(patsubst $(TEST_SUPPORT_DIR)/%.c, $(OBJ_DIR)/%.o, $(TEST_SUPPORT_FILES))
EXECUTABLES := $(patsubst $(TEST_DIR)/%.c, $(BIN_DIR)/%, $(TEST_FILES))

# Prevent makefile from automatically deleting object files
.SECONDARY: $(SRC_OBJS) $(TEST_OBJS) $(TEST_SUPPORT_OBJS)

# $(info SRC_OBJS = $(SRC_OBJS))
# $(info TEST_OBJS = $(TEST_OBJS))
# $(info TEST_SUPPORT_OBJS = $(TEST_SUPPORT_OBJS))
# $(info EXECUTABLES = $(EXECUTABLES))

.PHONY: all tests coverage clean

all: $(EXECUTABLES)

tests: $(EXECUTABLES)
	@for test in $(EXECUTABLES) ; do \
		echo "Running $$test..."; \
		./$$test; \
	done

coverage:
	# Clean and rebuild everything with coverage flags
	$(MAKE) clean
	$(MAKE) all CFLAGS="$(CFLAGS) $(GCOVFLAGS)"
	# Run tests to generate coverage data
	@for test in $(EXECUTABLES) ; do \
		./$$test; \
	done
	# Capture initial coverage data
	lcov --capture --directory $(OBJ_DIR) --output-file coverage.info
	# Remove coverage data for test and test_support directories using absolute paths
	# lcov --remove coverage.info "$(TEST_DIR_ABS)/*" "$(TEST_SUPPORT_DIR_ABS)/*" --output-file coverage.info.cleaned
	# Generate coverage report
	genhtml coverage.info --output-directory $(COVERAGE_DIR)
	@echo "Coverage report generated in $(COVERAGE_DIR)"
	# Optional: Clean up intermediate coverage files
	# rm -f coverage.info coverage.info.cleaned

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(COVERAGE_DIR) coverage.info *.gcda *.gcno

# Compile and generate dependencies for source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(DEPFLAGS) -c $< -o $@

# Compile and generate dependencies for test files
$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_SUPPORT_DIR) $(DEPFLAGS) -c $< -o $@

# Compile and generate dependencies for test support files
$(OBJ_DIR)/%.o: $(TEST_SUPPORT_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(DEPFLAGS) -c $< -o $@

-include $(OBJ_DIR)/*.d

# Link object files to create executables
$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(SRC_OBJS) $(TEST_SUPPORT_OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) $(LFLAGS) $^ -o $@
