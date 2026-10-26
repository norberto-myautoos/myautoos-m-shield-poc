CC ?= cc
BUILD ?= build
WARN = -Wall -Wextra -Wconversion
CFLAGS = -std=c99 $(WARN) $(EXTRA) -Iinclude
MODULES = vcu_state_machine torque_arbitration vcu_diagnostics safety_monitor

sil:
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) src/*.c tests/main_test_bench.c -o $(BUILD)/sil_bench
	$(BUILD)/sil_bench

coverage:
	mkdir -p $(BUILD)/cov
	cd $(BUILD)/cov && $(CC) -std=c99 --coverage -O0 -I$(CURDIR)/include $(CURDIR)/src/*.c $(CURDIR)/tests/main_test_bench.c -o bench && ./bench > /dev/null
	@for f in $(MODULES); do echo "$$f:"; (cd $(BUILD)/cov && gcov -b -c bench-$$f.gcda | sed -n '2,4p'); done
.PHONY: sil coverage

