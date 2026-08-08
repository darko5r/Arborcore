# ============================================================
# Arborcore build system
#
# Repository layout:
#
#   src/asm/       production Assembly
#   tests/asm/     Assembly tests
#   bench/         benchmarks
#   tools/         development/qualification tools
#   docs/          documentation
#   generated/     machine-specific generated policy
#   build/         all compiled artifacts
# ============================================================


SHELL := /bin/bash

.DEFAULT_GOAL := all


# ============================================================
# Toolchain
# ============================================================

NASM ?= nasm
LD ?= ld
READELF ?= readelf


NASMFLAGS := \
	-f elf64 \
	-Wall \
	-Wno-reloc-rel-dword \
	-Wno-reloc-abs-qword


# ============================================================
# Repository paths
# ============================================================

ROOT_DIR := $(CURDIR)

SRC_ASM_DIR := src/asm
TEST_ASM_DIR := tests/asm
BENCH_DIR := bench
TOOLS_DIR := tools
BUILD_DIR := build
GENERATED_DIR := generated


# ============================================================
# Generated policy
# ============================================================

MEMORY_POLICY := $(GENERATED_DIR)/memory_thresholds.inc


# ============================================================
# Production objects
# ============================================================

START_OBJ := $(BUILD_DIR)/start.o
WRITE_OBJ := $(BUILD_DIR)/write.o
MEMORY_THRESHOLD_OBJ := $(BUILD_DIR)/memory_threshold.o
MEMORY_OBJ := $(BUILD_DIR)/memory.o


ARBORCORE_OBJECTS := \
	$(START_OBJ) \
	$(WRITE_OBJ) \
	$(MEMORY_THRESHOLD_OBJ) \
	$(MEMORY_OBJ)


# ============================================================
# Test objects
# ============================================================

WRITE_TEST_OBJ := $(BUILD_DIR)/write_test.o
MEMORY_THRESHOLD_TEST_OBJ := $(BUILD_DIR)/memory_threshold_test.o
MEMORY_TEST_OBJ := $(BUILD_DIR)/memory_test.o


TEST_OBJECTS := \
	$(WRITE_TEST_OBJ) \
	$(MEMORY_THRESHOLD_TEST_OBJ) \
	$(MEMORY_TEST_OBJ)


# ============================================================
# Executables
# ============================================================

ARBORCORE := $(BUILD_DIR)/arborcore

WRITE_TEST := $(BUILD_DIR)/write-test
MEMORY_THRESHOLD_TEST := $(BUILD_DIR)/memory-threshold-test
MEMORY_TEST := $(BUILD_DIR)/memory-test


# ============================================================
# Benchmark / qualification sources
# ============================================================

MEMORY_BENCH_ASM := $(BENCH_DIR)/memory_bench.asm
MEMORY_BENCH_RUNNER := $(BENCH_DIR)/memory_bench_run.sh

MEMORY_QUALIFIER := $(TOOLS_DIR)/memory_threshold_qualify.sh


# ============================================================
# Phony targets
# ============================================================

.PHONY: all
.PHONY: run
.PHONY: check
.PHONY: benchmark
.PHONY: qualify-memory
.PHONY: show-memory-policy
.PHONY: clean
.PHONY: distclean


# ============================================================
# Main build
# ============================================================

all: $(ARBORCORE)


$(ARBORCORE): $(ARBORCORE_OBJECTS)
	$(LD) -o $@ $(ARBORCORE_OBJECTS)


run: $(ARBORCORE)
	./$(ARBORCORE)


# ============================================================
# Directory creation
# ============================================================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


$(GENERATED_DIR):
	mkdir -p $(GENERATED_DIR)


# ============================================================
# Generated memory-copy policy
#
# A fresh clone will not contain the machine-specific generated
# file because /generated/ is ignored by Git.
#
# These are safe fallback values.
#
# `make qualify-memory` replaces them using measurements from
# the current machine.
# ============================================================

$(MEMORY_POLICY): | $(GENERATED_DIR)
	@printf '%s\n' \
		'; Arborcore fallback memory-copy policy.' \
		';' \
		'; Automatically replaceable with:' \
		';' \
		';   make qualify-memory' \
		';' \
		'' \
		'%define MEMORY_COPY_QWORD_MIN 8' \
		'%define MEMORY_COPY_REP_MIN   160' \
		> $@


show-memory-policy: $(MEMORY_POLICY)
	@cat $(MEMORY_POLICY)


# ============================================================
# Production Assembly
#
# Generic rule handles normal production .asm files.
# ============================================================

$(BUILD_DIR)/%.o: $(SRC_ASM_DIR)/%.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@


