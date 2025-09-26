
# POSIX C (pthreads) build for libtslog + log_stress

APP_NAME := log_stress
BUILD_DIR := build
BIN_DIR := bin
SRC_DIR := src
TEST_DIR := tests
INCLUDE_DIRS := -I$(SRC_DIR)

CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -pedantic -pthread
LDFLAGS := -pthread

SRCS := \
	$(SRC_DIR)/threadsafe_queue.c \
	$(SRC_DIR)/tslog.c \
	$(SRC_DIR)/common.c

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TEST_SRCS := $(TEST_DIR)/log_stress.c
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.o,$(TEST_SRCS))

.PHONY: all clean dirs test run

all: dirs $(BIN_DIR)/$(APP_NAME)

$(BIN_DIR)/$(APP_NAME): $(OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) logs

# Quick test
run: all
	./bin/$(APP_NAME) -t 8 -n 50000 -o logs/app.log --level INFO
