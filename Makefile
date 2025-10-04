
# # POSIX C (pthreads) build for libtslog + log_stress

# APP_NAME := log_stress
# BUILD_DIR := build
# BIN_DIR := bin
# SRC_DIR := src
# TEST_DIR := tests
# INCLUDE_DIRS := -I$(SRC_DIR)

# CC := gcc
# CFLAGS := -std=c11 -O2 -Wall -Wextra -pedantic -pthread
# LDFLAGS := -pthread

# SRCS := \
# 	$(SRC_DIR)/threadsafe_queue.c \
# 	$(SRC_DIR)/tslog.c \
# 	$(SRC_DIR)/common.c

# OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# TEST_SRCS := $(TEST_DIR)/log_stress.c
# TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.o,$(TEST_SRCS))

# .PHONY: all clean dirs test run

# all: dirs $(BIN_DIR)/$(APP_NAME)

# $(BIN_DIR)/$(APP_NAME): $(OBJS) $(TEST_OBJS)
# 	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $^ -o $@ $(LDFLAGS)

# $(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
# 	@mkdir -p $(BUILD_DIR)
# 	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# $(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
# 	@mkdir -p $(BUILD_DIR)
# 	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# clean:
# 	rm -rf $(BUILD_DIR) $(BIN_DIR)

# dirs:
# 	@mkdir -p $(BUILD_DIR) $(BIN_DIR) logs

# # Quick test
# run: all
# 	./bin/$(APP_NAME) -t 8 -n 50000 -o logs/app.log --level INFO


# Makefile — Etapa 2 (v2-cli) usando a lib da Etapa 1
CC=gcc
CFLAGS=-O2 -Wall -Wextra -pedantic
LDFLAGS= -pthread
BIN_DIR=bin
SRC_DIR=src
LOG_DIR=logs

# === ARQUIVOS DA ETAPA 1 (ajuste os nomes se diferirem) ===
TSLOG=$(SRC_DIR)/tslog.c
TSLOG_H=$(SRC_DIR)/tslog.h
TSQ=$(SRC_DIR)/threadsafe_queue.c
TSQ_H=$(SRC_DIR)/threadsafe_queue.h
COMMON=$(SRC_DIR)/common.c
COMMON_H=$(SRC_DIR)/common.h

# === ETAPA 2 ===
SRV=$(SRC_DIR)/server.c
CLI=$(SRC_DIR)/client.c
SRV_H=$(SRC_DIR)/server.h
CLI_H=$(SRC_DIR)/client.h

all: prep $(BIN_DIR)/server $(BIN_DIR)/client

prep:
	mkdir -p $(BIN_DIR) $(LOG_DIR)

# Inclua tslog + queue + common em AMBOS os binários
$(BIN_DIR)/server: $(SRV) $(SRV_H) $(TSLOG) $(TSLOG_H) $(TSQ) $(TSQ_H) $(COMMON) $(COMMON_H)
	$(CC) $(CFLAGS) -o $@ $(SRV) $(TSLOG) $(TSQ) $(COMMON) $(LDFLAGS)

$(BIN_DIR)/client: $(CLI) $(CLI_H) $(TSLOG) $(TSLOG_H) $(TSQ) $(TSQ_H) $(COMMON) $(COMMON_H)
	$(CC) $(CFLAGS) -o $@ $(CLI) $(TSLOG) $(TSQ) $(COMMON) $(LDFLAGS)

clean:
	rm -f $(BIN_DIR)/server $(BIN_DIR)/client