# memory.asm depends on the generated machine policy.

$(MEMORY_OBJ): \
	$(SRC_ASM_DIR)/memory.asm \
	$(MEMORY_POLICY) \
	| $(BUILD_DIR)

	$(NASM) $(NASMFLAGS) \
		$(SRC_ASM_DIR)/memory.asm \
		-o $(MEMORY_OBJ)


# memory_threshold.asm exposes the exact same policy for
# diagnostics/tests and therefore has the same dependency.

$(MEMORY_THRESHOLD_OBJ): \
	$(SRC_ASM_DIR)/memory_threshold.asm \
	$(MEMORY_POLICY) \
	| $(BUILD_DIR)

	$(NASM) $(NASMFLAGS) \
		$(SRC_ASM_DIR)/memory_threshold.asm \
		-o $(MEMORY_THRESHOLD_OBJ)


# ============================================================
# Test objects
# ============================================================

$(WRITE_TEST_OBJ): \
	$(TEST_ASM_DIR)/write_test.asm \
	| $(BUILD_DIR)

	$(NASM) $(NASMFLAGS) \
		$(TEST_ASM_DIR)/write_test.asm \
		-o $(WRITE_TEST_OBJ)


$(MEMORY_THRESHOLD_TEST_OBJ): \
	$(TEST_ASM_DIR)/memory_threshold_test.asm \
	| $(BUILD_DIR)

	$(NASM) $(NASMFLAGS) \
		$(TEST_ASM_DIR)/memory_threshold_test.asm \
		-o $(MEMORY_THRESHOLD_TEST_OBJ)


$(MEMORY_TEST_OBJ): \
	$(TEST_ASM_DIR)/memory_test.asm \
	| $(BUILD_DIR)

	$(NASM) $(NASMFLAGS) \
		$(TEST_ASM_DIR)/memory_test.asm \
		-o $(MEMORY_TEST_OBJ)


# ============================================================
# Test executables
# ============================================================

$(WRITE_TEST): \
	$(WRITE_TEST_OBJ) \
	$(WRITE_OBJ)

	$(LD) -o $@ \
		$(WRITE_TEST_OBJ) \
		$(WRITE_OBJ)


$(MEMORY_THRESHOLD_TEST): \
	$(MEMORY_THRESHOLD_TEST_OBJ) \
	$(MEMORY_THRESHOLD_OBJ)

	$(LD) -o $@ \
		$(MEMORY_THRESHOLD_TEST_OBJ) \
		$(MEMORY_THRESHOLD_OBJ)


$(MEMORY_TEST): \
	$(MEMORY_TEST_OBJ) \
	$(MEMORY_OBJ)

	$(LD) -o $@ \
		$(MEMORY_TEST_OBJ) \
		$(MEMORY_OBJ)


# ============================================================
# Complete foundation verification
# ============================================================

