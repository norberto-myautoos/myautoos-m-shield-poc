CC ?= cc
BUILD ?= build
WARN = -Wall -Wextra -Wconversion
CFLAGS = -std=c99 $(WARN) $(EXTRA) -Iinclude

test:
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) src/mshield_core.c test/test_mshield.c -o $(BUILD)/test_core
	$(BUILD)/test_core
lint:
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c src/mcp2515.c -o $(BUILD)/mcp2515.o
	$(CC) $(CFLAGS) -c src/mshield_core.c -o $(BUILD)/mshield_core.o
.PHONY: test lint