check: \
	$(ARBORCORE) \
	$(WRITE_TEST) \
	$(MEMORY_THRESHOLD_TEST) \
	$(MEMORY_TEST)

	@set -eu; \
	tmp="$$(mktemp "$(BUILD_DIR)/arborcore-check.XXXXXX")"; \
	trap 'rm -f "$$tmp"' EXIT; \
	\
	echo "### Arborcore output"; \
	$(ARBORCORE) > "$$tmp"; \
	printf 'Arborcore: Assembly core active\n' | cmp -s - "$$tmp"; \
	echo "PASS: startup output"; \
	\
	echo; \
	echo "### Arborcore startup failure"; \
	status=0; \
	$(ARBORCORE) 1>&- || status=$$?; \
	if [ "$$status" -ne 1 ]; then \
		echo "FAIL: expected exit 1, got $$status"; \
		exit 1; \
	fi; \
	echo "PASS: closed stdout -> exit 1"; \
	\
	echo; \
	echo "### write_all"; \
	status=0; \
	$(WRITE_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: write-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: write-test"; \
	\
	echo; \
	echo "### memory thresholds"; \
	status=0; \
	$(MEMORY_THRESHOLD_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: memory-threshold-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: memory-threshold-test"; \
	echo "Current policy:"; \
	grep '^%define MEMORY_COPY_' $(MEMORY_POLICY); \
	\
	echo; \
	echo "### memory"; \
	status=0; \
	$(MEMORY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: memory-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: memory-test"; \
	\
	echo; \
	echo "### GNU-stack notes"; \
	for obj in $(ARBORCORE_OBJECTS) $(TEST_OBJECTS); do \
		if ! $(READELF) -SW "$$obj" | grep -q '\.note.GNU-stack'; then \
			echo "FAIL: $$obj has no .note.GNU-stack"; \
			exit 1; \
		fi; \
		echo "PASS: $$obj"; \
	done; \
	\
	echo; \
	echo "### Final stack permissions"; \
	if ! $(READELF) -lW $(ARBORCORE) \
		| grep 'GNU_STACK' \
		| grep -q ' RW '; then \
		echo "FAIL: expected writable, non-executable GNU_STACK"; \
		exit 1; \
	fi; \
	\
	if $(READELF) -lW $(ARBORCORE) \
		| grep 'GNU_STACK' \
		| grep -q 'RWE'; then \
		echo "FAIL: executable stack"; \
		exit 1; \
	fi; \
	echo "PASS: GNU_STACK is non-executable"; \
	\
	echo; \
	echo "ALL ARBORCORE FOUNDATION TESTS PASSED"


# ============================================================
# Memory benchmark
#
# The existing benchmark script expects these files in its
# current working directory:
#
#   memory_bench.asm
#   memory.o
#   write.o
#
# Instead of polluting the repository root, Make creates a
# temporary compatibility workspace inside build/.
# ============================================================

benchmark: \
	$(MEMORY_OBJ) \
	$(WRITE_OBJ) \
	$(MEMORY_BENCH_ASM) \
	$(MEMORY_BENCH_RUNNER)

	@set -eu; \
	workspace="$(ROOT_DIR)/$(BUILD_DIR)/.memory-benchmark-work"; \
	rm -rf "$$workspace"; \
	mkdir -p "$$workspace"; \
	trap 'rm -rf "$$workspace"' EXIT; \
	\
	ln -s \
		"$(ROOT_DIR)/$(MEMORY_BENCH_ASM)" \
		"$$workspace/memory_bench.asm"; \
	\
	ln -s \
		"$(ROOT_DIR)/$(MEMORY_OBJ)" \
		"$$workspace/memory.o"; \
	\
	ln -s \
		"$(ROOT_DIR)/$(WRITE_OBJ)" \
		"$$workspace/write.o"; \
	\
	cd "$$workspace"; \
	bash "$(ROOT_DIR)/$(MEMORY_BENCH_RUNNER)"


# ============================================================
# Machine-specific memory qualification
#
# The existing qualification script also expects:
#
#   memory_bench.asm
#   memory.o
#   write.o
#   generated/
#
# Make provides those through a temporary workspace.
#
# The generated/ entry is a symlink to the real repository
# generated directory, so the selected thresholds are written
# to:
#
#   generated/memory_thresholds.inc
#
# After qualification, all affected objects/executables are
# rebuilt and the complete test gate runs automatically.
# ============================================================

qualify-memory: \
	$(MEMORY_OBJ) \
	$(WRITE_OBJ) \
	$(MEMORY_POLICY) \
	$(MEMORY_BENCH_ASM) \
	$(MEMORY_QUALIFIER)

	@set -eu; \
	workspace="$(ROOT_DIR)/$(BUILD_DIR)/.memory-qualification-work"; \
	rm -rf "$$workspace"; \
	mkdir -p "$$workspace"; \
	trap 'rm -rf "$$workspace"' EXIT; \
	\
	ln -s \
		"$(ROOT_DIR)/$(MEMORY_BENCH_ASM)" \
		"$$workspace/memory_bench.asm"; \
	\
	ln -s \
		"$(ROOT_DIR)/$(MEMORY_OBJ)" \
		"$$workspace/memory.o"; \
	\
	ln -s \
		"$(ROOT_DIR)/$(WRITE_OBJ)" \
		"$$workspace/write.o"; \
	\
	ln -s \
		"$(ROOT_DIR)/$(GENERATED_DIR)" \
		"$$workspace/generated"; \
	\
	cd "$$workspace"; \
	bash "$(ROOT_DIR)/$(MEMORY_QUALIFIER)"

	$(MAKE) \
		$(MEMORY_THRESHOLD_OBJ) \
		$(MEMORY_OBJ) \
		$(ARBORCORE) \
		$(MEMORY_THRESHOLD_TEST) \
		$(MEMORY_TEST)

	$(MAKE) check


# ============================================================
# Cleanup
#
# `clean` removes reproducible build products only.
#
# The qualified machine policy is deliberately preserved.
# ============================================================

clean:
	rm -rf $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)


# ============================================================
# Full reset
#
# `distclean` also removes the qualified machine policy.
#
# The next `make` will recreate fallback values:
#
#   QWORD_MIN = 8
#   REP_MIN   = 160
# ============================================================

distclean: clean
	rm -f $(MEMORY_POLICY)
