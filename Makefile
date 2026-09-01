# ============================================================
# Arborcore build system
# ============================================================

SHELL := /bin/bash
.DEFAULT_GOAL := all

# Toolchain
NASM ?= nasm
LD ?= ld
READELF ?= readelf
AR ?= ar
NM ?= nm
OBJDUMP ?= objdump
SHA256SUM ?= sha256sum

NASMFLAGS := \
	-f elf64 \
	-Wall \
	-Wno-reloc-rel-dword \
	-Wno-reloc-abs-qword

# Repository paths
ROOT_DIR := $(CURDIR)
SRC_ASM_DIR := src/asm
TEST_ASM_DIR := tests/asm
BENCH_DIR := bench
TOOLS_DIR := tools
BUILD_DIR := build
GENERATED_DIR := generated

# Library installation policy
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
DATADIR ?= $(PREFIX)/share
DESTDIR ?=
ARBORCORE_ABI_INSTALL_DIR ?= $(DATADIR)/arborcore/abi

# Generated memory policy
MEMORY_POLICY := $(GENERATED_DIR)/memory_thresholds.inc
MEMORY_PERF_PROFILE = $(GENERATED_DIR)/performance/memory-$(ARBORCORE_PERF_PROFILE).env

# Production objects
START_OBJ := $(BUILD_DIR)/start.o
WRITE_OBJ := $(BUILD_DIR)/write.o
MEMORY_THRESHOLD_OBJ := $(BUILD_DIR)/memory_threshold.o
MEMORY_OBJ := $(BUILD_DIR)/memory.o
SECURITY_OBJ := $(BUILD_DIR)/security.o
BYTES_OBJ := $(BUILD_DIR)/bytes.o
ASCII_OBJ := $(BUILD_DIR)/ascii.o
BYTES_SCAN_OBJ := $(BUILD_DIR)/bytes_scan.o
PARSE_U64_OBJ := $(BUILD_DIR)/parse_u64.o
U64_CHECKED_OBJ := $(BUILD_DIR)/u64_checked.o
RANGE_OBJ := $(BUILD_DIR)/range.o
U64_FORMAT_OBJ := $(BUILD_DIR)/u64_format.o
HEX_CODEC_OBJ := $(BUILD_DIR)/hex_codec.o
PERCENT_CODEC_OBJ := $(BUILD_DIR)/percent_codec.o
BASE64_OBJ := $(BUILD_DIR)/base64.o
BUFFER_OBJ := $(BUILD_DIR)/buffer.o
ARENA_OBJ := $(BUILD_DIR)/arena.o
IO_OBJ := $(BUILD_DIR)/io.o
NET_OBJ := $(BUILD_DIR)/net.o
HTTP_PARSER_OBJ := $(BUILD_DIR)/http_parser.o
ROUTER_OBJ := $(BUILD_DIR)/router.o
EVENT_OBJ := $(BUILD_DIR)/event.o
CONNECTION_OBJ := $(BUILD_DIR)/connection.o
HTTP_RESPONSE_OBJ := $(BUILD_DIR)/http_response.o
REQUEST_TARGET_OBJ := $(BUILD_DIR)/request_target.o
ROUTE_PATTERN_OBJ := $(BUILD_DIR)/route_pattern.o
SERVER_OBJ := $(BUILD_DIR)/server.o

ARBORCORE_OBJECTS := \
	$(START_OBJ) \
	$(WRITE_OBJ) \
	$(MEMORY_THRESHOLD_OBJ) \
	$(MEMORY_OBJ) \
	$(SECURITY_OBJ) \
	$(BYTES_OBJ) \
	$(ASCII_OBJ) \
	$(BYTES_SCAN_OBJ) \
	$(PARSE_U64_OBJ) \
	$(U64_CHECKED_OBJ) \
	$(RANGE_OBJ) \
	$(U64_FORMAT_OBJ) \
	$(HEX_CODEC_OBJ) \
	$(PERCENT_CODEC_OBJ) \
	$(BASE64_OBJ) \
	$(BUFFER_OBJ) \
	$(ARENA_OBJ) \
	$(IO_OBJ) \
	$(NET_OBJ) \
	$(HTTP_PARSER_OBJ) \
	$(ROUTER_OBJ) \
	$(EVENT_OBJ) \
	$(CONNECTION_OBJ) \
	$(HTTP_RESPONSE_OBJ) \
	$(REQUEST_TARGET_OBJ) \
	$(ROUTE_PATTERN_OBJ) \
	$(SERVER_OBJ)

# Test objects
WRITE_TEST_OBJ := $(BUILD_DIR)/write_test.o
MEMORY_THRESHOLD_TEST_OBJ := $(BUILD_DIR)/memory_threshold_test.o
MEMORY_TEST_OBJ := $(BUILD_DIR)/memory_test.o
BYTES_TEST_OBJ := $(BUILD_DIR)/bytes_test.o
BYTES_PHASE2_TEST_OBJ := $(BUILD_DIR)/bytes_phase2_test.o
NUMERIC_TEST_OBJ := $(BUILD_DIR)/numeric_test.o
CORE_INTEGER_TEST_OBJ := $(BUILD_DIR)/core_integer_test.o
CORE_RANGE_TEST_OBJ := $(BUILD_DIR)/core_range_test.o
CORE_SEQUENCE_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_sequence_property_test.o
CORE_NUMERIC_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_numeric_property_test.o
CORE_CODEC_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_codec_property_test.o
CORE_MEMORY_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_memory_property_test.o
CORE_BUFFER_ARENA_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_buffer_arena_property_test.o
CORE_CONNECTION_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_connection_property_test.o
CORE_BUFFER_ALIAS_TEST_OBJ := $(BUILD_DIR)/core_buffer_alias_test.o
CORE_REQUEST_TARGET_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_request_target_property_test.o
CORE_ROUTE_CONTRACT_TEST_OBJ := $(BUILD_DIR)/core_route_contract_test.o
CORE_HTTP_RESPONSE_CONTRACT_TEST_OBJ := $(BUILD_DIR)/core_http_response_contract_test.o
CORE_ROUTE_INDEX_EXPERIMENT_TEST_OBJ := $(BUILD_DIR)/core_route_index_experiment_test.o
ENCODING_TEST_OBJ := $(BUILD_DIR)/encoding_test.o
BUFFER_TEST_OBJ := $(BUILD_DIR)/buffer_test.o
ARENA_TEST_OBJ := $(BUILD_DIR)/arena_test.o
POLISH_GATE1_TEST_OBJ := $(BUILD_DIR)/polish_gate1_test.o
POLISH_GATE2_TEST_OBJ := $(BUILD_DIR)/polish_gate2_test.o
IO_TEST_OBJ := $(BUILD_DIR)/io_test.o
NET_TEST_OBJ := $(BUILD_DIR)/net_test.o
HTTP_TEST_OBJ := $(BUILD_DIR)/http_test.o
ROUTER_TEST_OBJ := $(BUILD_DIR)/router_test.o
EVENT_TEST_OBJ := $(BUILD_DIR)/event_test.o
CONNECTION_TEST_OBJ := $(BUILD_DIR)/connection_test.o
HTTP_RESPONSE_TEST_OBJ := $(BUILD_DIR)/http_response_test.o
REQUEST_TARGET_TEST_OBJ := $(BUILD_DIR)/request_target_test.o
ROUTE_PATTERN_TEST_OBJ := $(BUILD_DIR)/route_pattern_test.o
SERVER_TEST_OBJ := $(BUILD_DIR)/server_test.o
POLISH_GATE3_TEST_OBJ := $(BUILD_DIR)/polish_gate3_test.o

TEST_OBJECTS := \
	$(WRITE_TEST_OBJ) \
	$(MEMORY_THRESHOLD_TEST_OBJ) \
	$(MEMORY_TEST_OBJ) \
	$(BYTES_TEST_OBJ) \
	$(BYTES_PHASE2_TEST_OBJ) \
	$(NUMERIC_TEST_OBJ) \
	$(CORE_INTEGER_TEST_OBJ) \
	$(CORE_RANGE_TEST_OBJ) \
	$(CORE_SEQUENCE_PROPERTY_TEST_OBJ) \
	$(CORE_NUMERIC_PROPERTY_TEST_OBJ) \
	$(CORE_CODEC_PROPERTY_TEST_OBJ) \
	$(CORE_MEMORY_PROPERTY_TEST_OBJ) \
	$(CORE_BUFFER_ARENA_PROPERTY_TEST_OBJ) \
	$(CORE_CONNECTION_PROPERTY_TEST_OBJ) \
	$(CORE_BUFFER_ALIAS_TEST_OBJ) \
	$(CORE_REQUEST_TARGET_PROPERTY_TEST_OBJ) \
	$(CORE_ROUTE_CONTRACT_TEST_OBJ) \
	$(CORE_HTTP_RESPONSE_CONTRACT_TEST_OBJ) \
	$(CORE_ROUTE_INDEX_EXPERIMENT_TEST_OBJ) \
	$(ENCODING_TEST_OBJ) \
	$(BUFFER_TEST_OBJ) \
	$(ARENA_TEST_OBJ) \
	$(POLISH_GATE1_TEST_OBJ) \
	$(POLISH_GATE2_TEST_OBJ) \
	$(IO_TEST_OBJ) \
	$(NET_TEST_OBJ) \
	$(HTTP_TEST_OBJ) \
	$(ROUTER_TEST_OBJ) \
	$(EVENT_TEST_OBJ) \
	$(CONNECTION_TEST_OBJ) \
	$(HTTP_RESPONSE_TEST_OBJ) \
	$(REQUEST_TARGET_TEST_OBJ) \
	$(ROUTE_PATTERN_TEST_OBJ) \
	$(SERVER_TEST_OBJ) \
	$(POLISH_GATE3_TEST_OBJ)

TEST_OBJECTS += \
	$(BUILD_DIR)/core_http_framing_property_test.o \
	$(BUILD_DIR)/core_accept_transaction_test.o \
	$(BUILD_DIR)/core_pipeline_budget_test.o \
	$(BUILD_DIR)/core_event_batch_test.o \
	$(BUILD_DIR)/core_writev_experiment_test.o
TEST_OBJECTS += \
	$(BUILD_DIR)/core_security_property_test.o \
	$(BUILD_DIR)/abi_consumer_test.o

# Executables
ARBORCORE := $(BUILD_DIR)/arborcore
WRITE_TEST := $(BUILD_DIR)/write-test
MEMORY_THRESHOLD_TEST := $(BUILD_DIR)/memory-threshold-test
MEMORY_TEST := $(BUILD_DIR)/memory-test
BYTES_TEST := $(BUILD_DIR)/bytes-test
BYTES_PHASE2_TEST := $(BUILD_DIR)/bytes-phase2-test
NUMERIC_TEST := $(BUILD_DIR)/numeric-test
CORE_INTEGER_TEST := $(BUILD_DIR)/core-integer-test
CORE_RANGE_TEST := $(BUILD_DIR)/core-range-test
CORE_SEQUENCE_PROPERTY_TEST := $(BUILD_DIR)/core-sequence-property-test
CORE_NUMERIC_PROPERTY_TEST := $(BUILD_DIR)/core-numeric-property-test
CORE_CODEC_PROPERTY_TEST := $(BUILD_DIR)/core-codec-property-test
CORE_MEMORY_PROPERTY_TEST := $(BUILD_DIR)/core-memory-property-test
CORE_BUFFER_ARENA_PROPERTY_TEST := $(BUILD_DIR)/core-buffer-arena-property-test
CORE_CONNECTION_PROPERTY_TEST := $(BUILD_DIR)/core-connection-property-test
CORE_BUFFER_ALIAS_TEST := $(BUILD_DIR)/core-buffer-alias-test
CORE_REQUEST_TARGET_PROPERTY_TEST := $(BUILD_DIR)/core-request-target-property-test
CORE_ROUTE_CONTRACT_TEST := $(BUILD_DIR)/core-route-contract-test
CORE_HTTP_RESPONSE_CONTRACT_TEST := $(BUILD_DIR)/core-http-response-contract-test
CORE_ROUTE_INDEX_EXPERIMENT_TEST := $(BUILD_DIR)/core-route-index-experiment-test
ENCODING_TEST := $(BUILD_DIR)/encoding-test
BUFFER_TEST := $(BUILD_DIR)/buffer-test
ARENA_TEST := $(BUILD_DIR)/arena-test
POLISH_GATE1_TEST := $(BUILD_DIR)/polish-gate1-test
POLISH_GATE2_TEST := $(BUILD_DIR)/polish-gate2-test
IO_TEST := $(BUILD_DIR)/io-test
NET_TEST := $(BUILD_DIR)/net-test
HTTP_TEST := $(BUILD_DIR)/http-test
ROUTER_TEST := $(BUILD_DIR)/router-test
EVENT_TEST := $(BUILD_DIR)/event-test
CONNECTION_TEST := $(BUILD_DIR)/connection-test
HTTP_RESPONSE_TEST := $(BUILD_DIR)/http-response-test
REQUEST_TARGET_TEST := $(BUILD_DIR)/request-target-test
ROUTE_PATTERN_TEST := $(BUILD_DIR)/route-pattern-test
SERVER_TEST := $(BUILD_DIR)/server-test
POLISH_GATE3_TEST := $(BUILD_DIR)/polish-gate3-test
CORE_SECURITY_PROPERTY_TEST := $(BUILD_DIR)/core-security-property-test

# Assembly ABI/library products
ARBORCORE_LIBRARY_OBJECTS := $(filter-out $(START_OBJ),$(ARBORCORE_OBJECTS))
LIBRARY_RELEASE_VERSION := 1.0.0
LIBRARY_ABI_MAJOR := 1
STATIC_LIB_BASENAME := libarborcore.a
SHARED_LIB_FULL_BASENAME := libarborcore.so.$(LIBRARY_RELEASE_VERSION)
SHARED_LIB_SONAME_BASENAME := libarborcore.so.$(LIBRARY_ABI_MAJOR)
SHARED_LIB_LINK_BASENAME := libarborcore.so
STATIC_LIB := $(BUILD_DIR)/$(STATIC_LIB_BASENAME)
SHARED_LIB_FULL := $(BUILD_DIR)/$(SHARED_LIB_FULL_BASENAME)
SHARED_LIB := $(BUILD_DIR)/$(SHARED_LIB_SONAME_BASENAME)
SHARED_LIB_LINK := $(BUILD_DIR)/$(SHARED_LIB_LINK_BASENAME)
ABI_STATIC_CONSUMER := $(BUILD_DIR)/abi-static-consumer
ABI_SHARED_CONSUMER := $(BUILD_DIR)/abi-shared-consumer
ABI_PUBLIC_SYMBOLS := abi/arborcore-1.symbols
ABI_INTERNAL_SYMBOLS := abi/arborcore-1.internal-symbols
ABI_VERSION_SCRIPT := abi/arborcore-1.map
ABI_LAYOUT := abi/arborcore-1.layout
ABI_FREEZE_MANIFEST := abi/arborcore-1.freeze
ABI_README := abi/README.md
LIBRARY_INSTALL_MANIFEST := packaging/arborcore-library-files.list

# Benchmark / qualification sources
MEMORY_BENCH_ASM := $(BENCH_DIR)/memory_bench.asm
MEMORY_BENCH_RUNNER := $(BENCH_DIR)/memory_bench_run.sh
MEMORY_QUALIFIER := $(TOOLS_DIR)/memory_threshold_qualify.sh
POLISH_GATE1 := $(TOOLS_DIR)/polish_gate1.sh
POLISH_GATE2 := $(TOOLS_DIR)/polish_gate2.sh
POLISH_GATE3 := $(TOOLS_DIR)/polish_gate3.sh

# Server performance benchmark infrastructure
BENCH_SUPPORT_OBJ := $(BUILD_DIR)/bench_support.o
PARSER_BENCH_OBJ := $(BUILD_DIR)/parser_bench.o
ROUTING_BENCH_OBJ := $(BUILD_DIR)/routing_bench.o
RESPONSE_BENCH_OBJ := $(BUILD_DIR)/response_bench.o
LIFECYCLE_BENCH_OBJ := $(BUILD_DIR)/lifecycle_bench.o
CONNECTION_BENCH_OBJ := $(BUILD_DIR)/connection_bench.o
LOOPBACK_BENCH_OBJ := $(BUILD_DIR)/loopback_bench.o
CODEC_BENCH_OBJ := $(BUILD_DIR)/codec_bench.o
ROUTE_INDEX_CANDIDATE_OBJ := $(BUILD_DIR)/route_index_candidate.o
ROUTE_INDEX_BENCH_OBJ := $(BUILD_DIR)/route_index_bench.o

PARSER_BENCH := $(BUILD_DIR)/bench-parser
ROUTING_BENCH := $(BUILD_DIR)/bench-routing
RESPONSE_BENCH := $(BUILD_DIR)/bench-response
LIFECYCLE_BENCH := $(BUILD_DIR)/bench-lifecycle
CONNECTION_BENCH := $(BUILD_DIR)/bench-connections
LOOPBACK_BENCH := $(BUILD_DIR)/bench-loopback
CODEC_BENCH := $(BUILD_DIR)/bench-codec
ROUTE_INDEX_BENCH := $(BUILD_DIR)/bench-route-index

ARBORCORE_PERF_PROFILE ?= local
export ARBORCORE_PERF_PROFILE

SERVER_BENCHMARK_RUNNER := $(TOOLS_DIR)/server_benchmark_run.sh
SERVER_BASELINE_QUALIFIER := $(TOOLS_DIR)/server_baseline_qualify.sh
SERVER_BASELINE_ACCEPTOR := $(TOOLS_DIR)/server_baseline_accept.sh
SERVER_PERFORMANCE_VERIFY := $(TOOLS_DIR)/server_performance_verify.sh
SERVER_BENCHMARK_PERF := $(TOOLS_DIR)/server_benchmark_perf.sh
SERVER_BENCHMARK_SYSCALLS := $(TOOLS_DIR)/server_benchmark_syscalls.sh
SERVER_PERF_BASELINE := $(GENERATED_DIR)/performance/$(ARBORCORE_PERF_PROFILE).env
CODEC_BENCHMARK_RUNNER := $(TOOLS_DIR)/codec_benchmark_run.sh
CODEC_BASELINE_QUALIFIER := $(TOOLS_DIR)/codec_baseline_qualify.sh
CODEC_PERFORMANCE_VERIFY := $(TOOLS_DIR)/codec_performance_verify.sh
ROUTE_INDEX_EXPERIMENT_RUNNER := $(TOOLS_DIR)/route_index_experiment_run.sh
ROUTE_INDEX_EXPERIMENT_VERIFY := $(TOOLS_DIR)/route_index_experiment_verify.sh
SERVER_ENVIRONMENT_ADMISSIBILITY := $(TOOLS_DIR)/server_environment_admissibility.sh
SERVER_PAIRED_COMPARE := $(TOOLS_DIR)/server_paired_compare.sh
SERVER_PERFORMANCE_QUALIFIED := $(TOOLS_DIR)/server_performance_qualified.sh
CORE_RETROFIT_D_GATE := $(TOOLS_DIR)/core_retrofit_d_gate.sh
CORE_RETROFIT_D_REFERENCE ?= 24e014b791b46c4b7c8dffd2dee14dcb8eb4354a
CORE_HTTP_FRAMING_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_http_framing_property_test.o
CORE_ACCEPT_TRANSACTION_TEST_OBJ := $(BUILD_DIR)/core_accept_transaction_test.o
CORE_PIPELINE_BUDGET_TEST_OBJ := $(BUILD_DIR)/core_pipeline_budget_test.o
CORE_EVENT_BATCH_TEST_OBJ := $(BUILD_DIR)/core_event_batch_test.o
CORE_WRITEV_EXPERIMENT_TEST_OBJ := $(BUILD_DIR)/core_writev_experiment_test.o
CORE_SECURITY_PROPERTY_TEST_OBJ := $(BUILD_DIR)/core_security_property_test.o
ABI_CONSUMER_TEST_OBJ := $(BUILD_DIR)/abi_consumer_test.o
CORE_HTTP_FRAMING_PROPERTY_TEST := $(BUILD_DIR)/core-http-framing-property-test
CORE_ACCEPT_TRANSACTION_TEST := $(BUILD_DIR)/core-accept-transaction-test
CORE_PIPELINE_BUDGET_TEST := $(BUILD_DIR)/core-pipeline-budget-test
CORE_EVENT_BATCH_TEST := $(BUILD_DIR)/core-event-batch-test
CORE_WRITEV_EXPERIMENT_TEST := $(BUILD_DIR)/core-writev-experiment-test
RESPONSE_IOVEC_CANDIDATE_OBJ := $(BUILD_DIR)/response_iovec_candidate.o
IOVEC_WRITE_CANDIDATE_OBJ := $(BUILD_DIR)/iovec_write_candidate.o
RESPONSE_IOVEC_BENCH_OBJ := $(BUILD_DIR)/response_iovec_bench.o
RESPONSE_IOVEC_BENCH := $(BUILD_DIR)/bench-writev-experiment
WRITEV_EXPERIMENT_VERIFY := $(TOOLS_DIR)/writev_experiment_verify.sh
RUNTIME_SYSCALL_VERIFY := $(TOOLS_DIR)/runtime_syscall_verify.sh
CORE_RETROFIT_E_GATE := $(TOOLS_DIR)/core_retrofit_e_gate.sh
ABI_SURFACE_VERIFY := $(TOOLS_DIR)/abi_surface_verify.sh
ABI_DEPENDENCY_VERIFY := $(TOOLS_DIR)/abi_dependency_verify.sh
ABI_LAYOUT_VERIFY := $(TOOLS_DIR)/abi_layout_verify.sh
SECURITY_SHAPE_VERIFY := $(TOOLS_DIR)/security_shape_verify.sh
LIBRARY_READINESS_VERIFY := $(TOOLS_DIR)/library_readiness_verify.sh
LIBRARY_REPRODUCIBILITY_VERIFY := $(TOOLS_DIR)/library_reproducibility_verify.sh
LIBRARY_INSTALL_VERIFY := $(TOOLS_DIR)/library_install_verify.sh
LIBRARY_RELEASE_GATE := $(TOOLS_DIR)/library_release_gate.sh
ASSEMBLY_ABI_FREEZE := $(TOOLS_DIR)/assembly_abi_freeze.sh
ASSEMBLY_SECURITY_ABI_GATE := $(TOOLS_DIR)/assembly_security_abi_gate.sh
ASSEMBLY_SECURITY_ABI_REFERENCE := 95c0229f8baed13583f0a55ddb6d097e2d38146f
CORE_RETROFIT_E_REFERENCE ?= e9b69ab5205033dac15128ff7e3fd6d627548cb2
CODEC_PERF_BASELINE := $(GENERATED_DIR)/performance/codec-$(ARBORCORE_PERF_PROFILE).env

.PHONY: all run check
.PHONY: write-test memory-threshold-test memory-test bytes-test bytes-phase2-test numeric-test core-integer-test core-range-test encoding-test
.PHONY: core-sequence-property-test core-numeric-property-test core-codec-property-test core-retrofit-b1
.PHONY: core-memory-property-test core-buffer-arena-property-test core-connection-property-test core-buffer-alias-test core-retrofit-c1 core-retrofit-c
.PHONY: core-request-target-property-test core-route-contract-test core-http-response-contract-test core-route-index-experiment-test core-retrofit-d
.PHONY: route-index-experiment verify-server-environment verify-server-performance-qualified-candidate core-retrofit-d-gate
.PHONY: core-http-framing-property-test core-accept-transaction-test core-pipeline-budget-test core-event-batch-test core-writev-experiment-test core-retrofit-e
.PHONY: writev-experiment runtime-syscall-experiment core-retrofit-e-gate
.PHONY: buffer-test arena-test polish-gate-1 polish-gate-2 io-test net-test http-test router-test
.PHONY: event-test connection-test http-response-test request-target-test route-pattern-test server-test polish-gate-3
.PHONY: benchmark qualify-memory show-memory-policy show-memory-profile
.PHONY: benchmark-server qualify-server-baseline accept-server-baseline show-server-baseline list-server-profiles verify-server-performance verify-server-performance-candidate
.PHONY: benchmark-server-perf benchmark-server-syscalls
.PHONY: benchmark-codec qualify-codec-baseline show-codec-baseline verify-codec-performance verify-codec-performance-candidate
.PHONY: core-security-property-test abi-surface-verify abi-dependency-verify abi-layout-verify security-shape-verify library-readiness assembly-security-abi-gate
.PHONY: libarborcore-libraries libarborcore-static libarborcore-shared
.PHONY: install-libraries uninstall-libraries library-reproducibility-verify library-install-verify library-release-gate
.PHONY: clean distclean

# Main build
all: $(ARBORCORE)

$(ARBORCORE): $(ARBORCORE_OBJECTS)
	$(LD) -o $@ $(ARBORCORE_OBJECTS)

run: $(ARBORCORE)
	./$(ARBORCORE)

# Directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GENERATED_DIR):
	mkdir -p $(GENERATED_DIR)

# Generated fallback memory policy
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

show-memory-profile:
	@if [[ -r $(MEMORY_PERF_PROFILE) ]]; then cat $(MEMORY_PERF_PROFILE); else echo "No memory profile at $(MEMORY_PERF_PROFILE)"; fi

# Generic production Assembly rule
$(BUILD_DIR)/%.o: $(SRC_ASM_DIR)/%.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

# Server benchmark objects
$(BENCH_SUPPORT_OBJ): $(BENCH_DIR)/bench_support.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(PARSER_BENCH_OBJ): $(BENCH_DIR)/parser_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(ROUTING_BENCH_OBJ): $(BENCH_DIR)/routing_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(RESPONSE_BENCH_OBJ): $(BENCH_DIR)/response_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(LIFECYCLE_BENCH_OBJ): $(BENCH_DIR)/lifecycle_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CONNECTION_BENCH_OBJ): $(BENCH_DIR)/connection_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(LOOPBACK_BENCH_OBJ): $(BENCH_DIR)/loopback_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CODEC_BENCH_OBJ): $(BENCH_DIR)/codec_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(ROUTE_INDEX_CANDIDATE_OBJ): $(BENCH_DIR)/route_index_candidate.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(ROUTE_INDEX_BENCH_OBJ): $(BENCH_DIR)/route_index_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(RESPONSE_IOVEC_CANDIDATE_OBJ): $(BENCH_DIR)/response_iovec_candidate.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(IOVEC_WRITE_CANDIDATE_OBJ): $(BENCH_DIR)/iovec_write_candidate.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(RESPONSE_IOVEC_BENCH_OBJ): $(BENCH_DIR)/response_iovec_bench.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

# memory.asm depends on generated policy
$(MEMORY_OBJ): $(SRC_ASM_DIR)/memory.asm $(MEMORY_POLICY) | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

# memory_threshold.asm exposes same policy
$(MEMORY_THRESHOLD_OBJ): $(SRC_ASM_DIR)/memory_threshold.asm $(MEMORY_POLICY) | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

# Test objects
$(WRITE_TEST_OBJ): $(TEST_ASM_DIR)/write_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(MEMORY_THRESHOLD_TEST_OBJ): $(TEST_ASM_DIR)/memory_threshold_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(MEMORY_TEST_OBJ): $(TEST_ASM_DIR)/memory_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BYTES_TEST_OBJ): $(TEST_ASM_DIR)/bytes_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BYTES_PHASE2_TEST_OBJ): $(TEST_ASM_DIR)/bytes_phase2_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(NUMERIC_TEST_OBJ): $(TEST_ASM_DIR)/numeric_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_INTEGER_TEST_OBJ): $(TEST_ASM_DIR)/core_integer_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_RANGE_TEST_OBJ): $(TEST_ASM_DIR)/core_range_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_SEQUENCE_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_sequence_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_NUMERIC_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_numeric_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_CODEC_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_codec_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_MEMORY_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_memory_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_BUFFER_ARENA_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_buffer_arena_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_CONNECTION_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_connection_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_BUFFER_ALIAS_TEST_OBJ): $(TEST_ASM_DIR)/core_buffer_alias_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_REQUEST_TARGET_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_request_target_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_ROUTE_CONTRACT_TEST_OBJ): $(TEST_ASM_DIR)/core_route_contract_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_HTTP_RESPONSE_CONTRACT_TEST_OBJ): $(TEST_ASM_DIR)/core_http_response_contract_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_ROUTE_INDEX_EXPERIMENT_TEST_OBJ): $(TEST_ASM_DIR)/core_route_index_experiment_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_HTTP_FRAMING_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_http_framing_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_ACCEPT_TRANSACTION_TEST_OBJ): $(TEST_ASM_DIR)/core_accept_transaction_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_PIPELINE_BUDGET_TEST_OBJ): $(TEST_ASM_DIR)/core_pipeline_budget_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_EVENT_BATCH_TEST_OBJ): $(TEST_ASM_DIR)/core_event_batch_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_WRITEV_EXPERIMENT_TEST_OBJ): $(TEST_ASM_DIR)/core_writev_experiment_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CORE_SECURITY_PROPERTY_TEST_OBJ): $(TEST_ASM_DIR)/core_security_property_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(ABI_CONSUMER_TEST_OBJ): $(TEST_ASM_DIR)/abi_consumer_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(ENCODING_TEST_OBJ): $(TEST_ASM_DIR)/encoding_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUFFER_TEST_OBJ): $(TEST_ASM_DIR)/buffer_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(ARENA_TEST_OBJ): $(TEST_ASM_DIR)/arena_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(POLISH_GATE1_TEST_OBJ): $(TEST_ASM_DIR)/polish_gate1_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(POLISH_GATE2_TEST_OBJ): $(TEST_ASM_DIR)/polish_gate2_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(IO_TEST_OBJ): $(TEST_ASM_DIR)/io_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(NET_TEST_OBJ): $(TEST_ASM_DIR)/net_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(HTTP_TEST_OBJ): $(TEST_ASM_DIR)/http_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(ROUTER_TEST_OBJ): $(TEST_ASM_DIR)/router_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(EVENT_TEST_OBJ): $(TEST_ASM_DIR)/event_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(CONNECTION_TEST_OBJ): $(TEST_ASM_DIR)/connection_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(HTTP_RESPONSE_TEST_OBJ): $(TEST_ASM_DIR)/http_response_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(REQUEST_TARGET_TEST_OBJ): $(TEST_ASM_DIR)/request_target_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(ROUTE_PATTERN_TEST_OBJ): $(TEST_ASM_DIR)/route_pattern_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(SERVER_TEST_OBJ): $(TEST_ASM_DIR)/server_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(POLISH_GATE3_TEST_OBJ): $(TEST_ASM_DIR)/polish_gate3_test.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

# Test executables
$(WRITE_TEST): $(WRITE_TEST_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ $(WRITE_TEST_OBJ) $(WRITE_OBJ)

$(MEMORY_THRESHOLD_TEST): $(MEMORY_THRESHOLD_TEST_OBJ) $(MEMORY_THRESHOLD_OBJ)
	$(LD) -o $@ $(MEMORY_THRESHOLD_TEST_OBJ) $(MEMORY_THRESHOLD_OBJ)

$(MEMORY_TEST): $(MEMORY_TEST_OBJ) $(MEMORY_OBJ)
	$(LD) -o $@ $(MEMORY_TEST_OBJ) $(MEMORY_OBJ)

$(BYTES_TEST): $(BYTES_TEST_OBJ) $(BYTES_OBJ)
	$(LD) -o $@ $(BYTES_TEST_OBJ) $(BYTES_OBJ)

$(BYTES_PHASE2_TEST): \
	$(BYTES_PHASE2_TEST_OBJ) \
	$(ASCII_OBJ) \
	$(BYTES_SCAN_OBJ) \
	$(PARSE_U64_OBJ)
	$(LD) -o $@ \
		$(BYTES_PHASE2_TEST_OBJ) \
		$(ASCII_OBJ) \
		$(BYTES_SCAN_OBJ) \
		$(PARSE_U64_OBJ)

$(NUMERIC_TEST): \
	$(NUMERIC_TEST_OBJ) \
	$(U64_CHECKED_OBJ) \
	$(U64_FORMAT_OBJ)
	$(LD) -o $@ \
		$(NUMERIC_TEST_OBJ) \
		$(U64_CHECKED_OBJ) \
		$(U64_FORMAT_OBJ)

$(CORE_INTEGER_TEST): $(CORE_INTEGER_TEST_OBJ) $(U64_CHECKED_OBJ)
	$(LD) -o $@ $(CORE_INTEGER_TEST_OBJ) $(U64_CHECKED_OBJ)

$(CORE_RANGE_TEST): $(CORE_RANGE_TEST_OBJ) $(RANGE_OBJ)
	$(LD) -o $@ $(CORE_RANGE_TEST_OBJ) $(RANGE_OBJ)

$(CORE_SEQUENCE_PROPERTY_TEST): \
	$(CORE_SEQUENCE_PROPERTY_TEST_OBJ) $(ASCII_OBJ) $(BYTES_OBJ) $(BYTES_SCAN_OBJ)
	$(LD) -o $@ \
		$(CORE_SEQUENCE_PROPERTY_TEST_OBJ) $(ASCII_OBJ) $(BYTES_OBJ) $(BYTES_SCAN_OBJ)

$(CORE_NUMERIC_PROPERTY_TEST): \
	$(CORE_NUMERIC_PROPERTY_TEST_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)
	$(LD) -o $@ \
		$(CORE_NUMERIC_PROPERTY_TEST_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)

$(CORE_CODEC_PROPERTY_TEST): \
	$(CORE_CODEC_PROPERTY_TEST_OBJ) $(HEX_CODEC_OBJ) $(PERCENT_CODEC_OBJ) $(BASE64_OBJ)
	$(LD) -o $@ \
		$(CORE_CODEC_PROPERTY_TEST_OBJ) $(HEX_CODEC_OBJ) $(PERCENT_CODEC_OBJ) $(BASE64_OBJ)

$(CORE_MEMORY_PROPERTY_TEST): $(CORE_MEMORY_PROPERTY_TEST_OBJ) $(MEMORY_OBJ)
	$(LD) -o $@ $(CORE_MEMORY_PROPERTY_TEST_OBJ) $(MEMORY_OBJ)

$(CORE_BUFFER_ARENA_PROPERTY_TEST): $(CORE_BUFFER_ARENA_PROPERTY_TEST_OBJ) $(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ)
	$(LD) -o $@ $(CORE_BUFFER_ARENA_PROPERTY_TEST_OBJ) $(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ)

$(CORE_CONNECTION_PROPERTY_TEST): $(CORE_CONNECTION_PROPERTY_TEST_OBJ) $(CONNECTION_OBJ)
	$(LD) -o $@ $(CORE_CONNECTION_PROPERTY_TEST_OBJ) $(CONNECTION_OBJ)

$(CORE_BUFFER_ALIAS_TEST): $(CORE_BUFFER_ALIAS_TEST_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ)
	$(LD) -o $@ $(CORE_BUFFER_ALIAS_TEST_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ)

$(CORE_REQUEST_TARGET_PROPERTY_TEST): $(CORE_REQUEST_TARGET_PROPERTY_TEST_OBJ) $(REQUEST_TARGET_OBJ)
	$(LD) -o $@ $(CORE_REQUEST_TARGET_PROPERTY_TEST_OBJ) $(REQUEST_TARGET_OBJ)

$(CORE_ROUTE_CONTRACT_TEST): $(CORE_ROUTE_CONTRACT_TEST_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_OBJ)
	$(LD) -o $@ $(CORE_ROUTE_CONTRACT_TEST_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_OBJ)

$(CORE_HTTP_RESPONSE_CONTRACT_TEST): $(CORE_HTTP_RESPONSE_CONTRACT_TEST_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ)
	$(LD) -o $@ $(CORE_HTTP_RESPONSE_CONTRACT_TEST_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ)

$(CORE_ROUTE_INDEX_EXPERIMENT_TEST): $(CORE_ROUTE_INDEX_EXPERIMENT_TEST_OBJ) $(ROUTE_INDEX_CANDIDATE_OBJ) $(ROUTER_OBJ) $(BYTES_OBJ)
	$(LD) -o $@ $(CORE_ROUTE_INDEX_EXPERIMENT_TEST_OBJ) $(ROUTE_INDEX_CANDIDATE_OBJ) $(ROUTER_OBJ) $(BYTES_OBJ)

$(CORE_HTTP_FRAMING_PROPERTY_TEST): $(CORE_HTTP_FRAMING_PROPERTY_TEST_OBJ) $(HTTP_PARSER_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ)
	$(LD) -o $@ $(CORE_HTTP_FRAMING_PROPERTY_TEST_OBJ) $(HTTP_PARSER_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ)

$(CORE_ACCEPT_TRANSACTION_TEST): $(CORE_ACCEPT_TRANSACTION_TEST_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) $(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)
	$(LD) -o $@ $(CORE_ACCEPT_TRANSACTION_TEST_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) $(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)

$(CORE_PIPELINE_BUDGET_TEST): $(CORE_PIPELINE_BUDGET_TEST_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) $(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)
	$(LD) -o $@ $(CORE_PIPELINE_BUDGET_TEST_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) $(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)

$(CORE_EVENT_BATCH_TEST): $(CORE_EVENT_BATCH_TEST_OBJ) $(EVENT_OBJ)
	$(LD) -o $@ $(CORE_EVENT_BATCH_TEST_OBJ) $(EVENT_OBJ)

$(CORE_SECURITY_PROPERTY_TEST): $(CORE_SECURITY_PROPERTY_TEST_OBJ) $(SECURITY_OBJ)
	$(LD) -o $@ $(CORE_SECURITY_PROPERTY_TEST_OBJ) $(SECURITY_OBJ)

$(STATIC_LIB): $(ARBORCORE_LIBRARY_OBJECTS)
	$(AR) rcsD $@ $(ARBORCORE_LIBRARY_OBJECTS)

$(SHARED_LIB_FULL): $(ARBORCORE_LIBRARY_OBJECTS) $(ABI_VERSION_SCRIPT)
	$(LD) -shared -Bsymbolic-functions -z defs -z text -z relro -z now -z noexecstack \
		-soname=$(SHARED_LIB_SONAME_BASENAME) --version-script=$(ABI_VERSION_SCRIPT) \
		-o $@ $(ARBORCORE_LIBRARY_OBJECTS)

$(SHARED_LIB): $(SHARED_LIB_FULL)
	ln -sfn $(SHARED_LIB_FULL_BASENAME) $@

$(SHARED_LIB_LINK): $(SHARED_LIB)
	ln -sfn $(SHARED_LIB_SONAME_BASENAME) $@

$(ABI_STATIC_CONSUMER): $(ABI_CONSUMER_TEST_OBJ) $(STATIC_LIB)
	$(LD) -o $@ $(ABI_CONSUMER_TEST_OBJ) $(STATIC_LIB)

$(ABI_SHARED_CONSUMER): $(ABI_CONSUMER_TEST_OBJ) $(SHARED_LIB_LINK)
	@set -eu; \
	interp="$$(readelf -lW /bin/sh | sed -n 's/.*Requesting program interpreter: \([^]]*\)\].*/\1/p' | head -n1)"; \
	test -n "$$interp"; \
	$(LD) -o $@ $(ABI_CONSUMER_TEST_OBJ) -L$(BUILD_DIR) -larborcore \
		-rpath '$$ORIGIN' -dynamic-linker "$$interp"

$(CORE_WRITEV_EXPERIMENT_TEST): $(CORE_WRITEV_EXPERIMENT_TEST_OBJ) $(RESPONSE_IOVEC_CANDIDATE_OBJ) $(IOVEC_WRITE_CANDIDATE_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ)
	$(LD) -o $@ $(CORE_WRITEV_EXPERIMENT_TEST_OBJ) $(RESPONSE_IOVEC_CANDIDATE_OBJ) $(IOVEC_WRITE_CANDIDATE_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ)

$(ENCODING_TEST): \
	$(ENCODING_TEST_OBJ) \
	$(HEX_CODEC_OBJ) \
	$(PERCENT_CODEC_OBJ) \
	$(BASE64_OBJ)
	$(LD) -o $@ \
		$(ENCODING_TEST_OBJ) \
		$(HEX_CODEC_OBJ) \
		$(PERCENT_CODEC_OBJ) \
		$(BASE64_OBJ)

$(BUFFER_TEST): $(BUFFER_TEST_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ)
	$(LD) -o $@ $(BUFFER_TEST_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ)

$(ARENA_TEST): $(ARENA_TEST_OBJ) $(ARENA_OBJ)
	$(LD) -o $@ $(ARENA_TEST_OBJ) $(ARENA_OBJ)

$(POLISH_GATE1_TEST): \
	$(POLISH_GATE1_TEST_OBJ) \
	$(BUFFER_OBJ) \
	$(ARENA_OBJ) \
	$(MEMORY_OBJ) \
	$(U64_FORMAT_OBJ) \
	$(HEX_CODEC_OBJ) \
	$(BYTES_OBJ)
	$(LD) -o $@ \
		$(POLISH_GATE1_TEST_OBJ) \
		$(BUFFER_OBJ) \
		$(ARENA_OBJ) \
		$(MEMORY_OBJ) \
		$(U64_FORMAT_OBJ) \
		$(HEX_CODEC_OBJ) \
		$(BYTES_OBJ)

$(POLISH_GATE2_TEST): \
	$(POLISH_GATE2_TEST_OBJ) \
	$(BUFFER_OBJ) \
	$(ARENA_OBJ) \
	$(MEMORY_OBJ) \
	$(HTTP_PARSER_OBJ) \
	$(ROUTER_OBJ) \
	$(BYTES_SCAN_OBJ) \
	$(BYTES_OBJ) \
	$(PARSE_U64_OBJ)
	$(LD) -o $@ \
		$(POLISH_GATE2_TEST_OBJ) \
		$(BUFFER_OBJ) \
		$(ARENA_OBJ) \
		$(MEMORY_OBJ) \
		$(HTTP_PARSER_OBJ) \
		$(ROUTER_OBJ) \
		$(BYTES_SCAN_OBJ) \
		$(BYTES_OBJ) \
		$(PARSE_U64_OBJ)

$(IO_TEST): $(IO_TEST_OBJ) $(IO_OBJ)
	$(LD) -o $@ $(IO_TEST_OBJ) $(IO_OBJ)

$(NET_TEST): $(NET_TEST_OBJ) $(NET_OBJ) $(IO_OBJ)
	$(LD) -o $@ $(NET_TEST_OBJ) $(NET_OBJ) $(IO_OBJ)

$(HTTP_TEST): \
	$(HTTP_TEST_OBJ) \
	$(HTTP_PARSER_OBJ) \
	$(BYTES_SCAN_OBJ) \
	$(BYTES_OBJ) \
	$(PARSE_U64_OBJ)
	$(LD) -o $@ \
		$(HTTP_TEST_OBJ) \
		$(HTTP_PARSER_OBJ) \
		$(BYTES_SCAN_OBJ) \
		$(BYTES_OBJ) \
		$(PARSE_U64_OBJ)

$(ROUTER_TEST): \
	$(ROUTER_TEST_OBJ) \
	$(ROUTER_OBJ) \
	$(HTTP_PARSER_OBJ) \
	$(BYTES_SCAN_OBJ) \
	$(BYTES_OBJ) \
	$(PARSE_U64_OBJ)
	$(LD) -o $@ \
		$(ROUTER_TEST_OBJ) \
		$(ROUTER_OBJ) \
		$(HTTP_PARSER_OBJ) \
		$(BYTES_SCAN_OBJ) \
		$(BYTES_OBJ) \
		$(PARSE_U64_OBJ)

$(EVENT_TEST): $(EVENT_TEST_OBJ) $(EVENT_OBJ)
	$(LD) -o $@ $(EVENT_TEST_OBJ) $(EVENT_OBJ)

$(CONNECTION_TEST): $(CONNECTION_TEST_OBJ) $(CONNECTION_OBJ)
	$(LD) -o $@ $(CONNECTION_TEST_OBJ) $(CONNECTION_OBJ)

$(HTTP_RESPONSE_TEST): \
	$(HTTP_RESPONSE_TEST_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ)
	$(LD) -o $@ \
		$(HTTP_RESPONSE_TEST_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ)

$(REQUEST_TARGET_TEST): $(REQUEST_TARGET_TEST_OBJ) $(REQUEST_TARGET_OBJ)
	$(LD) -o $@ $(REQUEST_TARGET_TEST_OBJ) $(REQUEST_TARGET_OBJ)

$(ROUTE_PATTERN_TEST): \
	$(ROUTE_PATTERN_TEST_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_OBJ)
	$(LD) -o $@ \
		$(ROUTE_PATTERN_TEST_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_OBJ)

$(SERVER_TEST): \
	$(SERVER_TEST_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) \
	$(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) \
	$(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)
	$(LD) -o $@ \
		$(SERVER_TEST_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) \
		$(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) \
		$(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)

$(POLISH_GATE3_TEST): \
	$(POLISH_GATE3_TEST_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) \
	$(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) \
	$(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)
	$(LD) -o $@ \
		$(POLISH_GATE3_TEST_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) \
		$(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) \
		$(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ)

# Server benchmark executables
$(PARSER_BENCH): \
	$(PARSER_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(HTTP_PARSER_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ \
		$(PARSER_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(HTTP_PARSER_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)

$(ROUTING_BENCH): \
	$(ROUTING_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ \
		$(ROUTING_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)

$(ROUTE_INDEX_BENCH): $(ROUTE_INDEX_BENCH_OBJ) $(ROUTE_INDEX_CANDIDATE_OBJ) $(BENCH_SUPPORT_OBJ) $(ROUTER_OBJ) $(BYTES_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ $(ROUTE_INDEX_BENCH_OBJ) $(ROUTE_INDEX_CANDIDATE_OBJ) $(BENCH_SUPPORT_OBJ) $(ROUTER_OBJ) $(BYTES_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)

$(RESPONSE_BENCH): \
	$(RESPONSE_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ \
		$(RESPONSE_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)

$(LIFECYCLE_BENCH): \
	$(LIFECYCLE_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(HTTP_PARSER_OBJ) $(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) \
	$(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ \
		$(LIFECYCLE_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(HTTP_PARSER_OBJ) $(ROUTER_OBJ) $(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) \
		$(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)

$(CONNECTION_BENCH): \
	$(CONNECTION_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(NET_OBJ) $(IO_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ \
		$(CONNECTION_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(NET_OBJ) $(IO_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)

$(LOOPBACK_BENCH): \
	$(LOOPBACK_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) \
	$(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) $(ROUTER_OBJ) \
	$(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ \
		$(LOOPBACK_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(SERVER_OBJ) $(EVENT_OBJ) $(CONNECTION_OBJ) $(HTTP_RESPONSE_OBJ) \
		$(BUFFER_OBJ) $(ARENA_OBJ) $(MEMORY_OBJ) $(IO_OBJ) $(NET_OBJ) $(HTTP_PARSER_OBJ) $(ROUTER_OBJ) \
		$(ROUTE_PATTERN_OBJ) $(REQUEST_TARGET_OBJ) $(BYTES_SCAN_OBJ) $(BYTES_OBJ) $(PARSE_U64_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)

$(RESPONSE_IOVEC_BENCH): $(RESPONSE_IOVEC_BENCH_OBJ) $(RESPONSE_IOVEC_CANDIDATE_OBJ) $(IOVEC_WRITE_CANDIDATE_OBJ) $(BENCH_SUPPORT_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ $(RESPONSE_IOVEC_BENCH_OBJ) $(RESPONSE_IOVEC_CANDIDATE_OBJ) $(IOVEC_WRITE_CANDIDATE_OBJ) $(BENCH_SUPPORT_OBJ) $(HTTP_RESPONSE_OBJ) $(BUFFER_OBJ) $(MEMORY_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)

# Individual test targets
write-test: $(WRITE_TEST)
memory-threshold-test: $(MEMORY_THRESHOLD_TEST)
memory-test: $(MEMORY_TEST)
bytes-test: $(BYTES_TEST)
bytes-phase2-test: $(BYTES_PHASE2_TEST)
numeric-test: $(NUMERIC_TEST)
core-integer-test: $(CORE_INTEGER_TEST)
core-range-test: $(CORE_RANGE_TEST)
core-sequence-property-test: $(CORE_SEQUENCE_PROPERTY_TEST)
core-numeric-property-test: $(CORE_NUMERIC_PROPERTY_TEST)
core-codec-property-test: $(CORE_CODEC_PROPERTY_TEST)
core-retrofit-b1: $(CORE_SEQUENCE_PROPERTY_TEST) $(CORE_NUMERIC_PROPERTY_TEST) $(CORE_CODEC_PROPERTY_TEST)
	@$(CORE_SEQUENCE_PROPERTY_TEST)
	@$(CORE_NUMERIC_PROPERTY_TEST)
	@$(CORE_CODEC_PROPERTY_TEST)
	@echo "PASS: Core Retrofit B1 qualification"
core-memory-property-test: $(CORE_MEMORY_PROPERTY_TEST)
core-buffer-arena-property-test: $(CORE_BUFFER_ARENA_PROPERTY_TEST)
core-connection-property-test: $(CORE_CONNECTION_PROPERTY_TEST)
core-buffer-alias-test: $(CORE_BUFFER_ALIAS_TEST)
core-retrofit-c1: $(CORE_MEMORY_PROPERTY_TEST) $(CORE_BUFFER_ARENA_PROPERTY_TEST) $(CORE_CONNECTION_PROPERTY_TEST)
	@$(CORE_MEMORY_PROPERTY_TEST)
	@$(CORE_BUFFER_ARENA_PROPERTY_TEST)
	@$(CORE_CONNECTION_PROPERTY_TEST)
	@echo "PASS: Core Retrofit C1 qualification"
core-retrofit-c: $(CORE_MEMORY_PROPERTY_TEST) $(CORE_BUFFER_ARENA_PROPERTY_TEST) $(CORE_CONNECTION_PROPERTY_TEST) $(CORE_BUFFER_ALIAS_TEST) $(BUFFER_TEST) $(ARENA_TEST) $(CONNECTION_TEST) $(CONNECTION_OBJ)
	@$(CORE_MEMORY_PROPERTY_TEST)
	@$(CORE_BUFFER_ARENA_PROPERTY_TEST)
	@$(CORE_CONNECTION_PROPERTY_TEST)
	@$(CORE_BUFFER_ALIAS_TEST)
	@$(BUFFER_TEST)
	@$(ARENA_TEST)
	@$(CONNECTION_TEST)
	@if nm -g --defined-only $(CONNECTION_OBJ) | awk '{print $$3}' | grep -qx connection_reset_io; then \
		echo "FAIL: connection_reset_io remains in the public ABI"; exit 1; \
	fi
	@echo "PASS: connection_reset_io removed from the public ABI"
	@echo "PASS: Core Retrofit C qualification"
core-request-target-property-test: $(CORE_REQUEST_TARGET_PROPERTY_TEST)
core-route-contract-test: $(CORE_ROUTE_CONTRACT_TEST)
core-http-response-contract-test: $(CORE_HTTP_RESPONSE_CONTRACT_TEST)
core-route-index-experiment-test: $(CORE_ROUTE_INDEX_EXPERIMENT_TEST)
core-retrofit-d: $(CORE_REQUEST_TARGET_PROPERTY_TEST) $(CORE_ROUTE_CONTRACT_TEST) $(CORE_HTTP_RESPONSE_CONTRACT_TEST) $(CORE_ROUTE_INDEX_EXPERIMENT_TEST)
	@$(CORE_REQUEST_TARGET_PROPERTY_TEST)
	@$(CORE_ROUTE_CONTRACT_TEST)
	@$(CORE_HTTP_RESPONSE_CONTRACT_TEST)
	@$(CORE_ROUTE_INDEX_EXPERIMENT_TEST)
	@echo "PASS: Core Retrofit D correctness qualification"
core-http-framing-property-test: $(CORE_HTTP_FRAMING_PROPERTY_TEST)
core-accept-transaction-test: $(CORE_ACCEPT_TRANSACTION_TEST)
core-pipeline-budget-test: $(CORE_PIPELINE_BUDGET_TEST)
core-event-batch-test: $(CORE_EVENT_BATCH_TEST)
core-writev-experiment-test: $(CORE_WRITEV_EXPERIMENT_TEST)
core-security-property-test: $(CORE_SECURITY_PROPERTY_TEST)
	@$(CORE_SECURITY_PROPERTY_TEST)

libarborcore-libraries: $(STATIC_LIB) $(SHARED_LIB_FULL) $(SHARED_LIB) $(SHARED_LIB_LINK)

libarborcore-static: $(STATIC_LIB) $(ABI_STATIC_CONSUMER)

libarborcore-shared: $(SHARED_LIB_FULL) $(SHARED_LIB) $(SHARED_LIB_LINK) $(ABI_SHARED_CONSUMER)

abi-surface-verify: $(ARBORCORE_OBJECTS) $(SECURITY_OBJ) $(ABI_SURFACE_VERIFY) $(ABI_PUBLIC_SYMBOLS) $(ABI_INTERNAL_SYMBOLS)
	@bash $(ABI_SURFACE_VERIFY)

abi-dependency-verify: $(ARBORCORE_OBJECTS) $(ABI_DEPENDENCY_VERIFY) $(ABI_PUBLIC_SYMBOLS) $(ABI_INTERNAL_SYMBOLS)
	@bash $(ABI_DEPENDENCY_VERIFY)

abi-layout-verify: $(ABI_LAYOUT_VERIFY) $(ABI_LAYOUT)
	@bash $(ABI_LAYOUT_VERIFY)

security-shape-verify: $(SECURITY_OBJ) $(SECURITY_SHAPE_VERIFY)
	@bash $(SECURITY_SHAPE_VERIFY)

library-readiness: $(STATIC_LIB) $(SHARED_LIB_FULL) $(SHARED_LIB) $(SHARED_LIB_LINK) $(ABI_STATIC_CONSUMER) $(ABI_SHARED_CONSUMER) $(LIBRARY_READINESS_VERIFY)
	@bash $(LIBRARY_READINESS_VERIFY)

library-reproducibility-verify: $(LIBRARY_REPRODUCIBILITY_VERIFY) $(ABI_FREEZE_MANIFEST)
	@bash $(LIBRARY_REPRODUCIBILITY_VERIFY)

library-install-verify: libarborcore-libraries $(ABI_CONSUMER_TEST_OBJ) $(LIBRARY_INSTALL_VERIFY) $(ABI_FREEZE_MANIFEST)
	@bash $(LIBRARY_INSTALL_VERIFY)

library-release-gate: $(LIBRARY_RELEASE_GATE) $(ABI_FREEZE_MANIFEST)
	@bash $(LIBRARY_RELEASE_GATE)
core-retrofit-e: $(CORE_HTTP_FRAMING_PROPERTY_TEST) $(CORE_ACCEPT_TRANSACTION_TEST) $(CORE_PIPELINE_BUDGET_TEST) $(CORE_EVENT_BATCH_TEST) $(EVENT_TEST) $(NET_TEST) $(SERVER_TEST) $(POLISH_GATE3_TEST)
	@$(CORE_HTTP_FRAMING_PROPERTY_TEST)
	@$(CORE_ACCEPT_TRANSACTION_TEST)
	@$(CORE_PIPELINE_BUDGET_TEST)
	@$(CORE_EVENT_BATCH_TEST)
	@$(EVENT_TEST)
	@$(NET_TEST)
	@$(SERVER_TEST)
	@$(POLISH_GATE3_TEST)
	@echo "PASS: Core Retrofit E runtime correctness qualification"
encoding-test: $(ENCODING_TEST)
buffer-test: $(BUFFER_TEST)
arena-test: $(ARENA_TEST)
polish-gate-1: $(BUFFER_TEST) $(ARENA_TEST) $(POLISH_GATE1_TEST) $(POLISH_GATE1)
	@bash $(POLISH_GATE1)
polish-gate-2: $(IO_TEST) $(NET_TEST) $(HTTP_TEST) $(ROUTER_TEST) $(POLISH_GATE2_TEST) $(POLISH_GATE2)
	@bash $(POLISH_GATE2)
io-test: $(IO_TEST)
net-test: $(NET_TEST)
http-test: $(HTTP_TEST)
router-test: $(ROUTER_TEST)
event-test: $(EVENT_TEST)
connection-test: $(CONNECTION_TEST)
http-response-test: $(HTTP_RESPONSE_TEST)
request-target-test: $(REQUEST_TARGET_TEST)
route-pattern-test: $(ROUTE_PATTERN_TEST)
server-test: $(SERVER_TEST)
polish-gate-3: $(EVENT_TEST) $(CONNECTION_TEST) $(HTTP_RESPONSE_TEST) $(REQUEST_TARGET_TEST) $(ROUTE_PATTERN_TEST) $(SERVER_TEST) $(POLISH_GATE3_TEST) $(POLISH_GATE3)
	@bash $(POLISH_GATE3)

# Complete verification
check: \
	$(ARBORCORE) \
	$(WRITE_TEST) \
	$(MEMORY_THRESHOLD_TEST) \
	$(MEMORY_TEST) \
	$(BYTES_TEST) \
	$(BYTES_PHASE2_TEST) \
	$(NUMERIC_TEST) \
	$(CORE_INTEGER_TEST) \
	$(CORE_RANGE_TEST) \
	$(CORE_SEQUENCE_PROPERTY_TEST) \
	$(CORE_NUMERIC_PROPERTY_TEST) \
	$(CORE_CODEC_PROPERTY_TEST) \
	$(CORE_MEMORY_PROPERTY_TEST) \
	$(CORE_BUFFER_ARENA_PROPERTY_TEST) \
	$(CORE_CONNECTION_PROPERTY_TEST) \
	$(CORE_BUFFER_ALIAS_TEST) \
	$(CORE_REQUEST_TARGET_PROPERTY_TEST) \
	$(CORE_ROUTE_CONTRACT_TEST) \
	$(CORE_HTTP_RESPONSE_CONTRACT_TEST) \
	$(CORE_ROUTE_INDEX_EXPERIMENT_TEST) \
	$(CORE_HTTP_FRAMING_PROPERTY_TEST) \
	$(CORE_ACCEPT_TRANSACTION_TEST) \
	$(CORE_PIPELINE_BUDGET_TEST) \
	$(CORE_EVENT_BATCH_TEST) \
	$(CORE_WRITEV_EXPERIMENT_TEST_OBJ) \
	$(CORE_SECURITY_PROPERTY_TEST) \
	$(ABI_CONSUMER_TEST_OBJ) \
	$(ENCODING_TEST) \
	$(BUFFER_TEST) \
	$(ARENA_TEST) \
	$(POLISH_GATE1_TEST) \
	$(POLISH_GATE2_TEST) \
	$(IO_TEST) \
	$(NET_TEST) \
	$(HTTP_TEST) \
	$(ROUTER_TEST) \
	$(EVENT_TEST) \
	$(CONNECTION_TEST) \
	$(HTTP_RESPONSE_TEST) \
	$(REQUEST_TARGET_TEST) \
	$(ROUTE_PATTERN_TEST) \
	$(SERVER_TEST) \
	$(POLISH_GATE3_TEST)
	@set -eu; \
	tmp="$$(mktemp "$(BUILD_DIR)/arborcore-check.XXXXXX")"; \
	trap 'rm -f "$$tmp"' EXIT; \
	echo "### Arborcore output"; \
	$(ARBORCORE) > "$$tmp"; \
	printf 'Arborcore: Assembly core active\n' | cmp -s - "$$tmp"; \
	echo "PASS: startup output"; \
	echo; \
	echo "### Arborcore startup failure"; \
	status=0; \
	$(ARBORCORE) 1>&- || status=$$?; \
	if [ "$$status" -ne 1 ]; then \
		echo "FAIL: expected exit 1, got $$status"; \
		exit 1; \
	fi; \
	echo "PASS: closed stdout -> exit 1"; \
	echo; \
	echo "### write_all"; \
	status=0; \
	$(WRITE_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: write-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: write-test"; \
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
	echo; \
	echo "### memory"; \
	status=0; \
	$(MEMORY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: memory-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: memory-test"; \
	echo; \
	echo "### bytes"; \
	status=0; \
	$(BYTES_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: bytes-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: bytes-test"; \
	echo; \
	echo "### bytes phase 2"; \
	status=0; \
	$(BYTES_PHASE2_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: bytes-phase2-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: bytes-phase2-test"; \
	echo; \
	echo "### numeric"; \
	status=0; \
	$(NUMERIC_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: numeric-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: numeric-test"; \
	echo; \
	echo "### Core Retrofit A: integer geometry"; \
	status=0; \
	$(CORE_INTEGER_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: core-integer-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: core-integer-test"; \
	echo; \
	echo "### Core Retrofit A: range algebra"; \
	status=0; \
	$(CORE_RANGE_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: core-range-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: core-range-test"; \
	echo; \
	echo "### Core Retrofit B1: sequence properties"; \
	status=0; \
	$(CORE_SEQUENCE_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: core-sequence-property-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: core-sequence-property-test"; \
	echo; \
	echo "### Core Retrofit B1: numeric round-trip properties"; \
	status=0; \
	$(CORE_NUMERIC_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: core-numeric-property-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: core-numeric-property-test"; \
	echo; \
	echo "### Core Retrofit B1: codec properties"; \
	status=0; \
	$(CORE_CODEC_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: core-codec-property-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: core-codec-property-test"; \
	echo; \
	echo "### Core Retrofit C1: memory geometry"; \
	status=0; \
	$(CORE_MEMORY_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-memory-property-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-memory-property-test"; \
	echo; \
	echo "### Core Retrofit C1: buffer/arena algebra"; \
	status=0; \
	$(CORE_BUFFER_ARENA_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-buffer-arena-property-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-buffer-arena-property-test"; \
	echo; \
	echo "### Core Retrofit C1: connection state relation"; \
	status=0; \
	$(CORE_CONNECTION_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-connection-property-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-connection-property-test"; \
	echo; \
	echo "### Core Retrofit C2: buffer alias snapshot semantics"; \
	status=0; \
	$(CORE_BUFFER_ALIAS_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-buffer-alias-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-buffer-alias-test"; \
	echo; \
	echo "### Core Retrofit C2: connection public-state hardening"; \
	if nm -g --defined-only $(CONNECTION_OBJ) | awk '{print $$3}' | grep -qx connection_reset_io; then \
		echo "FAIL: connection_reset_io remains in the public ABI"; exit 1; \
	fi; \
	echo "PASS: connection_reset_io removed from the public ABI"; \
	echo; \
	echo "### Core Retrofit D1: request-target full-domain validation"; \
	status=0; $(CORE_REQUEST_TARGET_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-request-target-property-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-request-target-property-test"; \
	echo; \
	echo "### Core Retrofit D2: routing contracts and lifetimes"; \
	status=0; $(CORE_ROUTE_CONTRACT_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-route-contract-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-route-contract-test"; \
	echo; \
	echo "### Core Retrofit D3: response alias/failure contracts"; \
	status=0; $(CORE_HTTP_RESPONSE_CONTRACT_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-http-response-contract-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-http-response-contract-test"; \
	echo; \
	echo "### Core Retrofit D4: prepared-index equivalence"; \
	status=0; $(CORE_ROUTE_INDEX_EXPERIMENT_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-route-index-experiment-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-route-index-experiment-test"; \
	echo; \
	echo "### Core Retrofit E3: incremental framing"; \
	status=0; $(CORE_HTTP_FRAMING_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-http-framing-property-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-http-framing-property-test"; \
	echo; \
	echo "### Core Retrofit E1/E5: accept transaction and pristine reuse"; \
	status=0; $(CORE_ACCEPT_TRANSACTION_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-accept-transaction-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-accept-transaction-test"; \
	echo; \
	echo "### Core Retrofit E4/E7/E9: pipeline drain, immediate write, budget"; \
	status=0; $(CORE_PIPELINE_BUDGET_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-pipeline-budget-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-pipeline-budget-test"; \
	echo; \
	echo "### Core Retrofit E9: epoll event batching"; \
	status=0; $(CORE_EVENT_BATCH_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-event-batch-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-event-batch-test"; \
	echo; \
	echo "### Assembly Security S1/S2"; \
	status=0; $(CORE_SECURITY_PROPERTY_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: core-security-property-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: core-security-property-test"; \
	echo; \
	echo "### encoding"; \
	status=0; \
	$(ENCODING_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then \
		echo "FAIL: encoding-test exit=$$status"; \
		exit "$$status"; \
	fi; \
	echo "PASS: encoding-test"; \
	echo; \
	echo "### POLISH GATE #1"; \
	bash $(POLISH_GATE1); \
	echo; \
	echo "### linux io"; \
	status=0; \
	$(IO_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: io-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: io-test"; \
	echo; \
	echo "### network"; \
	status=0; \
	$(NET_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: net-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: net-test"; \
	echo; \
	echo "### http/1.1 parser"; \
	status=0; \
	$(HTTP_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: http-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: http-test"; \
	echo; \
	echo "### router / request dispatch"; \
	status=0; \
	$(ROUTER_TEST) || status=$$?; \
	if [ "$$status" -ne 0 ]; then echo "FAIL: router-test exit=$$status"; exit "$$status"; fi; \
	echo "PASS: router-test"; \
	echo; \
	echo "### POLISH GATE #2"; \
	bash $(POLISH_GATE2); \
	echo; \
	echo "### POLISH GATE #3"; \
	bash $(POLISH_GATE3); \
	echo; \
	echo "### GNU-stack notes"; \
	for obj in $(ARBORCORE_OBJECTS) $(TEST_OBJECTS); do \
		if ! $(READELF) -SW "$$obj" | grep -q '\.note.GNU-stack'; then \
			echo "FAIL: $$obj has no .note.GNU-stack"; \
			exit 1; \
		fi; \
		echo "PASS: $$obj"; \
	done; \
	echo; \
	echo "### Final stack permissions"; \
	if ! $(READELF) -lW $(ARBORCORE) | grep 'GNU_STACK' | grep -q ' RW '; then \
		echo "FAIL: expected writable, non-executable GNU_STACK"; \
		exit 1; \
	fi; \
	if $(READELF) -lW $(ARBORCORE) | grep 'GNU_STACK' | grep -q 'RWE'; then \
		echo "FAIL: executable stack"; \
		exit 1; \
	fi; \
	echo "PASS: GNU_STACK is non-executable"; \
	echo; \
	echo "ALL ARBORCORE TESTS PASSED"

# Server performance baseline
SERVER_BENCH_EXECUTABLES := \
	$(PARSER_BENCH) \
	$(ROUTING_BENCH) \
	$(RESPONSE_BENCH) \
	$(LIFECYCLE_BENCH) \
	$(CONNECTION_BENCH) \
	$(LOOPBACK_BENCH)

$(CODEC_BENCH): $(CODEC_BENCH_OBJ) $(BENCH_SUPPORT_OBJ) $(PERCENT_CODEC_OBJ) $(U64_FORMAT_OBJ) $(WRITE_OBJ)
	$(LD) -o $@ $^

benchmark-server: $(SERVER_BENCH_EXECUTABLES) $(SERVER_BENCHMARK_RUNNER)
	@bash $(SERVER_BENCHMARK_RUNNER)

qualify-server-baseline: $(SERVER_BENCH_EXECUTABLES) $(SERVER_BASELINE_QUALIFIER) | $(GENERATED_DIR)
	@bash $(SERVER_BASELINE_QUALIFIER)

accept-server-baseline: $(SERVER_BASELINE_ACCEPTOR) | $(GENERATED_DIR)
	@bash $(SERVER_BASELINE_ACCEPTOR)

show-server-baseline:
	@echo "Performance profile: $(ARBORCORE_PERF_PROFILE)"
	@if [[ -r $(SERVER_PERF_BASELINE) ]]; then cat $(SERVER_PERF_BASELINE); else echo "No accepted/proposed profile at $(SERVER_PERF_BASELINE)"; fi

list-server-profiles:
	@if [[ -d $(GENERATED_DIR)/performance ]]; then find $(GENERATED_DIR)/performance -maxdepth 1 -type f -name '*.env' -printf '%f\n' | sed 's/\.env$$//' | sort; else echo "No local performance profiles"; fi

verify-server-performance: $(ARBORCORE_OBJECTS) $(SERVER_BENCH_EXECUTABLES) $(SERVER_PERFORMANCE_VERIFY)
	@bash $(SERVER_PERFORMANCE_VERIFY)

# Pre-commit construction qualification. The runner records the exact dirty
# production source hash and tree state, but the accepted profile remains
# read-only and machine-specific.
verify-server-performance-candidate: $(ARBORCORE_OBJECTS) $(SERVER_BENCH_EXECUTABLES) $(SERVER_PERFORMANCE_VERIFY)
	@ARBORCORE_BENCH_ALLOW_DIRTY_PRODUCTION=1 bash $(SERVER_PERFORMANCE_VERIFY)

benchmark-server-perf: $(LIFECYCLE_BENCH) $(LOOPBACK_BENCH) $(SERVER_BENCHMARK_PERF)
	@bash $(SERVER_BENCHMARK_PERF)

benchmark-server-syscalls: $(LOOPBACK_BENCH) $(SERVER_BENCHMARK_SYSCALLS)
	@bash $(SERVER_BENCHMARK_SYSCALLS)

# Retrofit D routing experiment and environment-qualified performance.
route-index-experiment: $(CORE_ROUTE_INDEX_EXPERIMENT_TEST) $(ROUTE_INDEX_BENCH) $(ROUTE_INDEX_EXPERIMENT_VERIFY)
	@bash $(ROUTE_INDEX_EXPERIMENT_VERIFY)

verify-server-environment: $(ARBORCORE_OBJECTS) $(SERVER_BENCH_EXECUTABLES) $(SERVER_ENVIRONMENT_ADMISSIBILITY)
	@ARBORCORE_QUALIFICATION_REFERENCE_COMMIT=$(CORE_RETROFIT_D_REFERENCE) bash $(SERVER_ENVIRONMENT_ADMISSIBILITY); rc=$$?; \
	if [[ $$rc -eq 10 ]]; then exit 0; else exit $$rc; fi

verify-server-performance-qualified-candidate: $(ARBORCORE_OBJECTS) $(SERVER_BENCH_EXECUTABLES) $(SERVER_PERFORMANCE_QUALIFIED)
	@ARBORCORE_QUALIFICATION_REFERENCE_COMMIT=$(CORE_RETROFIT_D_REFERENCE) ARBORCORE_PERF_PROFILE=$(ARBORCORE_PERF_PROFILE) bash $(SERVER_PERFORMANCE_QUALIFIED) candidate

core-retrofit-d-gate: $(CORE_RETROFIT_D_GATE)
	@ARBORCORE_QUALIFICATION_REFERENCE_COMMIT=$(CORE_RETROFIT_D_REFERENCE) ARBORCORE_PERF_PROFILE=$(ARBORCORE_PERF_PROFILE) bash $(CORE_RETROFIT_D_GATE)

# Retrofit E experiments and final runtime gate.
writev-experiment: $(CORE_WRITEV_EXPERIMENT_TEST) $(RESPONSE_IOVEC_BENCH) $(WRITEV_EXPERIMENT_VERIFY)
	@bash $(WRITEV_EXPERIMENT_VERIFY)

runtime-syscall-experiment: $(LOOPBACK_BENCH) $(RUNTIME_SYSCALL_VERIFY)
	@bash $(RUNTIME_SYSCALL_VERIFY)

core-retrofit-e-gate: $(CORE_RETROFIT_E_GATE)
	@ARBORCORE_QUALIFICATION_REFERENCE_COMMIT=$(CORE_RETROFIT_E_REFERENCE) ARBORCORE_PERF_PROFILE=$(ARBORCORE_PERF_PROFILE) bash $(CORE_RETROFIT_E_GATE)

assembly-security-abi-gate: $(ASSEMBLY_SECURITY_ABI_GATE)
	@ARBORCORE_QUALIFICATION_REFERENCE_COMMIT=$(ASSEMBLY_SECURITY_ABI_REFERENCE) ARBORCORE_PERF_PROFILE=$(ARBORCORE_PERF_PROFILE) bash $(ASSEMBLY_SECURITY_ABI_GATE)

# Dedicated percent-codec reference/candidate performance evidence.
benchmark-codec: $(CODEC_BENCH) $(CODEC_BENCHMARK_RUNNER)
	@bash $(CODEC_BENCHMARK_RUNNER)

qualify-codec-baseline: $(ARBORCORE_OBJECTS) $(CODEC_BENCH) $(CODEC_BASELINE_QUALIFIER) | $(GENERATED_DIR)
	@bash $(CODEC_BASELINE_QUALIFIER)

show-codec-baseline:
	@echo "Codec performance profile: $(ARBORCORE_PERF_PROFILE)"
	@if [[ -r $(CODEC_PERF_BASELINE) ]]; then cat $(CODEC_PERF_BASELINE); else echo "No codec profile at $(CODEC_PERF_BASELINE)"; fi

verify-codec-performance: $(ARBORCORE_OBJECTS) $(CODEC_BENCH) $(CODEC_PERFORMANCE_VERIFY)
	@bash $(CODEC_PERFORMANCE_VERIFY)

verify-codec-performance-candidate: $(ARBORCORE_OBJECTS) $(CODEC_BENCH) $(CODEC_PERFORMANCE_VERIFY)
	@ARBORCORE_CODEC_ALLOW_DIRTY_PRODUCTION=1 bash $(CODEC_PERFORMANCE_VERIFY)

# Memory benchmark
benchmark: $(MEMORY_OBJ) $(WRITE_OBJ) $(MEMORY_BENCH_ASM) $(MEMORY_BENCH_RUNNER)
	@set -eu; \
	workspace="$(ROOT_DIR)/$(BUILD_DIR)/.memory-benchmark-work"; \
	rm -rf "$$workspace"; \
	mkdir -p "$$workspace"; \
	trap 'rm -rf "$$workspace"' EXIT; \
	ln -s "$(ROOT_DIR)/$(MEMORY_BENCH_ASM)" "$$workspace/memory_bench.asm"; \
	ln -s "$(ROOT_DIR)/$(MEMORY_OBJ)" "$$workspace/memory.o"; \
	ln -s "$(ROOT_DIR)/$(WRITE_OBJ)" "$$workspace/write.o"; \
	cd "$$workspace"; \
	bash "$(ROOT_DIR)/$(MEMORY_BENCH_RUNNER)"

# Machine-specific memory qualification
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
	ln -s "$(ROOT_DIR)/$(MEMORY_BENCH_ASM)" "$$workspace/memory_bench.asm"; \
	ln -s "$(ROOT_DIR)/$(MEMORY_OBJ)" "$$workspace/memory.o"; \
	ln -s "$(ROOT_DIR)/$(WRITE_OBJ)" "$$workspace/write.o"; \
	ln -s "$(ROOT_DIR)/$(GENERATED_DIR)" "$$workspace/generated"; \
	cd "$$workspace"; \
	ARBORCORE_ROOT="$(ROOT_DIR)" ARBORCORE_PERF_PROFILE="$(ARBORCORE_PERF_PROFILE)" bash "$(ROOT_DIR)/$(MEMORY_QUALIFIER)"
	$(MAKE) \
		$(MEMORY_THRESHOLD_OBJ) \
		$(MEMORY_OBJ) \
		$(ARBORCORE) \
		$(MEMORY_THRESHOLD_TEST) \
		$(MEMORY_TEST)
	$(MAKE) check

# Formal Assembly library installation. These targets never install start.o
# and never mutate the frozen ABI metadata. DESTDIR is supported for package
# managers and qualification.
install-libraries: libarborcore-libraries $(ABI_FREEZE_MANIFEST) $(ABI_PUBLIC_SYMBOLS) $(ABI_INTERNAL_SYMBOLS) $(ABI_LAYOUT) $(ABI_VERSION_SCRIPT) $(ABI_README)
	install -d "$(DESTDIR)$(LIBDIR)" "$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)"
	install -m 0644 $(STATIC_LIB) "$(DESTDIR)$(LIBDIR)/$(STATIC_LIB_BASENAME)"
	install -m 0755 $(SHARED_LIB_FULL) "$(DESTDIR)$(LIBDIR)/$(SHARED_LIB_FULL_BASENAME)"
	ln -sfn $(SHARED_LIB_FULL_BASENAME) "$(DESTDIR)$(LIBDIR)/$(SHARED_LIB_SONAME_BASENAME)"
	ln -sfn $(SHARED_LIB_SONAME_BASENAME) "$(DESTDIR)$(LIBDIR)/$(SHARED_LIB_LINK_BASENAME)"
	install -m 0644 $(ABI_README) "$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)/README.md"
	install -m 0644 $(ABI_FREEZE_MANIFEST) $(ABI_PUBLIC_SYMBOLS) $(ABI_INTERNAL_SYMBOLS) $(ABI_LAYOUT) $(ABI_VERSION_SCRIPT) "$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)/"

uninstall-libraries:
	rm -f \
		"$(DESTDIR)$(LIBDIR)/$(STATIC_LIB_BASENAME)" \
		"$(DESTDIR)$(LIBDIR)/$(SHARED_LIB_FULL_BASENAME)" \
		"$(DESTDIR)$(LIBDIR)/$(SHARED_LIB_SONAME_BASENAME)" \
		"$(DESTDIR)$(LIBDIR)/$(SHARED_LIB_LINK_BASENAME)" \
		"$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)/README.md" \
		"$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)/arborcore-1.freeze" \
		"$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)/arborcore-1.symbols" \
		"$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)/arborcore-1.internal-symbols" \
		"$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)/arborcore-1.layout" \
		"$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)/arborcore-1.map"
	-rmdir "$(DESTDIR)$(ARBORCORE_ABI_INSTALL_DIR)" 2>/dev/null || true
	-rmdir "$(DESTDIR)$(DATADIR)/arborcore" 2>/dev/null || true

# Cleanup
clean:
	rm -rf $(BUILD_DIR)

# Full reset including machine-qualified policy
distclean: clean
	rm -f $(MEMORY_POLICY) $(SERVER_PERF_BASELINE) $(CODEC_PERF_BASELINE)

# ============================================================
# C Runtime Bridge CR0-CR8
# ============================================================

CC ?= cc

C_RUNTIME_INCLUDE_DIR := include
C_RUNTIME_SRC_DIR := src/c
C_TEST_DIR := tests/c
C_RUNTIME_BUILD_DIR := $(BUILD_DIR)/c-runtime
C_TEST_BUILD_DIR := $(BUILD_DIR)/c-tests

ARBORCORE_C_CPPFLAGS := -I$(C_RUNTIME_INCLUDE_DIR) -D_POSIX_C_SOURCE=200809L
ARBORCORE_C_CFLAGS := \
	-std=c17 \
	-O2 \
	-fPIC \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Werror \
	-Wconversion \
	-Wsign-conversion \
	-Wshadow \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wformat=2 \
	-Wundef
ARBORCORE_C_LDFLAGS := -no-pie -Wl,-z,relro,-z,now,-z,noexecstack

C_RUNTIME_SOURCES := \
	$(C_RUNTIME_SRC_DIR)/status.c \
	$(C_RUNTIME_SRC_DIR)/security.c \
	$(C_RUNTIME_SRC_DIR)/request.c \
	$(C_RUNTIME_SRC_DIR)/route.c \
	$(C_RUNTIME_SRC_DIR)/runtime.c
C_RUNTIME_OBJECTS := \
	$(C_RUNTIME_BUILD_DIR)/status.o \
	$(C_RUNTIME_BUILD_DIR)/security.o \
	$(C_RUNTIME_BUILD_DIR)/request.o \
	$(C_RUNTIME_BUILD_DIR)/route.o \
	$(C_RUNTIME_BUILD_DIR)/runtime.o
C_RUNTIME_LIB := $(BUILD_DIR)/libarborcore_runtime.a

C_ABI_LAYOUT_TEST_OBJ := $(C_TEST_BUILD_DIR)/abi_layout_test.o
C_RUNTIME_CONSUMER_OBJ := $(C_TEST_BUILD_DIR)/runtime_bridge_consumer.o
C_RUNTIME_ADVERSARIAL_TEST_OBJ := $(C_TEST_BUILD_DIR)/runtime_adversarial_test.o
C_RUNTIME_SERVER_TEST_OBJ := $(C_TEST_BUILD_DIR)/server_bridge_test.o
C_RUNTIME_BENCH_OBJ := $(C_TEST_BUILD_DIR)/c_runtime_bridge_bench.o

C_ABI_LAYOUT_TEST := $(BUILD_DIR)/c-abi-layout-test
C_RUNTIME_STATIC_CONSUMER := $(BUILD_DIR)/c-runtime-static-consumer
C_RUNTIME_SHARED_CONSUMER := $(BUILD_DIR)/c-runtime-shared-consumer
C_RUNTIME_ADVERSARIAL_TEST := $(BUILD_DIR)/c-runtime-adversarial-test
C_RUNTIME_SERVER_TEST := $(BUILD_DIR)/c-runtime-server-test
C_RUNTIME_SANITIZE_TEST := $(BUILD_DIR)/c-runtime-sanitize-test
C_RUNTIME_BENCH := $(BUILD_DIR)/bench-c-runtime-bridge

C_ASSEMBLY_ABI_HEADER_VERIFY := $(TOOLS_DIR)/c_assembly_abi_header_verify.sh
C_RUNTIME_FROZEN_ASSEMBLY_VERIFY := $(TOOLS_DIR)/c_runtime_frozen_assembly_verify.sh
C_RUNTIME_DEPENDENCY_VERIFY := $(TOOLS_DIR)/c_runtime_dependency_verify.sh
C_RUNTIME_REPRODUCIBILITY_VERIFY := $(TOOLS_DIR)/c_runtime_reproducibility_verify.sh
C_RUNTIME_BENCHMARK_VERIFY := $(TOOLS_DIR)/c_runtime_benchmark_verify.sh
C_RUNTIME_BRIDGE_GATE := $(TOOLS_DIR)/c_runtime_bridge_gate.sh

.PHONY: c-runtime-library c-runtime-all c-runtime-check c-runtime-sanitize
.PHONY: c-assembly-abi-header-verify c-runtime-frozen-assembly-verify c-runtime-dependency-verify
.PHONY: c-runtime-reproducibility-verify c-runtime-benchmark-verify c-runtime-bridge-gate

$(C_RUNTIME_BUILD_DIR) $(C_TEST_BUILD_DIR):
	mkdir -p $@

$(C_RUNTIME_BUILD_DIR)/%.o: $(C_RUNTIME_SRC_DIR)/%.c include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(C_RUNTIME_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(C_TEST_BUILD_DIR)/%.o: $(C_TEST_DIR)/%.c include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(C_TEST_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(C_RUNTIME_BENCH_OBJ): $(BENCH_DIR)/c_runtime_bridge_bench.c include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(C_TEST_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(C_RUNTIME_LIB): $(C_RUNTIME_OBJECTS) | $(BUILD_DIR)
	$(AR) rcsD $@ $(C_RUNTIME_OBJECTS)

c-runtime-library: $(C_RUNTIME_LIB)

$(C_ABI_LAYOUT_TEST): $(C_ABI_LAYOUT_TEST_OBJ) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(C_ABI_LAYOUT_TEST_OBJ) $(STATIC_LIB)

$(C_RUNTIME_STATIC_CONSUMER): $(C_RUNTIME_CONSUMER_OBJ) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(C_RUNTIME_CONSUMER_OBJ) $(C_RUNTIME_LIB) $(STATIC_LIB)

$(C_RUNTIME_SHARED_CONSUMER): $(C_RUNTIME_CONSUMER_OBJ) $(C_RUNTIME_LIB) $(SHARED_LIB_LINK)
	$(CC) $(ARBORCORE_C_LDFLAGS) -Wl,-rpath,'$$ORIGIN' -o $@ \
		$(C_RUNTIME_CONSUMER_OBJ) $(C_RUNTIME_LIB) -L$(BUILD_DIR) -larborcore

$(C_RUNTIME_ADVERSARIAL_TEST): $(C_RUNTIME_ADVERSARIAL_TEST_OBJ) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(C_RUNTIME_ADVERSARIAL_TEST_OBJ) $(C_RUNTIME_LIB) $(STATIC_LIB)

$(C_RUNTIME_SERVER_TEST): $(C_RUNTIME_SERVER_TEST_OBJ) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(C_RUNTIME_SERVER_TEST_OBJ) $(C_RUNTIME_LIB) $(STATIC_LIB)

$(C_RUNTIME_SANITIZE_TEST): $(C_TEST_DIR)/runtime_adversarial_test.c $(C_RUNTIME_SOURCES) $(STATIC_LIB) include/arborcore/arborcore.h include/arborcore/assembly_abi.h
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(C_TEST_DIR)/runtime_adversarial_test.c $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(C_RUNTIME_BENCH): $(C_RUNTIME_BENCH_OBJ) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(C_RUNTIME_BENCH_OBJ) $(C_RUNTIME_LIB) $(STATIC_LIB)

c-runtime-all: \
	$(C_RUNTIME_LIB) \
	$(C_ABI_LAYOUT_TEST) \
	$(C_RUNTIME_STATIC_CONSUMER) \
	$(C_RUNTIME_SHARED_CONSUMER) \
	$(C_RUNTIME_ADVERSARIAL_TEST) \
	$(C_RUNTIME_SERVER_TEST)

c-runtime-check: c-runtime-all
	@$(C_ABI_LAYOUT_TEST)
	@echo "PASS: CR0/CR2 native aggregate/layout ABI"
	@$(C_RUNTIME_STATIC_CONSUMER)
	@echo "PASS: CR1/CR3/CR4/CR5 static Assembly consumer"
	@LD_LIBRARY_PATH=$(BUILD_DIR) $(C_RUNTIME_SHARED_CONSUMER)
	@echo "PASS: CR1 shared Assembly equivalence consumer"
	@$(C_RUNTIME_ADVERSARIAL_TEST)
	@echo "PASS: CR7 adversarial C bridge contracts"
	@$(C_RUNTIME_SERVER_TEST)
	@echo "PASS: CR6 C-to-Assembly server lifecycle"
	@echo "PASS: C Runtime Bridge CR0-CR7 qualification"

c-runtime-sanitize: $(C_RUNTIME_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(C_RUNTIME_SANITIZE_TEST)
	@echo "PASS: CR7 ASan/UBSan bridge qualification"

c-assembly-abi-header-verify: $(C_ASSEMBLY_ABI_HEADER_VERIFY)
	@CC=$(CC) bash $(C_ASSEMBLY_ABI_HEADER_VERIFY)

c-runtime-frozen-assembly-verify: library-readiness $(C_RUNTIME_FROZEN_ASSEMBLY_VERIFY)
	@bash $(C_RUNTIME_FROZEN_ASSEMBLY_VERIFY)

c-runtime-dependency-verify: $(C_RUNTIME_LIB) $(C_RUNTIME_DEPENDENCY_VERIFY)
	@bash $(C_RUNTIME_DEPENDENCY_VERIFY)

c-runtime-reproducibility-verify: $(C_RUNTIME_REPRODUCIBILITY_VERIFY)
	@bash $(C_RUNTIME_REPRODUCIBILITY_VERIFY)

c-runtime-benchmark-verify: $(C_RUNTIME_BENCH) $(C_RUNTIME_BENCHMARK_VERIFY)
	@ARBORCORE_PERF_PROFILE=$(ARBORCORE_PERF_PROFILE) bash $(C_RUNTIME_BENCHMARK_VERIFY)

c-runtime-bridge-gate: $(C_RUNTIME_BRIDGE_GATE)
	@ARBORCORE_PERF_PROFILE=$(ARBORCORE_PERF_PROFILE) bash $(C_RUNTIME_BRIDGE_GATE)

# ============================================================
# Geometry Precision G0-G1 experimental qualification
# ============================================================

GEOMETRY_EXPERIMENT_DIR := experiments/geometry
GEOMETRY_EXPERIMENT_BUILD_DIR := $(BUILD_DIR)/geometry-precision-g0-g1
GEOMETRY_CANDIDATE_HEADER := $(GEOMETRY_EXPERIMENT_DIR)/fixed_point_candidates.h
GEOMETRY_CANDIDATE_TEST_SRC := $(C_TEST_DIR)/geometry_precision_candidate_test.c
GEOMETRY_CANDIDATE_TEST := $(BUILD_DIR)/geometry-precision-candidate-test
GEOMETRY_BENCH_SRC := $(BENCH_DIR)/geometry_precision_bench.c
GEOMETRY_BENCH := $(BUILD_DIR)/geometry-precision-bench
GEOMETRY_BENCH_RUNNER := $(TOOLS_DIR)/geometry_precision_benchmark_run.sh
GEOMETRY_SELECTION_TOOL := $(TOOLS_DIR)/geometry_precision_select.sh
GEOMETRY_WASM_PROBE := $(TOOLS_DIR)/geometry_precision_wasm_probe.sh
GEOMETRY_G0_G1_GATE := $(TOOLS_DIR)/geometry_precision_g0_g1_gate.sh
GEOMETRY_CPPFLAGS := $(ARBORCORE_C_CPPFLAGS) -I$(GEOMETRY_EXPERIMENT_DIR)

.PHONY: geometry-precision-candidate-test geometry-precision-benchmark-run
.PHONY: geometry-precision-g0-g1-gate

$(GEOMETRY_CANDIDATE_TEST): $(GEOMETRY_CANDIDATE_TEST_SRC) $(GEOMETRY_CANDIDATE_HEADER) | $(BUILD_DIR)
	$(CC) $(GEOMETRY_CPPFLAGS) $(ARBORCORE_C_CFLAGS) $< -o $@

$(GEOMETRY_BENCH): $(GEOMETRY_BENCH_SRC) $(GEOMETRY_CANDIDATE_HEADER) | $(BUILD_DIR)
	$(CC) $(GEOMETRY_CPPFLAGS) $(ARBORCORE_C_CFLAGS) $< -o $@

geometry-precision-candidate-test: $(GEOMETRY_CANDIDATE_TEST)
	@$(GEOMETRY_CANDIDATE_TEST)

geometry-precision-benchmark-run: $(GEOMETRY_BENCH) $(GEOMETRY_BENCH_RUNNER)
	@ARBORCORE_ROOT=$(ROOT_DIR) ARBOR_GEOMETRY_BENCH=$(ROOT_DIR)/$(GEOMETRY_BENCH) \
		bash $(GEOMETRY_BENCH_RUNNER)

geometry-precision-g0-g1-gate: $(GEOMETRY_G0_G1_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(GEOMETRY_G0_G1_GATE)

# ============================================================
# Geometry Precision G2-G4 production numerical layer
# ============================================================

GEOMETRY_BUILD_DIR := $(BUILD_DIR)/geometry
GEOMETRY_TEST_BUILD_DIR := $(BUILD_DIR)/geometry-tests
GEOMETRY_HEADER := include/arborcore/geometry.h
GEOMETRY_SOURCE := src/c/geometry.c
GEOMETRY_WASM_BUILTINS := src/wasm/geometry_int128_builtins.c
GEOMETRY_CONTRACT := geometry/arborcore-geometry-1.contract
GEOMETRY_OBJECT := $(GEOMETRY_BUILD_DIR)/geometry.o
GEOMETRY_LIB := $(BUILD_DIR)/libarborcore_geometry.a

GEOMETRY_SCALAR_TEST_OBJ := $(GEOMETRY_TEST_BUILD_DIR)/geometry_scalar_test.o
GEOMETRY_RECT_TEST_OBJ := $(GEOMETRY_TEST_BUILD_DIR)/geometry_rect_property_test.o
GEOMETRY_AFFINE_TEST_OBJ := $(GEOMETRY_TEST_BUILD_DIR)/geometry_affine_test.o
GEOMETRY_DEVICE_TEST_OBJ := $(GEOMETRY_TEST_BUILD_DIR)/geometry_device_test.o
GEOMETRY_VECTOR_TEST_OBJ := $(GEOMETRY_TEST_BUILD_DIR)/geometry_semantic_vector_test.o
GEOMETRY_SCALAR_TEST := $(BUILD_DIR)/geometry-scalar-test
GEOMETRY_RECT_TEST := $(BUILD_DIR)/geometry-rect-property-test
GEOMETRY_AFFINE_TEST := $(BUILD_DIR)/geometry-affine-test
GEOMETRY_DEVICE_TEST := $(BUILD_DIR)/geometry-device-test
GEOMETRY_VECTOR_TEST := $(BUILD_DIR)/geometry-semantic-vector-test

GEOMETRY_PRODUCTION_BENCH_SRC := $(BENCH_DIR)/geometry_production_bench.c
GEOMETRY_PRODUCTION_BENCH_OBJ := $(GEOMETRY_TEST_BUILD_DIR)/geometry_production_bench.o
GEOMETRY_PRODUCTION_BENCH := $(BUILD_DIR)/geometry-production-bench

GEOMETRY_LOWER_LAYER_VERIFY := $(TOOLS_DIR)/geometry_lower_layer_verify.sh
GEOMETRY_CONTRACT_VERIFY := $(TOOLS_DIR)/geometry_numerical_contract_verify.sh
GEOMETRY_WASM_VERIFY := $(TOOLS_DIR)/geometry_wasm_verify.sh
GEOMETRY_REPRO_VERIFY := $(TOOLS_DIR)/geometry_reproducibility_verify.sh
GEOMETRY_SANITIZE_VERIFY := $(TOOLS_DIR)/geometry_sanitize_verify.sh
GEOMETRY_PRODUCTION_BENCH_VERIFY := $(TOOLS_DIR)/geometry_production_benchmark_verify.sh
GEOMETRY_G2_G4_GATE := $(TOOLS_DIR)/geometry_g2_g4_gate.sh

.PHONY: geometry-library geometry-all geometry-check geometry-sanitize-verify
.PHONY: geometry-lower-layer-verify geometry-numerical-contract-verify
.PHONY: geometry-wasm-verify geometry-reproducibility-verify
.PHONY: geometry-production-benchmark-verify geometry-g2-g4-gate

$(GEOMETRY_BUILD_DIR) $(GEOMETRY_TEST_BUILD_DIR):
	mkdir -p $@

$(GEOMETRY_OBJECT): $(GEOMETRY_SOURCE) $(GEOMETRY_HEADER) | $(GEOMETRY_BUILD_DIR)
	$(CC) -Iinclude $(ARBORCORE_C_CFLAGS) -c $(GEOMETRY_SOURCE) -o $@

$(GEOMETRY_LIB): $(GEOMETRY_OBJECT) | $(BUILD_DIR)
	$(AR) rcsD $@ $(GEOMETRY_OBJECT)

geometry-library: $(GEOMETRY_LIB)

$(GEOMETRY_TEST_BUILD_DIR)/%.o: $(C_TEST_DIR)/%.c $(GEOMETRY_HEADER) | $(GEOMETRY_TEST_BUILD_DIR)
	$(CC) -Iinclude $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(GEOMETRY_SCALAR_TEST): $(GEOMETRY_SCALAR_TEST_OBJ) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(GEOMETRY_SCALAR_TEST_OBJ) $(GEOMETRY_LIB)

$(GEOMETRY_RECT_TEST): $(GEOMETRY_RECT_TEST_OBJ) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(GEOMETRY_RECT_TEST_OBJ) $(GEOMETRY_LIB)

$(GEOMETRY_AFFINE_TEST): $(GEOMETRY_AFFINE_TEST_OBJ) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(GEOMETRY_AFFINE_TEST_OBJ) $(GEOMETRY_LIB)

$(GEOMETRY_DEVICE_TEST): $(GEOMETRY_DEVICE_TEST_OBJ) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(GEOMETRY_DEVICE_TEST_OBJ) $(GEOMETRY_LIB)

$(GEOMETRY_VECTOR_TEST_OBJ): tests/c/geometry_semantic_vectors.h

$(GEOMETRY_VECTOR_TEST): $(GEOMETRY_VECTOR_TEST_OBJ) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(GEOMETRY_VECTOR_TEST_OBJ) $(GEOMETRY_LIB)

geometry-all: \
	$(GEOMETRY_LIB) \
	$(GEOMETRY_SCALAR_TEST) \
	$(GEOMETRY_RECT_TEST) \
	$(GEOMETRY_AFFINE_TEST) \
	$(GEOMETRY_DEVICE_TEST) \
	$(GEOMETRY_VECTOR_TEST)

geometry-check: geometry-all
	@$(GEOMETRY_SCALAR_TEST)
	@echo "PASS: G2 checked Q32.32 scalar arithmetic"
	@$(GEOMETRY_RECT_TEST)
	@echo "PASS: G2 point/rect/line algebra and transactional boundaries"
	@$(GEOMETRY_AFFINE_TEST)
	@echo "PASS: G3 affine/inverse/CORDIC/clipping contracts"
	@$(GEOMETRY_DEVICE_TEST)
	@echo "PASS: G4 exact rational logical/device mapping"
	@$(GEOMETRY_VECTOR_TEST)
	@echo "PASS: native G2-G4 semantic vector set"
	@echo "PASS: Geometry Precision G2-G4 native qualification"

$(GEOMETRY_PRODUCTION_BENCH_OBJ): $(GEOMETRY_PRODUCTION_BENCH_SRC) $(GEOMETRY_HEADER) $(GEOMETRY_CANDIDATE_HEADER) | $(GEOMETRY_TEST_BUILD_DIR)
	$(CC) -Iinclude -I$(GEOMETRY_EXPERIMENT_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(GEOMETRY_PRODUCTION_BENCH): $(GEOMETRY_PRODUCTION_BENCH_OBJ) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(GEOMETRY_PRODUCTION_BENCH_OBJ) $(GEOMETRY_LIB)

geometry-lower-layer-verify: $(GEOMETRY_LOWER_LAYER_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(GEOMETRY_LOWER_LAYER_VERIFY)

geometry-numerical-contract-verify: $(GEOMETRY_LIB) $(GEOMETRY_CONTRACT) $(GEOMETRY_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(GEOMETRY_CONTRACT_VERIFY)

geometry-wasm-verify: $(GEOMETRY_HEADER) $(GEOMETRY_SOURCE) $(GEOMETRY_WASM_BUILTINS) tests/c/geometry_wasm_selftest.c tests/c/geometry_semantic_vectors.h $(GEOMETRY_WASM_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(GEOMETRY_WASM_VERIFY)

geometry-reproducibility-verify: $(GEOMETRY_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) CC=$(CC) AR=$(AR) bash $(GEOMETRY_REPRO_VERIFY)

geometry-sanitize-verify: $(GEOMETRY_SANITIZE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) CC=$(CC) bash $(GEOMETRY_SANITIZE_VERIFY)

geometry-production-benchmark-verify: $(GEOMETRY_PRODUCTION_BENCH) $(GEOMETRY_PRODUCTION_BENCH_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) ARBOR_GEOMETRY_PRODUCTION_BENCH=$(ROOT_DIR)/$(GEOMETRY_PRODUCTION_BENCH) \
		bash $(GEOMETRY_PRODUCTION_BENCH_VERIFY)

geometry-g2-g4-gate: $(GEOMETRY_G2_G4_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(GEOMETRY_G2_G4_GATE)

# ============================================================
# Precision Renderer R0-R3 foundational qualification
# ============================================================

RENDERER_EXPERIMENT_DIR := experiments/renderer
RENDERER_BUILD_DIR := $(BUILD_DIR)/renderer-r0-r3
RENDERER_CANDIDATE_HEADER := $(RENDERER_EXPERIMENT_DIR)/raster_foundation_candidates.h
RENDERER_CANDIDATE_TEST_SRC := $(C_TEST_DIR)/renderer_foundation_candidate_test.c
RENDERER_CANDIDATE_TEST := $(BUILD_DIR)/renderer-foundation-candidate-test
RENDERER_WASM_TEST_SRC := $(C_TEST_DIR)/renderer_foundation_wasm_selftest.c
RENDERER_BENCH_SRC := $(BENCH_DIR)/renderer_foundation_bench.c
RENDERER_BENCH := $(BUILD_DIR)/renderer-foundation-bench
RENDERER_BENCH_RUNNER := $(TOOLS_DIR)/renderer_r0_r3_benchmark_run.sh
RENDERER_SELECT := $(TOOLS_DIR)/renderer_r0_r3_select.sh
RENDERER_WASM_VERIFY := $(TOOLS_DIR)/renderer_r0_r3_wasm_verify.sh
RENDERER_LOWER_LAYER_VERIFY := $(TOOLS_DIR)/renderer_r0_r3_lower_layer_verify.sh
RENDERER_R0_R3_GATE := $(TOOLS_DIR)/renderer_r0_r3_gate.sh
RENDERER_CPPFLAGS := -D_POSIX_C_SOURCE=200809L -I$(RENDERER_EXPERIMENT_DIR)

.PHONY: renderer-r0-r3-candidate-test renderer-r0-r3-benchmark-run
.PHONY: renderer-r0-r3-wasm-verify renderer-r0-r3-lower-layer-verify renderer-r0-r3-gate

$(RENDERER_CANDIDATE_TEST): $(RENDERER_CANDIDATE_TEST_SRC) $(RENDERER_CANDIDATE_HEADER) | $(BUILD_DIR)
	$(CC) $(RENDERER_CPPFLAGS) $(ARBORCORE_C_CFLAGS) $< -o $@

$(RENDERER_BENCH): $(RENDERER_BENCH_SRC) $(RENDERER_CANDIDATE_HEADER) | $(BUILD_DIR)
	$(CC) $(RENDERER_CPPFLAGS) $(ARBORCORE_C_CFLAGS) $< -o $@

renderer-r0-r3-candidate-test: $(RENDERER_CANDIDATE_TEST)
	@$(RENDERER_CANDIDATE_TEST)

renderer-r0-r3-benchmark-run: $(RENDERER_BENCH) $(RENDERER_BENCH_RUNNER)
	@ARBORCORE_ROOT=$(ROOT_DIR) ARBOR_RENDERER_FOUNDATION_BENCH=$(ROOT_DIR)/$(RENDERER_BENCH) \
		bash $(RENDERER_BENCH_RUNNER)

renderer-r0-r3-wasm-verify: $(RENDERER_CANDIDATE_HEADER) $(RENDERER_WASM_TEST_SRC) $(RENDERER_WASM_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_WASM_VERIFY)

renderer-r0-r3-lower-layer-verify: $(RENDERER_LOWER_LAYER_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_LOWER_LAYER_VERIFY)

renderer-r0-r3-gate: $(RENDERER_R0_R3_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_R0_R3_GATE)

# ============================================================
# Precision Renderer R4-R9 production reference renderer
# ============================================================

RENDERER_PROD_BUILD_DIR := $(BUILD_DIR)/renderer
RENDERER_PROD_TEST_BUILD_DIR := $(BUILD_DIR)/renderer-tests
RENDERER_HEADER := include/arborcore/renderer.h
RENDERER_SOURCE := src/c/renderer.c
RENDERER_LUT := renderer/srgb8_linear16_lut.h
RENDERER_CONTRACT := renderer/arborcore-renderer-1.contract
RENDERER_WASM_MEMORY_BUILTINS := src/wasm/renderer_memory_builtins.c
RENDERER_OBJECT := $(RENDERER_PROD_BUILD_DIR)/renderer.o
RENDERER_LIB := $(BUILD_DIR)/libarborcore_renderer.a

RENDERER_SURFACE_TEST_SRC := $(C_TEST_DIR)/renderer_surface_test.c
RENDERER_BLEND_TEST_SRC := $(C_TEST_DIR)/renderer_blend_test.c
RENDERER_RECT_TEST_SRC := $(C_TEST_DIR)/renderer_rect_test.c
RENDERER_PATH_TEST_SRC := $(C_TEST_DIR)/renderer_path_test.c
RENDERER_GOLDEN_NATIVE_SRC := $(C_TEST_DIR)/renderer_golden_native.c
RENDERER_GOLDEN_SCENE := $(C_TEST_DIR)/renderer_golden_scene.h
RENDERER_WASM_SELFTEST_SRC := $(C_TEST_DIR)/renderer_wasm_selftest.c

RENDERER_SURFACE_TEST_OBJ := $(RENDERER_PROD_TEST_BUILD_DIR)/renderer_surface_test.o
RENDERER_BLEND_TEST_OBJ := $(RENDERER_PROD_TEST_BUILD_DIR)/renderer_blend_test.o
RENDERER_RECT_TEST_OBJ := $(RENDERER_PROD_TEST_BUILD_DIR)/renderer_rect_test.o
RENDERER_PATH_TEST_OBJ := $(RENDERER_PROD_TEST_BUILD_DIR)/renderer_path_test.o
RENDERER_GOLDEN_NATIVE_OBJ := $(RENDERER_PROD_TEST_BUILD_DIR)/renderer_golden_native.o

RENDERER_SURFACE_TEST := $(BUILD_DIR)/renderer-surface-test
RENDERER_BLEND_TEST := $(BUILD_DIR)/renderer-blend-test
RENDERER_RECT_TEST := $(BUILD_DIR)/renderer-rect-test
RENDERER_PATH_TEST := $(BUILD_DIR)/renderer-path-test
RENDERER_GOLDEN_NATIVE := $(BUILD_DIR)/renderer-golden-native

RENDERER_PRODUCTION_BENCH_SRC := $(BENCH_DIR)/renderer_production_bench.c
RENDERER_PRODUCTION_BENCH_OBJ := $(RENDERER_PROD_TEST_BUILD_DIR)/renderer_production_bench.o
RENDERER_PRODUCTION_BENCH := $(BUILD_DIR)/renderer-production-bench

RENDERER_R4_R9_LOWER_LAYER_VERIFY := $(TOOLS_DIR)/renderer_r4_r9_lower_layer_verify.sh
RENDERER_CONTRACT_VERIFY := $(TOOLS_DIR)/renderer_contract_verify.sh
RENDERER_WASM_GOLDEN_VERIFY := $(TOOLS_DIR)/renderer_wasm_golden_verify.sh
RENDERER_REPRO_VERIFY := $(TOOLS_DIR)/renderer_reproducibility_verify.sh
RENDERER_SANITIZE_VERIFY := $(TOOLS_DIR)/renderer_sanitize_verify.sh
RENDERER_PRODUCTION_BENCH_VERIFY := $(TOOLS_DIR)/renderer_production_benchmark_verify.sh
RENDERER_R4_R9_GATE := $(TOOLS_DIR)/renderer_r4_r9_gate.sh

.PHONY: renderer-library renderer-all renderer-check renderer-sanitize-verify
.PHONY: renderer-r4-r9-lower-layer-verify renderer-contract-verify
.PHONY: renderer-wasm-golden-verify renderer-reproducibility-verify
.PHONY: renderer-production-benchmark-verify renderer-r4-r9-gate

$(RENDERER_PROD_BUILD_DIR) $(RENDERER_PROD_TEST_BUILD_DIR):
	mkdir -p $@

$(RENDERER_OBJECT): $(RENDERER_SOURCE) $(RENDERER_HEADER) $(RENDERER_LUT) $(GEOMETRY_HEADER) | $(RENDERER_PROD_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $(RENDERER_SOURCE) -o $@

$(RENDERER_LIB): $(RENDERER_OBJECT)
	$(AR) rcsD $@ $^

renderer-library: $(RENDERER_LIB)

$(RENDERER_PROD_TEST_BUILD_DIR)/%.o: $(C_TEST_DIR)/%.c $(RENDERER_HEADER) $(GEOMETRY_HEADER) | $(RENDERER_PROD_TEST_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -I$(C_TEST_DIR) -c $< -o $@

$(RENDERER_SURFACE_TEST): $(RENDERER_SURFACE_TEST_OBJ) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

$(RENDERER_BLEND_TEST): $(RENDERER_BLEND_TEST_OBJ) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

$(RENDERER_RECT_TEST): $(RENDERER_RECT_TEST_OBJ) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

$(RENDERER_PATH_TEST): $(RENDERER_PATH_TEST_OBJ) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

$(RENDERER_GOLDEN_NATIVE_OBJ): $(RENDERER_GOLDEN_SCENE)
$(RENDERER_GOLDEN_NATIVE): $(RENDERER_GOLDEN_NATIVE_OBJ) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

renderer-all: \
	$(RENDERER_SURFACE_TEST) \
	$(RENDERER_BLEND_TEST) \
	$(RENDERER_RECT_TEST) \
	$(RENDERER_PATH_TEST) \
	$(RENDERER_GOLDEN_NATIVE)

renderer-check: renderer-all
	@$(RENDERER_SURFACE_TEST)
	@$(RENDERER_BLEND_TEST)
	@$(RENDERER_RECT_TEST)
	@$(RENDERER_PATH_TEST)
	@echo "PASS: Precision Renderer R4-R7 native qualification"

renderer-r4-r9-lower-layer-verify: $(RENDERER_R4_R9_LOWER_LAYER_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_R4_R9_LOWER_LAYER_VERIFY)

renderer-contract-verify: $(RENDERER_LIB) $(RENDERER_CONTRACT) $(RENDERER_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_CONTRACT_VERIFY)

renderer-wasm-golden-verify: $(RENDERER_HEADER) $(RENDERER_SOURCE) $(RENDERER_LUT) $(RENDERER_CONTRACT) $(RENDERER_WASM_MEMORY_BUILTINS) $(RENDERER_WASM_SELFTEST_SRC) $(RENDERER_GOLDEN_SCENE) $(RENDERER_WASM_GOLDEN_VERIFY) $(RENDERER_GOLDEN_NATIVE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_WASM_GOLDEN_VERIFY)

renderer-reproducibility-verify: $(RENDERER_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_REPRO_VERIFY)

renderer-sanitize-verify: $(RENDERER_SANITIZE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_SANITIZE_VERIFY)

$(RENDERER_PRODUCTION_BENCH_OBJ): $(RENDERER_PRODUCTION_BENCH_SRC) $(RENDERER_HEADER) $(RENDERER_CANDIDATE_HEADER) | $(RENDERER_PROD_TEST_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) -I$(RENDERER_EXPERIMENT_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(RENDERER_PRODUCTION_BENCH): $(RENDERER_PRODUCTION_BENCH_OBJ) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

renderer-production-benchmark-verify: $(RENDERER_PRODUCTION_BENCH) $(RENDERER_PRODUCTION_BENCH_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) ARBOR_RENDERER_PRODUCTION_BENCH=$(ROOT_DIR)/$(RENDERER_PRODUCTION_BENCH) \
		bash $(RENDERER_PRODUCTION_BENCH_VERIFY)

renderer-r4-r9-gate: $(RENDERER_R4_R9_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(RENDERER_R4_R9_GATE)

# ============================================================
# Browser Precision Surface v1 — retained C/WASM lower layer
# ============================================================

BROWSER_BUILD_DIR := $(BUILD_DIR)/browser-surface
BROWSER_TEST_BUILD_DIR := $(BUILD_DIR)/browser-tests
BROWSER_HEADER := include/arborcore/browser_surface.h
BROWSER_SOURCE := src/c/browser_surface.c
BROWSER_CONTRACT := browser/arborcore-browser-surface-1.contract
BROWSER_EXPORT_ACCEL := browser/linear16_srgb8_bucket12.h
BROWSER_OBJECT := $(BROWSER_BUILD_DIR)/browser_surface.o
BROWSER_LIB := $(BUILD_DIR)/libarborcore_browser_surface.a
BROWSER_TEST_SRC := $(C_TEST_DIR)/browser_surface_test.c
BROWSER_TEST_OBJ := $(BROWSER_TEST_BUILD_DIR)/browser_surface_test.o
BROWSER_TEST := $(BUILD_DIR)/browser-surface-test
BROWSER_GOLDEN_NATIVE_SRC := $(C_TEST_DIR)/browser_export_golden_native.c
BROWSER_GOLDEN_NATIVE_OBJ := $(BROWSER_TEST_BUILD_DIR)/browser_export_golden_native.o
BROWSER_GOLDEN_NATIVE := $(BUILD_DIR)/browser-export-golden-native
BROWSER_WASM_SELFTEST_SRC := $(C_TEST_DIR)/browser_wasm_selftest.c
BROWSER_BENCH_SRC := $(BENCH_DIR)/browser_surface_bench.c
BROWSER_BENCH_OBJ := $(BROWSER_TEST_BUILD_DIR)/browser_surface_bench.o
BROWSER_BENCH := $(BUILD_DIR)/browser-surface-bench
BROWSER_LOWER_VERIFY := $(TOOLS_DIR)/browser_b0_b6_lower_layer_verify.sh
BROWSER_CONTRACT_VERIFY := $(TOOLS_DIR)/browser_contract_verify.sh
BROWSER_WASM_VERIFY := $(TOOLS_DIR)/browser_wasm_verify.sh
BROWSER_REPRO_VERIFY := $(TOOLS_DIR)/browser_reproducibility_verify.sh
BROWSER_SANITIZE_VERIFY := $(TOOLS_DIR)/browser_sanitize_verify.sh
BROWSER_BENCH_VERIFY := $(TOOLS_DIR)/browser_benchmark_verify.sh

.PHONY: browser-library browser-check browser-b0-b6-lower-layer-verify
.PHONY: browser-contract-verify browser-wasm-verify
.PHONY: browser-reproducibility-verify browser-sanitize-verify browser-benchmark-verify

$(BROWSER_BUILD_DIR) $(BROWSER_TEST_BUILD_DIR):
	mkdir -p $@

$(BROWSER_OBJECT): $(BROWSER_SOURCE) $(BROWSER_HEADER) $(BROWSER_EXPORT_ACCEL) $(RENDERER_HEADER) $(RENDERER_LUT) | $(BROWSER_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $(BROWSER_SOURCE) -o $@

$(BROWSER_LIB): $(BROWSER_OBJECT)
	$(AR) rcsD $@ $^

browser-library: $(BROWSER_LIB)

$(BROWSER_TEST_OBJ): $(BROWSER_TEST_SRC) $(BROWSER_HEADER) | $(BROWSER_TEST_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(BROWSER_TEST): $(BROWSER_TEST_OBJ) $(BROWSER_LIB) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

$(BROWSER_GOLDEN_NATIVE_OBJ): $(BROWSER_GOLDEN_NATIVE_SRC) $(BROWSER_HEADER) $(RENDERER_GOLDEN_SCENE) | $(BROWSER_TEST_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) -I$(C_TEST_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(BROWSER_GOLDEN_NATIVE): $(BROWSER_GOLDEN_NATIVE_OBJ) $(BROWSER_LIB) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

$(BROWSER_BENCH_OBJ): $(BROWSER_BENCH_SRC) $(BROWSER_HEADER) | $(BROWSER_TEST_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(BROWSER_BENCH): $(BROWSER_BENCH_OBJ) $(BROWSER_LIB) $(RENDERER_LIB) $(GEOMETRY_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

browser-check: $(BROWSER_TEST) $(BROWSER_GOLDEN_NATIVE)
	@$(BROWSER_TEST)
	@echo "PASS: Browser Precision Surface B0-B1 native qualification"

browser-b0-b6-lower-layer-verify: $(BROWSER_LOWER_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(BROWSER_LOWER_VERIFY)

browser-contract-verify: $(BROWSER_LIB) $(BROWSER_CONTRACT) $(BROWSER_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(BROWSER_CONTRACT_VERIFY)

browser-wasm-verify: $(BROWSER_GOLDEN_NATIVE) $(BROWSER_WASM_VERIFY) $(BROWSER_WASM_SELFTEST_SRC)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(BROWSER_WASM_VERIFY)

browser-reproducibility-verify: $(BROWSER_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) CC=$(CC) AR=$(AR) bash $(BROWSER_REPRO_VERIFY)

browser-sanitize-verify: $(BROWSER_SANITIZE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) CC=$(CC) bash $(BROWSER_SANITIZE_VERIFY)

browser-benchmark-verify: $(BROWSER_BENCH) $(BROWSER_BENCH_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) ARBOR_BROWSER_BENCH=$(ROOT_DIR)/$(BROWSER_BENCH) bash $(BROWSER_BENCH_VERIFY)

# ============================================================
# Browser WebGPU v1 — historical contract only
# ============================================================

WEBGPU_CONTRACT := browser/arborcore-browser-webgpu-1.contract
WEBGPU_CONTRACT_VERIFY := $(TOOLS_DIR)/webgpu_contract_verify.sh

.PHONY: webgpu-contract-verify

webgpu-contract-verify: $(WEBGPU_CONTRACT) $(WEBGPU_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(WEBGPU_CONTRACT_VERIFY)

# ============================================================
# Browser Language Boundary v2 — Assembly/C/WASM/WGSL first
# ============================================================

LBV2_HEADER := include/arborcore/browser_host_v2.h
LBV2_C_SOURCE := src/c/browser_host_v2.c
LBV2_HOST_JS := browser/arborcore_host.js
LBV2_CONTRACT := browser/arborcore-browser-language-boundary-2.contract
LBV2_NATIVE_TEST_SRC := tests/c/browser_host_v2_test.c
LBV2_NATIVE_TEST := $(BUILD_DIR)/browser-host-v2-test
LBV2_CONTRACT_VERIFY := $(TOOLS_DIR)/browser_language_boundary_v2_contract_verify.sh
LBV2_WASM_VERIFY := $(TOOLS_DIR)/browser_language_boundary_v2_wasm_verify.sh
LBV2_LIVE_VERIFY := $(TOOLS_DIR)/browser_language_boundary_v2_live_verify.sh
LBV2_REPRO_VERIFY := $(TOOLS_DIR)/browser_language_boundary_v2_reproducibility_verify.sh
LBV2_GATE := $(TOOLS_DIR)/browser_language_boundary_v2_gate.sh

.PHONY: browser-language-boundary-v2-native-test browser-language-boundary-v2-contract-verify
.PHONY: browser-language-boundary-v2-wasm-verify browser-language-boundary-v2-live-verify
.PHONY: browser-language-boundary-v2-live-evidence-verify browser-language-boundary-v2-reproducibility-verify
.PHONY: browser-language-boundary-v2-gate

$(LBV2_NATIVE_TEST): $(LBV2_NATIVE_TEST_SRC) $(LBV2_C_SOURCE) $(LBV2_HEADER) | $(BUILD_DIR)
	$(CC) -Iinclude -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef $(LBV2_NATIVE_TEST_SRC) $(LBV2_C_SOURCE) -o $@

browser-language-boundary-v2-native-test: $(LBV2_NATIVE_TEST)
	@$(LBV2_NATIVE_TEST)

browser-language-boundary-v2-contract-verify: $(LBV2_CONTRACT) $(LBV2_HOST_JS) $(LBV2_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(LBV2_CONTRACT_VERIFY)

browser-language-boundary-v2-wasm-verify: $(LBV2_WASM_VERIFY) $(LBV2_C_SOURCE) $(LBV2_HEADER)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(LBV2_WASM_VERIFY)

browser-language-boundary-v2-live-verify: $(LBV2_LIVE_VERIFY) $(LBV2_HOST_JS)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(LBV2_LIVE_VERIFY) serve

browser-language-boundary-v2-live-evidence-verify: $(LBV2_LIVE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(LBV2_LIVE_VERIFY) evidence

browser-language-boundary-v2-reproducibility-verify: $(LBV2_REPRO_VERIFY) $(LBV2_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(LBV2_REPRO_VERIFY)

browser-language-boundary-v2-gate: $(LBV2_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(LBV2_GATE)

# ============================================================
# Browser/WebGPU v1 historical implementation verification
# ============================================================

BROWSER_V1_HISTORY_VERIFY := $(TOOLS_DIR)/browser_v1_history_verify.sh
BROWSER_V1_PRECISION_VECTORS := tests/data/browser_v1_precision_vectors.json

.PHONY: browser-v1-history-verify

browser-v1-history-verify: \
	$(BROWSER_V1_HISTORY_VERIFY) \
	$(BROWSER_V1_PRECISION_VECTORS) \
	browser/arborcore-browser-surface-1.contract \
	browser/arborcore-browser-webgpu-1.contract
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(BROWSER_V1_HISTORY_VERIFY)

# ============================================================
# Browser v1 JavaScript retirement
# ============================================================

BROWSER_V1_JS_RETIREMENT_VERIFY := $(TOOLS_DIR)/browser_v1_js_retirement_verify.sh
BROWSER_V1_JS_RETIREMENT_GATE := $(TOOLS_DIR)/browser_v1_js_retirement_gate.sh
BROWSER_V1_JS_RETIREMENT_CONTRACT := browser/arborcore-browser-v1-js-retirement-1.contract

.PHONY: browser-v1-js-retirement-verify browser-v1-js-retirement-gate

browser-v1-js-retirement-verify: \
	$(BROWSER_V1_JS_RETIREMENT_VERIFY) \
	$(BROWSER_V1_JS_RETIREMENT_CONTRACT) \
	$(BROWSER_V1_PRECISION_VECTORS)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(BROWSER_V1_JS_RETIREMENT_VERIFY)

browser-v1-js-retirement-gate: \
	$(BROWSER_V1_JS_RETIREMENT_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(BROWSER_V1_JS_RETIREMENT_GATE)

# >>> ARBORCORE BV2H0-BV2H9 INTEGRATED CANDIDATE >>>
BV2H_INVENTORY := tools/browser_v2_hardening_inventory.sh
BV2H_CONTRACT_VERIFY := tools/browser_v2_hardening_contract_verify.sh
BV2H_NATIVE_VERIFY := tools/browser_v2_hardening_native_verify.sh
BV2H_WASM_VERIFY := tools/browser_v2_hardening_wasm_verify.sh
BV2H_LIVE_VERIFY := tools/browser_v2_hardening_live_verify.sh
BV2H_PERFORMANCE_VERIFY := tools/browser_v2_hardening_performance_verify.py
BV2H_OPT3_VERIFY := tools/browser_v2_opt3_qualification_verify.py
BV2H_REPRO_VERIFY := tools/browser_v2_hardening_reproducibility_verify.sh
BV2H_GATE := tools/browser_v2_hardening_gate.sh

.PHONY: browser-v2-hardening-inventory browser-v2-hardening-contract-verify browser-v2-hardening-native-verify browser-v2-hardening-wasm-verify browser-v2-hardening-live-verify browser-v2-hardening-live-evidence-verify browser-v2-hardening-performance-verify browser-v2-opt3-qualification-verify browser-v2-hardening-reproducibility-verify browser-v2-hardening-prelive-gate browser-v2-hardening-gate

browser-v2-hardening-inventory:
	bash $(BV2H_INVENTORY)

browser-v2-hardening-contract-verify:
	bash $(BV2H_CONTRACT_VERIFY)

browser-v2-hardening-native-verify:
	bash $(BV2H_NATIVE_VERIFY)

browser-v2-hardening-wasm-verify:
	bash $(BV2H_WASM_VERIFY)

browser-v2-hardening-live-verify:
	bash $(BV2H_LIVE_VERIFY)

browser-v2-hardening-live-evidence-verify:
	bash $(BV2H_LIVE_VERIFY) evidence

browser-v2-hardening-performance-verify:
	python3 $(BV2H_PERFORMANCE_VERIFY)

browser-v2-opt3-qualification-verify:
	python3 $(BV2H_OPT3_VERIFY)

browser-v2-hardening-reproducibility-verify:
	bash $(BV2H_REPRO_VERIFY)

browser-v2-hardening-prelive-gate:
	bash $(BV2H_GATE) prelive

browser-v2-hardening-gate:
	bash $(BV2H_GATE) full
# <<< ARBORCORE BV2H0-BV2H9 INTEGRATED CANDIDATE <<<

# ============================================================
# Application / DDD / MVC Foundation AF0-AF1
# ============================================================

APPLICATION_FOUNDATION_CONTRACT := application/arborcore-application-ddd-mvc-foundation-1.contract
APPLICATION_FOUNDATION_HEADER := include/arborcore/application.h
APPLICATION_FOUNDATION_SOURCE := src/c/application_foundation.c
APPLICATION_FOUNDATION_TEST_SOURCE := tests/c/application_foundation_test.c
APPLICATION_FOUNDATION_BUILD_DIR := $(BUILD_DIR)/application-foundation
APPLICATION_FOUNDATION_OBJ := $(APPLICATION_FOUNDATION_BUILD_DIR)/application_foundation.o
APPLICATION_FOUNDATION_TEST_OBJ := $(APPLICATION_FOUNDATION_BUILD_DIR)/application_foundation_test.o
APPLICATION_FOUNDATION_LIB := $(BUILD_DIR)/libarborcore_application.a
APPLICATION_FOUNDATION_TEST := $(BUILD_DIR)/application-foundation-test
APPLICATION_FOUNDATION_SANITIZE_TEST := $(BUILD_DIR)/application-foundation-sanitize-test
APPLICATION_FOUNDATION_CONTRACT_VERIFY := tools/application_foundation_contract_verify.sh
APPLICATION_FOUNDATION_FROZEN_VERIFY := tools/application_foundation_frozen_layers_verify.sh
APPLICATION_FOUNDATION_NATIVE_VERIFY := tools/application_foundation_native_verify.sh
APPLICATION_FOUNDATION_REPRO_VERIFY := tools/application_foundation_reproducibility_verify.sh
APPLICATION_FOUNDATION_SCOPE_VERIFY := tools/application_foundation_scope_verify.sh
APPLICATION_FOUNDATION_GATE := tools/application_foundation_gate.sh

.PHONY: application-foundation-library application-foundation-native-test application-foundation-sanitize
.PHONY: application-foundation-contract-verify application-foundation-frozen-layers-verify
.PHONY: application-foundation-reproducibility-verify application-foundation-scope-verify application-foundation-gate

$(APPLICATION_FOUNDATION_BUILD_DIR):
	mkdir -p $@

$(APPLICATION_FOUNDATION_OBJ): $(APPLICATION_FOUNDATION_SOURCE) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(APPLICATION_FOUNDATION_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(APPLICATION_FOUNDATION_TEST_OBJ): $(APPLICATION_FOUNDATION_TEST_SOURCE) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(APPLICATION_FOUNDATION_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(APPLICATION_FOUNDATION_LIB): $(APPLICATION_FOUNDATION_OBJ) | $(BUILD_DIR)
	$(AR) rcsD $@ $(APPLICATION_FOUNDATION_OBJ)

application-foundation-library: $(APPLICATION_FOUNDATION_LIB)

$(APPLICATION_FOUNDATION_TEST): $(APPLICATION_FOUNDATION_TEST_OBJ) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(APPLICATION_FOUNDATION_TEST_OBJ) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

application-foundation-native-test: $(APPLICATION_FOUNDATION_TEST)
	@$(APPLICATION_FOUNDATION_TEST)

$(APPLICATION_FOUNDATION_SANITIZE_TEST): $(APPLICATION_FOUNDATION_TEST_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(APPLICATION_FOUNDATION_TEST_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

application-foundation-sanitize: $(APPLICATION_FOUNDATION_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(APPLICATION_FOUNDATION_SANITIZE_TEST)
	@echo "PASS: AF1 ASan/UBSan foundation qualification"

application-foundation-contract-verify: $(APPLICATION_FOUNDATION_CONTRACT) $(APPLICATION_FOUNDATION_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_FOUNDATION_CONTRACT_VERIFY)

application-foundation-frozen-layers-verify: $(APPLICATION_FOUNDATION_FROZEN_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_FOUNDATION_FROZEN_VERIFY)

application-foundation-scope-verify: $(APPLICATION_FOUNDATION_SCOPE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_FOUNDATION_SCOPE_VERIFY)

application-foundation-reproducibility-verify: $(APPLICATION_FOUNDATION_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_FOUNDATION_REPRO_VERIFY)

application-foundation-gate: $(APPLICATION_FOUNDATION_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_FOUNDATION_GATE)

# ============================================================
# Application / DDD / MVC AF2 Shared Capability Kernel
# ============================================================

APPLICATION_CAPABILITY_KERNEL_CONTRACT := application/arborcore-application-capability-kernel-1.contract
APPLICATION_CAPABILITY_KERNEL_HEADER := include/arborcore/capability.h
APPLICATION_CAPABILITY_KERNEL_SOURCE := src/c/capability_kernel.c
APPLICATION_CAPABILITY_KERNEL_TEST_SOURCE := tests/c/application_capability_kernel_test.c
APPLICATION_CAPABILITY_KERNEL_BUILD_DIR := $(BUILD_DIR)/application-capability-kernel
APPLICATION_CAPABILITY_KERNEL_OBJ := $(APPLICATION_CAPABILITY_KERNEL_BUILD_DIR)/capability_kernel.o
APPLICATION_CAPABILITY_KERNEL_TEST_OBJ := $(APPLICATION_CAPABILITY_KERNEL_BUILD_DIR)/application_capability_kernel_test.o
APPLICATION_CAPABILITY_KERNEL_LIB := $(BUILD_DIR)/libarborcore_capability.a
APPLICATION_CAPABILITY_KERNEL_TEST := $(BUILD_DIR)/application-capability-kernel-test
APPLICATION_CAPABILITY_KERNEL_SANITIZE_TEST := $(BUILD_DIR)/application-capability-kernel-sanitize-test
APPLICATION_CAPABILITY_KERNEL_BASELINE_VERIFY := tools/application_capability_kernel_baseline_verify.sh
APPLICATION_CAPABILITY_KERNEL_CONTRACT_VERIFY := tools/application_capability_kernel_contract_verify.sh
APPLICATION_CAPABILITY_KERNEL_NATIVE_VERIFY := tools/application_capability_kernel_native_verify.sh
APPLICATION_CAPABILITY_KERNEL_REPRO_VERIFY := tools/application_capability_kernel_reproducibility_verify.sh
APPLICATION_CAPABILITY_KERNEL_SCOPE_VERIFY := tools/application_capability_kernel_scope_verify.sh
APPLICATION_CAPABILITY_KERNEL_GATE := tools/application_capability_kernel_gate.sh

.PHONY: application-capability-kernel-library application-capability-kernel-native-test application-capability-kernel-sanitize
.PHONY: application-capability-kernel-baseline-verify application-capability-kernel-contract-verify
.PHONY: application-capability-kernel-reproducibility-verify application-capability-kernel-scope-verify application-capability-kernel-gate

$(APPLICATION_CAPABILITY_KERNEL_BUILD_DIR):
	mkdir -p $@

$(APPLICATION_CAPABILITY_KERNEL_OBJ): $(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/application.h include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(APPLICATION_CAPABILITY_KERNEL_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(APPLICATION_CAPABILITY_KERNEL_TEST_OBJ): $(APPLICATION_CAPABILITY_KERNEL_TEST_SOURCE) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/application.h include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(APPLICATION_CAPABILITY_KERNEL_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(APPLICATION_CAPABILITY_KERNEL_LIB): $(APPLICATION_CAPABILITY_KERNEL_OBJ) | $(BUILD_DIR)
	$(AR) rcsD $@ $(APPLICATION_CAPABILITY_KERNEL_OBJ)

application-capability-kernel-library: $(APPLICATION_CAPABILITY_KERNEL_LIB)

$(APPLICATION_CAPABILITY_KERNEL_TEST): $(APPLICATION_CAPABILITY_KERNEL_TEST_OBJ) $(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(APPLICATION_CAPABILITY_KERNEL_TEST_OBJ) $(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

application-capability-kernel-native-test: $(APPLICATION_CAPABILITY_KERNEL_TEST)
	@$(APPLICATION_CAPABILITY_KERNEL_TEST)

$(APPLICATION_CAPABILITY_KERNEL_SANITIZE_TEST): $(APPLICATION_CAPABILITY_KERNEL_TEST_SOURCE) $(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(APPLICATION_CAPABILITY_KERNEL_HEADER) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(APPLICATION_CAPABILITY_KERNEL_TEST_SOURCE) $(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

application-capability-kernel-sanitize: $(APPLICATION_CAPABILITY_KERNEL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(APPLICATION_CAPABILITY_KERNEL_SANITIZE_TEST)
	@echo "PASS: AF2 ASan/UBSan capability-kernel qualification"

application-capability-kernel-baseline-verify: $(APPLICATION_CAPABILITY_KERNEL_BASELINE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_CAPABILITY_KERNEL_BASELINE_VERIFY)

application-capability-kernel-contract-verify: $(APPLICATION_CAPABILITY_KERNEL_CONTRACT) $(APPLICATION_CAPABILITY_KERNEL_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_CAPABILITY_KERNEL_CONTRACT_VERIFY)

application-capability-kernel-scope-verify: $(APPLICATION_CAPABILITY_KERNEL_SCOPE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_CAPABILITY_KERNEL_SCOPE_VERIFY)

application-capability-kernel-reproducibility-verify: $(APPLICATION_CAPABILITY_KERNEL_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_CAPABILITY_KERNEL_REPRO_VERIFY)

application-capability-kernel-gate: $(APPLICATION_CAPABILITY_KERNEL_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_CAPABILITY_KERNEL_GATE)

# ============================================================
# Application / DDD / MVC AF3 Application Service Runtime
# ============================================================

APPLICATION_SERVICE_RUNTIME_CONTRACT := application/arborcore-application-service-runtime-1.contract
APPLICATION_SERVICE_RUNTIME_HEADER := include/arborcore/application_service.h
APPLICATION_SERVICE_RUNTIME_SOURCE := src/c/application_service.c
APPLICATION_SERVICE_RUNTIME_TEST_SOURCE := tests/c/application_service_runtime_test.c
APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_SOURCE := tests/c/application_service_runtime_adversarial_test.c
APPLICATION_SERVICE_RUNTIME_ABI_SOURCE := tests/asm/application_service_runtime_abi_test.asm
APPLICATION_SERVICE_RUNTIME_BUILD_DIR := $(BUILD_DIR)/application-service-runtime
APPLICATION_SERVICE_RUNTIME_OBJ := $(APPLICATION_SERVICE_RUNTIME_BUILD_DIR)/application_service.o
APPLICATION_SERVICE_RUNTIME_TEST_OBJ := $(APPLICATION_SERVICE_RUNTIME_BUILD_DIR)/application_service_runtime_test.o
APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_OBJ := $(APPLICATION_SERVICE_RUNTIME_BUILD_DIR)/application_service_runtime_adversarial_test.o
APPLICATION_SERVICE_RUNTIME_ABI_OBJ := $(APPLICATION_SERVICE_RUNTIME_BUILD_DIR)/application_service_runtime_abi_test.o
APPLICATION_SERVICE_RUNTIME_LIB := $(BUILD_DIR)/libarborcore_application_service.a
APPLICATION_SERVICE_RUNTIME_TEST := $(BUILD_DIR)/application-service-runtime-test
APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_TEST := $(BUILD_DIR)/application-service-runtime-adversarial-test
APPLICATION_SERVICE_RUNTIME_SANITIZE_TEST := $(BUILD_DIR)/application-service-runtime-sanitize-test
APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/application-service-runtime-adversarial-sanitize-test
APPLICATION_SERVICE_RUNTIME_BASELINE_VERIFY := tools/application_service_runtime_baseline_verify.sh
APPLICATION_SERVICE_RUNTIME_CONTRACT_VERIFY := tools/application_service_runtime_contract_verify.sh
APPLICATION_SERVICE_RUNTIME_NATIVE_VERIFY := tools/application_service_runtime_native_verify.sh
APPLICATION_SERVICE_RUNTIME_ABI_VERIFY := tools/application_service_runtime_abi_verify.sh
APPLICATION_SERVICE_RUNTIME_SCOPE_VERIFY := tools/application_service_runtime_scope_verify.sh
APPLICATION_SERVICE_RUNTIME_REPRO_VERIFY := tools/application_service_runtime_reproducibility_verify.sh
APPLICATION_SERVICE_RUNTIME_GATE := tools/application_service_runtime_gate.sh

.PHONY: application-service-runtime-library application-service-runtime-native-test
.PHONY: application-service-runtime-adversarial-test application-service-runtime-sanitize
.PHONY: application-service-runtime-baseline-verify application-service-runtime-contract-verify
.PHONY: application-service-runtime-native-verify application-service-runtime-abi-verify
.PHONY: application-service-runtime-scope-verify application-service-runtime-reproducibility-verify
.PHONY: application-service-runtime-gate

$(APPLICATION_SERVICE_RUNTIME_BUILD_DIR):
	mkdir -p $@

$(APPLICATION_SERVICE_RUNTIME_OBJ): $(APPLICATION_SERVICE_RUNTIME_SOURCE) $(APPLICATION_SERVICE_RUNTIME_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(APPLICATION_SERVICE_RUNTIME_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(APPLICATION_SERVICE_RUNTIME_TEST_OBJ): $(APPLICATION_SERVICE_RUNTIME_TEST_SOURCE) $(APPLICATION_SERVICE_RUNTIME_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(APPLICATION_SERVICE_RUNTIME_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_OBJ): $(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_SOURCE) $(APPLICATION_SERVICE_RUNTIME_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(APPLICATION_SERVICE_RUNTIME_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(APPLICATION_SERVICE_RUNTIME_ABI_OBJ): $(APPLICATION_SERVICE_RUNTIME_ABI_SOURCE) | $(APPLICATION_SERVICE_RUNTIME_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(APPLICATION_SERVICE_RUNTIME_LIB): $(APPLICATION_SERVICE_RUNTIME_OBJ) | $(BUILD_DIR)
	$(AR) rcsD $@ $(APPLICATION_SERVICE_RUNTIME_OBJ)

application-service-runtime-library: $(APPLICATION_SERVICE_RUNTIME_LIB)

$(APPLICATION_SERVICE_RUNTIME_TEST): $(APPLICATION_SERVICE_RUNTIME_TEST_OBJ) $(APPLICATION_SERVICE_RUNTIME_ABI_OBJ) $(APPLICATION_SERVICE_RUNTIME_LIB) $(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(APPLICATION_SERVICE_RUNTIME_TEST_OBJ) $(APPLICATION_SERVICE_RUNTIME_ABI_OBJ) \
		$(APPLICATION_SERVICE_RUNTIME_LIB) $(APPLICATION_CAPABILITY_KERNEL_LIB) \
		$(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

application-service-runtime-native-test: $(APPLICATION_SERVICE_RUNTIME_TEST)
	@$(APPLICATION_SERVICE_RUNTIME_TEST)

$(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_TEST): $(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_OBJ) $(APPLICATION_SERVICE_RUNTIME_LIB) $(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_OBJ) $(APPLICATION_SERVICE_RUNTIME_LIB) \
		$(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) \
		$(C_RUNTIME_LIB) $(STATIC_LIB)

application-service-runtime-adversarial-test: $(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_TEST)
	@$(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_TEST)

$(APPLICATION_SERVICE_RUNTIME_SANITIZE_TEST): $(APPLICATION_SERVICE_RUNTIME_TEST_SOURCE) $(APPLICATION_SERVICE_RUNTIME_SOURCE) $(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(APPLICATION_SERVICE_RUNTIME_ABI_OBJ) $(STATIC_LIB) $(APPLICATION_SERVICE_RUNTIME_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(APPLICATION_SERVICE_RUNTIME_TEST_SOURCE) $(APPLICATION_SERVICE_RUNTIME_SOURCE) \
		$(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(C_RUNTIME_SOURCES) $(APPLICATION_SERVICE_RUNTIME_ABI_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_SANITIZE_TEST): $(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_SOURCE) $(APPLICATION_SERVICE_RUNTIME_SOURCE) $(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(APPLICATION_SERVICE_RUNTIME_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_SOURCE) $(APPLICATION_SERVICE_RUNTIME_SOURCE) \
		$(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

application-service-runtime-sanitize: $(APPLICATION_SERVICE_RUNTIME_SANITIZE_TEST) $(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(APPLICATION_SERVICE_RUNTIME_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(APPLICATION_SERVICE_RUNTIME_ADVERSARIAL_SANITIZE_TEST)
	@echo "PASS: AF3 ASan/UBSan service-runtime qualification"

application-service-runtime-baseline-verify: $(APPLICATION_SERVICE_RUNTIME_BASELINE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_SERVICE_RUNTIME_BASELINE_VERIFY)

application-service-runtime-contract-verify: $(APPLICATION_SERVICE_RUNTIME_CONTRACT) $(APPLICATION_SERVICE_RUNTIME_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_SERVICE_RUNTIME_CONTRACT_VERIFY)

application-service-runtime-native-verify: $(APPLICATION_SERVICE_RUNTIME_NATIVE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_SERVICE_RUNTIME_NATIVE_VERIFY)

application-service-runtime-abi-verify: $(APPLICATION_SERVICE_RUNTIME_ABI_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_SERVICE_RUNTIME_ABI_VERIFY)

application-service-runtime-scope-verify: $(APPLICATION_SERVICE_RUNTIME_SCOPE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_SERVICE_RUNTIME_SCOPE_VERIFY)

application-service-runtime-reproducibility-verify: $(APPLICATION_SERVICE_RUNTIME_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_SERVICE_RUNTIME_REPRO_VERIFY)

application-service-runtime-gate: $(APPLICATION_SERVICE_RUNTIME_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(APPLICATION_SERVICE_RUNTIME_GATE)


# ============================================================
# Application / DDD / MVC AF4 DDD Support
# ============================================================

DDD_SUPPORT_CONTRACT := application/arborcore-application-ddd-support-1.contract
DDD_SUPPORT_HEADER := include/arborcore/ddd_support.h
DDD_SUPPORT_SOURCE := src/c/ddd_support.c
DDD_SUPPORT_TEST_SOURCE := tests/c/ddd_support_test.c
DDD_SUPPORT_ADVERSARIAL_SOURCE := tests/c/ddd_support_adversarial_test.c
DDD_SUPPORT_ABI_SOURCE := tests/asm/ddd_support_abi_test.asm
DDD_SUPPORT_BUILD_DIR := $(BUILD_DIR)/ddd-support
DDD_SUPPORT_OBJ := $(DDD_SUPPORT_BUILD_DIR)/ddd_support.o
DDD_SUPPORT_TEST_OBJ := $(DDD_SUPPORT_BUILD_DIR)/ddd_support_test.o
DDD_SUPPORT_ADVERSARIAL_OBJ := $(DDD_SUPPORT_BUILD_DIR)/ddd_support_adversarial_test.o
DDD_SUPPORT_ABI_OBJ := $(DDD_SUPPORT_BUILD_DIR)/ddd_support_abi_test.o
DDD_SUPPORT_LIB := $(BUILD_DIR)/libarborcore_ddd_support.a
DDD_SUPPORT_TEST := $(BUILD_DIR)/ddd-support-test
DDD_SUPPORT_ADVERSARIAL_TEST := $(BUILD_DIR)/ddd-support-adversarial-test
DDD_SUPPORT_SANITIZE_TEST := $(BUILD_DIR)/ddd-support-sanitize-test
DDD_SUPPORT_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/ddd-support-adversarial-sanitize-test
DDD_SUPPORT_BASELINE_VERIFY := tools/ddd_support_baseline_verify.sh
DDD_SUPPORT_CONTRACT_VERIFY := tools/ddd_support_contract_verify.sh
DDD_SUPPORT_NATIVE_VERIFY := tools/ddd_support_native_verify.sh
DDD_SUPPORT_ABI_VERIFY := tools/ddd_support_abi_verify.sh
DDD_SUPPORT_SCOPE_VERIFY := tools/ddd_support_scope_verify.sh
DDD_SUPPORT_REPRO_VERIFY := tools/ddd_support_reproducibility_verify.sh
DDD_SUPPORT_GATE := tools/ddd_support_gate.sh

.PHONY: ddd-support-library ddd-support-native-test ddd-support-adversarial-test
.PHONY: ddd-support-sanitize ddd-support-baseline-verify ddd-support-contract-verify
.PHONY: ddd-support-native-verify ddd-support-abi-verify ddd-support-scope-verify
.PHONY: ddd-support-reproducibility-verify ddd-support-gate

$(DDD_SUPPORT_BUILD_DIR):
	mkdir -p $@

$(DDD_SUPPORT_OBJ): $(DDD_SUPPORT_SOURCE) $(DDD_SUPPORT_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(DDD_SUPPORT_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(DDD_SUPPORT_TEST_OBJ): $(DDD_SUPPORT_TEST_SOURCE) $(DDD_SUPPORT_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(DDD_SUPPORT_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(DDD_SUPPORT_ADVERSARIAL_OBJ): $(DDD_SUPPORT_ADVERSARIAL_SOURCE) $(DDD_SUPPORT_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(DDD_SUPPORT_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(DDD_SUPPORT_ABI_OBJ): $(DDD_SUPPORT_ABI_SOURCE) | $(DDD_SUPPORT_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(DDD_SUPPORT_LIB): $(DDD_SUPPORT_OBJ) | $(BUILD_DIR)
	$(AR) rcsD $@ $(DDD_SUPPORT_OBJ)

ddd-support-library: $(DDD_SUPPORT_LIB)

$(DDD_SUPPORT_TEST): $(DDD_SUPPORT_TEST_OBJ) $(DDD_SUPPORT_ABI_OBJ) $(DDD_SUPPORT_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(DDD_SUPPORT_TEST_OBJ) $(DDD_SUPPORT_ABI_OBJ) $(DDD_SUPPORT_LIB) \
		$(C_RUNTIME_LIB) $(STATIC_LIB)

ddd-support-native-test: $(DDD_SUPPORT_TEST)
	@$(DDD_SUPPORT_TEST)

$(DDD_SUPPORT_ADVERSARIAL_TEST): $(DDD_SUPPORT_ADVERSARIAL_OBJ) $(DDD_SUPPORT_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(DDD_SUPPORT_ADVERSARIAL_OBJ) $(DDD_SUPPORT_LIB) \
		$(C_RUNTIME_LIB) $(STATIC_LIB)

ddd-support-adversarial-test: $(DDD_SUPPORT_ADVERSARIAL_TEST)
	@$(DDD_SUPPORT_ADVERSARIAL_TEST)

$(DDD_SUPPORT_SANITIZE_TEST): $(DDD_SUPPORT_TEST_SOURCE) $(DDD_SUPPORT_SOURCE) $(C_RUNTIME_SOURCES) $(DDD_SUPPORT_ABI_OBJ) $(STATIC_LIB) $(DDD_SUPPORT_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(DDD_SUPPORT_TEST_SOURCE) $(DDD_SUPPORT_SOURCE) $(C_RUNTIME_SOURCES) \
		$(DDD_SUPPORT_ABI_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(DDD_SUPPORT_ADVERSARIAL_SANITIZE_TEST): $(DDD_SUPPORT_ADVERSARIAL_SOURCE) $(DDD_SUPPORT_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(DDD_SUPPORT_HEADER) $(APPLICATION_CAPABILITY_KERNEL_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(DDD_SUPPORT_ADVERSARIAL_SOURCE) $(DDD_SUPPORT_SOURCE) $(C_RUNTIME_SOURCES) \
		$(STATIC_LIB) $(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

ddd-support-sanitize: $(DDD_SUPPORT_SANITIZE_TEST) $(DDD_SUPPORT_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(DDD_SUPPORT_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(DDD_SUPPORT_ADVERSARIAL_SANITIZE_TEST)
	@echo "PASS: AF4 ASan/UBSan DDD-support qualification"

ddd-support-baseline-verify: $(DDD_SUPPORT_BASELINE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(DDD_SUPPORT_BASELINE_VERIFY)

ddd-support-contract-verify: $(DDD_SUPPORT_CONTRACT) $(DDD_SUPPORT_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(DDD_SUPPORT_CONTRACT_VERIFY)

ddd-support-native-verify: $(DDD_SUPPORT_NATIVE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(DDD_SUPPORT_NATIVE_VERIFY)

ddd-support-abi-verify: $(DDD_SUPPORT_ABI_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(DDD_SUPPORT_ABI_VERIFY)

ddd-support-scope-verify: $(DDD_SUPPORT_SCOPE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(DDD_SUPPORT_SCOPE_VERIFY)

ddd-support-reproducibility-verify: $(DDD_SUPPORT_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(DDD_SUPPORT_REPRO_VERIFY)

ddd-support-gate: $(DDD_SUPPORT_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(DDD_SUPPORT_GATE)
# ============================================================
# MVC0 Core MVC + Parallel Rich Application Transport
# ============================================================

MVC0_CONTRACT := mvc/arborcore-mvc-core-transport-1.contract
MVC0_HEADER := include/arborcore/mvc.h
MVC0_TRANSPORT_HEADER := include/arborcore/application_transport.h
MVC0_ASM_SOURCE := src/asm/application_transport.asm
MVC0_TRANSPORT_C_SOURCE := src/c/application_transport.c
MVC0_C_SOURCE := src/c/mvc.c
MVC0_ABI_SOURCE := tests/asm/mvc0_abi_test.asm
MVC0_CORE_TEST_SOURCE := tests/c/mvc_core_test.c
MVC0_ADVERSARIAL_TEST_SOURCE := tests/c/mvc_adversarial_test.c
MVC0_INTEGRATION_TEST_SOURCE := tests/c/mvc_integration_test.c
MVC0_END_TO_END_TEST_SOURCE := tests/c/mvc_end_to_end_test.c
MVC0_BUILD_DIR := $(BUILD_DIR)/mvc0
MVC0_ASM_OBJ := $(MVC0_BUILD_DIR)/application_transport.o
MVC0_TRANSPORT_C_OBJ := $(MVC0_BUILD_DIR)/application_transport_c.o
MVC0_C_OBJ := $(MVC0_BUILD_DIR)/mvc.o
MVC0_ABI_OBJ := $(MVC0_BUILD_DIR)/mvc0_abi_test.o
MVC0_CORE_TEST_OBJ := $(MVC0_BUILD_DIR)/mvc_core_test.o
MVC0_ADVERSARIAL_TEST_OBJ := $(MVC0_BUILD_DIR)/mvc_adversarial_test.o
MVC0_INTEGRATION_TEST_OBJ := $(MVC0_BUILD_DIR)/mvc_integration_test.o
MVC0_END_TO_END_TEST_OBJ := $(MVC0_BUILD_DIR)/mvc_end_to_end_test.o
MVC0_LIB := $(BUILD_DIR)/libarborcore_mvc0.a
MVC0_CORE_TEST := $(BUILD_DIR)/mvc0-core-test
MVC0_ADVERSARIAL_TEST := $(BUILD_DIR)/mvc0-adversarial-test
MVC0_INTEGRATION_TEST := $(BUILD_DIR)/mvc0-integration-test
MVC0_END_TO_END_TEST := $(BUILD_DIR)/mvc0-end-to-end-test
MVC0_CORE_SANITIZE_TEST := $(BUILD_DIR)/mvc0-core-sanitize-test
MVC0_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/mvc0-adversarial-sanitize-test
MVC0_INTEGRATION_SANITIZE_TEST := $(BUILD_DIR)/mvc0-integration-sanitize-test
MVC0_END_TO_END_SANITIZE_TEST := $(BUILD_DIR)/mvc0-end-to-end-sanitize-test
MVC0_BASELINE_VERIFY := tools/mvc0_baseline_verify.sh
MVC0_CONTRACT_VERIFY := tools/mvc0_contract_verify.sh
MVC0_NATIVE_VERIFY := tools/mvc0_native_verify.sh
MVC0_ABI_VERIFY := tools/mvc0_abi_verify.sh
MVC0_SCOPE_VERIFY := tools/mvc0_scope_verify.sh
MVC0_REPRO_VERIFY := tools/mvc0_reproducibility_verify.sh
MVC0_GATE := tools/mvc0_gate.sh

.PHONY: mvc0-library mvc0-core-test mvc0-adversarial-test mvc0-integration-test mvc0-end-to-end-test
.PHONY: mvc0-sanitize mvc0-baseline-verify mvc0-contract-verify mvc0-native-verify
.PHONY: mvc0-abi-verify mvc0-scope-verify mvc0-reproducibility-verify mvc0-gate

$(MVC0_BUILD_DIR):
	mkdir -p $@

$(MVC0_ASM_OBJ): $(MVC0_ASM_SOURCE) | $(MVC0_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(MVC0_TRANSPORT_C_OBJ): $(MVC0_TRANSPORT_C_SOURCE) $(MVC0_TRANSPORT_HEADER) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(MVC0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(MVC0_C_OBJ): $(MVC0_C_SOURCE) $(MVC0_HEADER) $(APPLICATION_FOUNDATION_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(MVC0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(MVC0_ABI_OBJ): $(MVC0_ABI_SOURCE) | $(MVC0_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(MVC0_CORE_TEST_OBJ): $(MVC0_CORE_TEST_SOURCE) $(MVC0_HEADER) | $(MVC0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(MVC0_ADVERSARIAL_TEST_OBJ): $(MVC0_ADVERSARIAL_TEST_SOURCE) $(MVC0_HEADER) | $(MVC0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(MVC0_INTEGRATION_TEST_OBJ): $(MVC0_INTEGRATION_TEST_SOURCE) $(MVC0_HEADER) $(APPLICATION_SERVICE_RUNTIME_HEADER) $(DDD_SUPPORT_HEADER) | $(MVC0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(MVC0_END_TO_END_TEST_OBJ): $(MVC0_END_TO_END_TEST_SOURCE) $(MVC0_HEADER) $(MVC0_TRANSPORT_HEADER) $(APPLICATION_SERVICE_RUNTIME_HEADER) $(DDD_SUPPORT_HEADER) | $(MVC0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(MVC0_LIB): $(MVC0_ASM_OBJ) $(MVC0_TRANSPORT_C_OBJ) $(MVC0_C_OBJ) | $(BUILD_DIR)
	$(AR) rcsD $@ $(MVC0_ASM_OBJ) $(MVC0_TRANSPORT_C_OBJ) $(MVC0_C_OBJ)

mvc0-library: $(MVC0_LIB)

$(MVC0_CORE_TEST): $(MVC0_CORE_TEST_OBJ) $(MVC0_ABI_OBJ) $(MVC0_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(MVC0_CORE_TEST_OBJ) $(MVC0_ABI_OBJ) $(MVC0_LIB) \
		$(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

mvc0-core-test: $(MVC0_CORE_TEST)
	@$(MVC0_CORE_TEST)

$(MVC0_ADVERSARIAL_TEST): $(MVC0_ADVERSARIAL_TEST_OBJ) $(MVC0_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(MVC0_ADVERSARIAL_TEST_OBJ) $(MVC0_LIB) \
		$(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

mvc0-adversarial-test: $(MVC0_ADVERSARIAL_TEST)
	@$(MVC0_ADVERSARIAL_TEST)

$(MVC0_INTEGRATION_TEST): $(MVC0_INTEGRATION_TEST_OBJ) $(MVC0_LIB) $(APPLICATION_SERVICE_RUNTIME_LIB) $(DDD_SUPPORT_LIB) $(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(MVC0_INTEGRATION_TEST_OBJ) $(MVC0_LIB) \
		$(APPLICATION_SERVICE_RUNTIME_LIB) $(DDD_SUPPORT_LIB) \
		$(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) \
		$(C_RUNTIME_LIB) $(STATIC_LIB)

mvc0-integration-test: $(MVC0_INTEGRATION_TEST)
	@$(MVC0_INTEGRATION_TEST)

$(MVC0_END_TO_END_TEST): $(MVC0_END_TO_END_TEST_OBJ) $(MVC0_ABI_OBJ) $(MVC0_LIB) $(APPLICATION_SERVICE_RUNTIME_LIB) $(DDD_SUPPORT_LIB) $(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(MVC0_END_TO_END_TEST_OBJ) $(MVC0_ABI_OBJ) $(MVC0_LIB) \
		$(APPLICATION_SERVICE_RUNTIME_LIB) $(DDD_SUPPORT_LIB) \
		$(APPLICATION_CAPABILITY_KERNEL_LIB) $(APPLICATION_FOUNDATION_LIB) \
		$(C_RUNTIME_LIB) $(STATIC_LIB)

mvc0-end-to-end-test: $(MVC0_END_TO_END_TEST)
	@$(MVC0_END_TO_END_TEST)

$(MVC0_CORE_SANITIZE_TEST): $(MVC0_CORE_TEST_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(MVC0_ABI_OBJ) $(STATIC_LIB) $(MVC0_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(MVC0_CORE_TEST_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(C_RUNTIME_SOURCES) $(MVC0_ABI_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(MVC0_ADVERSARIAL_SANITIZE_TEST): $(MVC0_ADVERSARIAL_TEST_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(MVC0_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(MVC0_ADVERSARIAL_TEST_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(MVC0_INTEGRATION_SANITIZE_TEST): $(MVC0_INTEGRATION_TEST_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_SERVICE_RUNTIME_SOURCE) $(DDD_SUPPORT_SOURCE) $(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(MVC0_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(MVC0_INTEGRATION_TEST_SOURCE) $(MVC0_C_SOURCE) \
		$(APPLICATION_SERVICE_RUNTIME_SOURCE) $(DDD_SUPPORT_SOURCE) \
		$(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(MVC0_END_TO_END_SANITIZE_TEST): $(MVC0_END_TO_END_TEST_SOURCE) $(MVC0_TRANSPORT_C_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_SERVICE_RUNTIME_SOURCE) $(DDD_SUPPORT_SOURCE) $(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(C_RUNTIME_SOURCES) $(MVC0_ASM_OBJ) $(MVC0_ABI_OBJ) $(STATIC_LIB) $(MVC0_HEADER) $(MVC0_TRANSPORT_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(MVC0_END_TO_END_TEST_SOURCE) $(MVC0_TRANSPORT_C_SOURCE) $(MVC0_C_SOURCE) \
		$(APPLICATION_SERVICE_RUNTIME_SOURCE) $(DDD_SUPPORT_SOURCE) \
		$(APPLICATION_CAPABILITY_KERNEL_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(C_RUNTIME_SOURCES) $(MVC0_ASM_OBJ) $(MVC0_ABI_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

mvc0-sanitize: $(MVC0_CORE_SANITIZE_TEST) $(MVC0_ADVERSARIAL_SANITIZE_TEST) $(MVC0_INTEGRATION_SANITIZE_TEST) $(MVC0_END_TO_END_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(MVC0_CORE_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(MVC0_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(MVC0_INTEGRATION_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(MVC0_END_TO_END_SANITIZE_TEST)
	@echo "PASS: MVC0 ASan/UBSan core, integration and rich-transport qualification"

mvc0-baseline-verify: $(MVC0_BASELINE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(MVC0_BASELINE_VERIFY)

mvc0-contract-verify: $(MVC0_CONTRACT) $(MVC0_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(MVC0_CONTRACT_VERIFY)

mvc0-native-verify: $(MVC0_NATIVE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(MVC0_NATIVE_VERIFY)

mvc0-abi-verify: $(MVC0_ABI_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(MVC0_ABI_VERIFY)

mvc0-scope-verify: $(MVC0_SCOPE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(MVC0_SCOPE_VERIFY)

mvc0-reproducibility-verify: $(MVC0_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(MVC0_REPRO_VERIFY)

mvc0-gate: $(MVC0_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(MVC0_GATE)

# ============================================================
# HTTP0 Message Metadata + Final-Response Semantics
# ============================================================

HTTP0_CONTRACT := http/arborcore-http-message-semantics-1.contract
HTTP0_HEADER := include/arborcore/http.h
HTTP0_C_SOURCE := src/c/http.c
HTTP0_HEADER_ASM_SOURCE := src/asm/http_header.asm
HTTP0_RESPONSE_ASM_SOURCE := src/asm/http_response_v2.asm
HTTP0_ABI_SOURCE := tests/asm/http0_abi_test.asm
HTTP0_HEADER_TEST_SOURCE := tests/c/http0_header_test.c
HTTP0_RESPONSE_TEST_SOURCE := tests/c/http0_response_test.c
HTTP0_ADVERSARIAL_TEST_SOURCE := tests/c/http0_adversarial_test.c
HTTP0_INTEGRATION_TEST_SOURCE := tests/c/http0_integration_test.c
HTTP0_BENCH_SOURCE := bench/http0_response_bench.c
HTTP0_BUILD_DIR := $(BUILD_DIR)/http0
HTTP0_C_OBJ := $(HTTP0_BUILD_DIR)/http.o
HTTP0_HEADER_ASM_OBJ := $(HTTP0_BUILD_DIR)/http_header.o
HTTP0_RESPONSE_ASM_OBJ := $(HTTP0_BUILD_DIR)/http_response_v2.o
HTTP0_ABI_OBJ := $(HTTP0_BUILD_DIR)/http0_abi_test.o
HTTP0_HEADER_TEST_OBJ := $(HTTP0_BUILD_DIR)/http0_header_test.o
HTTP0_RESPONSE_TEST_OBJ := $(HTTP0_BUILD_DIR)/http0_response_test.o
HTTP0_ADVERSARIAL_TEST_OBJ := $(HTTP0_BUILD_DIR)/http0_adversarial_test.o
HTTP0_INTEGRATION_TEST_OBJ := $(HTTP0_BUILD_DIR)/http0_integration_test.o
HTTP0_BENCH_OBJ := $(HTTP0_BUILD_DIR)/http0_response_bench.o
HTTP0_LIB := $(BUILD_DIR)/libarborcore_http0.a
HTTP0_HEADER_TEST := $(BUILD_DIR)/http0-header-test
HTTP0_RESPONSE_TEST := $(BUILD_DIR)/http0-response-test
HTTP0_ADVERSARIAL_TEST := $(BUILD_DIR)/http0-adversarial-test
HTTP0_INTEGRATION_TEST := $(BUILD_DIR)/http0-integration-test
HTTP0_HEADER_SANITIZE_TEST := $(BUILD_DIR)/http0-header-sanitize-test
HTTP0_RESPONSE_SANITIZE_TEST := $(BUILD_DIR)/http0-response-sanitize-test
HTTP0_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/http0-adversarial-sanitize-test
HTTP0_INTEGRATION_SANITIZE_TEST := $(BUILD_DIR)/http0-integration-sanitize-test
HTTP0_BENCH := $(BUILD_DIR)/http0-response-bench
HTTP0_BASELINE_VERIFY := tools/http0_baseline_verify.sh
HTTP0_CONTRACT_VERIFY := tools/http0_contract_verify.sh
HTTP0_NATIVE_VERIFY := tools/http0_native_verify.sh
HTTP0_ABI_VERIFY := tools/http0_abi_verify.sh
HTTP0_SCOPE_VERIFY := tools/http0_scope_verify.sh
HTTP0_REPRO_VERIFY := tools/http0_reproducibility_verify.sh
HTTP0_BENCH_RUN := tools/http0_benchmark_run.sh
HTTP0_GATE := tools/http0_gate.sh

.PHONY: http0-library http0-header-test http0-response-test http0-adversarial-test http0-integration-test
.PHONY: http0-sanitize http0-baseline-verify http0-contract-verify http0-native-verify http0-abi-verify
.PHONY: http0-scope-verify http0-reproducibility-verify http0-benchmark-run http0-gate

$(HTTP0_BUILD_DIR):
	mkdir -p $@

$(HTTP0_C_OBJ): $(HTTP0_C_SOURCE) $(HTTP0_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(HTTP0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP0_HEADER_ASM_OBJ): $(HTTP0_HEADER_ASM_SOURCE) | $(HTTP0_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(HTTP0_RESPONSE_ASM_OBJ): $(HTTP0_RESPONSE_ASM_SOURCE) | $(HTTP0_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(HTTP0_ABI_OBJ): $(HTTP0_ABI_SOURCE) | $(HTTP0_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(HTTP0_HEADER_TEST_OBJ): $(HTTP0_HEADER_TEST_SOURCE) $(HTTP0_HEADER) | $(HTTP0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP0_RESPONSE_TEST_OBJ): $(HTTP0_RESPONSE_TEST_SOURCE) $(HTTP0_HEADER) | $(HTTP0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP0_ADVERSARIAL_TEST_OBJ): $(HTTP0_ADVERSARIAL_TEST_SOURCE) $(HTTP0_HEADER) | $(HTTP0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP0_INTEGRATION_TEST_OBJ): $(HTTP0_INTEGRATION_TEST_SOURCE) $(HTTP0_HEADER) | $(HTTP0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP0_BENCH_OBJ): $(HTTP0_BENCH_SOURCE) $(HTTP0_HEADER) | $(HTTP0_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP0_LIB): $(HTTP0_C_OBJ) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) | $(BUILD_DIR)
	$(AR) rcsD $@ $(HTTP0_C_OBJ) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ)

http0-library: $(HTTP0_LIB)

$(HTTP0_HEADER_TEST): $(HTTP0_HEADER_TEST_OBJ) $(HTTP0_ABI_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(HTTP0_HEADER_TEST_OBJ) $(HTTP0_ABI_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
http0-header-test: $(HTTP0_HEADER_TEST)
	@$(HTTP0_HEADER_TEST)

$(HTTP0_RESPONSE_TEST): $(HTTP0_RESPONSE_TEST_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(HTTP0_RESPONSE_TEST_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
http0-response-test: $(HTTP0_RESPONSE_TEST)
	@$(HTTP0_RESPONSE_TEST)

$(HTTP0_ADVERSARIAL_TEST): $(HTTP0_ADVERSARIAL_TEST_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(HTTP0_ADVERSARIAL_TEST_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
http0-adversarial-test: $(HTTP0_ADVERSARIAL_TEST)
	@$(HTTP0_ADVERSARIAL_TEST)

$(HTTP0_INTEGRATION_TEST): $(HTTP0_INTEGRATION_TEST_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(HTTP0_INTEGRATION_TEST_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
http0-integration-test: $(HTTP0_INTEGRATION_TEST)
	@$(HTTP0_INTEGRATION_TEST)

$(HTTP0_BENCH): $(HTTP0_BENCH_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(HTTP0_BENCH_OBJ) $(HTTP0_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

$(HTTP0_HEADER_SANITIZE_TEST): $(HTTP0_HEADER_TEST_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(HTTP0_ABI_OBJ) $(STATIC_LIB) $(HTTP0_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
		$(HTTP0_HEADER_TEST_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) \
		$(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(HTTP0_ABI_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(HTTP0_RESPONSE_SANITIZE_TEST): $(HTTP0_RESPONSE_TEST_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) $(HTTP0_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
		$(HTTP0_RESPONSE_TEST_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) \
		$(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(HTTP0_ADVERSARIAL_SANITIZE_TEST): $(HTTP0_ADVERSARIAL_TEST_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) $(HTTP0_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
		$(HTTP0_ADVERSARIAL_TEST_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) \
		$(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(HTTP0_INTEGRATION_SANITIZE_TEST): $(HTTP0_INTEGRATION_TEST_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) $(HTTP0_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
		$(HTTP0_INTEGRATION_TEST_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) \
		$(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

http0-sanitize: $(HTTP0_HEADER_SANITIZE_TEST) $(HTTP0_RESPONSE_SANITIZE_TEST) $(HTTP0_ADVERSARIAL_SANITIZE_TEST) $(HTTP0_INTEGRATION_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(HTTP0_HEADER_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(HTTP0_RESPONSE_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(HTTP0_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(HTTP0_INTEGRATION_SANITIZE_TEST)
	@echo "PASS: HTTP0 ASan/UBSan header, response, adversarial and integration qualification"

http0-baseline-verify: $(HTTP0_BASELINE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP0_BASELINE_VERIFY)
http0-contract-verify: $(HTTP0_CONTRACT) $(HTTP0_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP0_CONTRACT_VERIFY)
http0-native-verify: $(HTTP0_NATIVE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP0_NATIVE_VERIFY)
http0-abi-verify: $(HTTP0_ABI_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP0_ABI_VERIFY)
http0-scope-verify: $(HTTP0_SCOPE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP0_SCOPE_VERIFY)
http0-reproducibility-verify: $(HTTP0_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP0_REPRO_VERIFY)
http0-benchmark-run: $(HTTP0_BENCH_RUN)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP0_BENCH_RUN)
http0-gate: $(HTTP0_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP0_GATE)

# ============================================================
# HTTP1 MVC Presentation Adapter
# ============================================================

HTTP1_CONTRACT := http/arborcore-http-mvc-adapter-1.contract
HTTP1_HEADER := include/arborcore/http_mvc.h
HTTP1_C_SOURCE := src/c/http_mvc.c
HTTP1_ABI_SOURCE := tests/asm/http1_abi_test.asm
HTTP1_CORE_TEST_SOURCE := tests/c/http1_core_test.c
HTTP1_ADVERSARIAL_TEST_SOURCE := tests/c/http1_adversarial_test.c
HTTP1_INTEGRATION_TEST_SOURCE := tests/c/http1_integration_test.c
HTTP1_SOCKET_TEST_SOURCE := tests/c/http1_socket_test.c
HTTP1_BENCH_SOURCE := bench/http1_adapter_bench.c
HTTP1_BUILD_DIR := $(BUILD_DIR)/http1
HTTP1_C_OBJ := $(HTTP1_BUILD_DIR)/http_mvc.o
HTTP1_ABI_OBJ := $(HTTP1_BUILD_DIR)/http1_abi_test.o
HTTP1_CORE_TEST_OBJ := $(HTTP1_BUILD_DIR)/http1_core_test.o
HTTP1_ADVERSARIAL_TEST_OBJ := $(HTTP1_BUILD_DIR)/http1_adversarial_test.o
HTTP1_INTEGRATION_TEST_OBJ := $(HTTP1_BUILD_DIR)/http1_integration_test.o
HTTP1_SOCKET_TEST_OBJ := $(HTTP1_BUILD_DIR)/http1_socket_test.o
HTTP1_BENCH_OBJ := $(HTTP1_BUILD_DIR)/http1_adapter_bench.o
HTTP1_LIB := $(BUILD_DIR)/libarborcore_http1.a
HTTP1_CORE_TEST := $(BUILD_DIR)/http1-core-test
HTTP1_ADVERSARIAL_TEST := $(BUILD_DIR)/http1-adversarial-test
HTTP1_INTEGRATION_TEST := $(BUILD_DIR)/http1-integration-test
HTTP1_SOCKET_TEST := $(BUILD_DIR)/http1-socket-test
HTTP1_CORE_SANITIZE_TEST := $(BUILD_DIR)/http1-core-sanitize-test
HTTP1_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/http1-adversarial-sanitize-test
HTTP1_INTEGRATION_SANITIZE_TEST := $(BUILD_DIR)/http1-integration-sanitize-test
HTTP1_SOCKET_SANITIZE_TEST := $(BUILD_DIR)/http1-socket-sanitize-test
HTTP1_BENCH := $(BUILD_DIR)/http1-adapter-bench
HTTP1_AF1_VERIFY := tools/http1_af1_retrofit_verify.sh
HTTP1_BASELINE_VERIFY := tools/http1_baseline_verify.sh
HTTP1_CONTRACT_VERIFY := tools/http1_contract_verify.sh
HTTP1_NATIVE_VERIFY := tools/http1_native_verify.sh
HTTP1_ABI_VERIFY := tools/http1_abi_verify.sh
HTTP1_SCOPE_VERIFY := tools/http1_scope_verify.sh
HTTP1_REPRO_VERIFY := tools/http1_reproducibility_verify.sh
HTTP1_BENCH_RUN := tools/http1_benchmark_run.sh
HTTP1_GATE := tools/http1_gate.sh

.PHONY: http1-library http1-core-test http1-adversarial-test http1-integration-test http1-socket-test
.PHONY: http1-sanitize http1-af1-retrofit-verify http1-baseline-verify http1-contract-verify
.PHONY: http1-native-verify http1-abi-verify http1-scope-verify http1-reproducibility-verify
.PHONY: http1-benchmark-run http1-gate

$(HTTP1_BUILD_DIR):
	mkdir -p $@

$(HTTP1_C_OBJ): $(HTTP1_C_SOURCE) $(HTTP1_HEADER) $(MVC0_HEADER) $(HTTP0_HEADER) | $(HTTP1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP1_ABI_OBJ): $(HTTP1_ABI_SOURCE) | $(HTTP1_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(HTTP1_CORE_TEST_OBJ): $(HTTP1_CORE_TEST_SOURCE) $(HTTP1_HEADER) | $(HTTP1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP1_ADVERSARIAL_TEST_OBJ): $(HTTP1_ADVERSARIAL_TEST_SOURCE) $(HTTP1_HEADER) | $(HTTP1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP1_INTEGRATION_TEST_OBJ): $(HTTP1_INTEGRATION_TEST_SOURCE) $(HTTP1_HEADER) | $(HTTP1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP1_SOCKET_TEST_OBJ): $(HTTP1_SOCKET_TEST_SOURCE) $(HTTP1_HEADER) | $(HTTP1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP1_BENCH_OBJ): $(HTTP1_BENCH_SOURCE) $(HTTP1_HEADER) | $(HTTP1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(HTTP1_LIB): $(HTTP1_C_OBJ) | $(BUILD_DIR)
	$(AR) rcsD $@ $(HTTP1_C_OBJ)

http1-library: $(HTTP1_LIB)

$(HTTP1_CORE_TEST): $(HTTP1_CORE_TEST_OBJ) $(HTTP1_ABI_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(HTTP1_CORE_TEST_OBJ) $(HTTP1_ABI_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) \
		$(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
http1-core-test: $(HTTP1_CORE_TEST)
	@$(HTTP1_CORE_TEST)

$(HTTP1_ADVERSARIAL_TEST): $(HTTP1_ADVERSARIAL_TEST_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(HTTP1_ADVERSARIAL_TEST_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) \
		$(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
http1-adversarial-test: $(HTTP1_ADVERSARIAL_TEST)
	@$(HTTP1_ADVERSARIAL_TEST)

$(HTTP1_INTEGRATION_TEST): $(HTTP1_INTEGRATION_TEST_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(HTTP1_INTEGRATION_TEST_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) \
		$(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
http1-integration-test: $(HTTP1_INTEGRATION_TEST)
	@$(HTTP1_INTEGRATION_TEST)

$(HTTP1_SOCKET_TEST): $(HTTP1_SOCKET_TEST_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(HTTP1_SOCKET_TEST_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) \
		$(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
http1-socket-test: $(HTTP1_SOCKET_TEST)
	@$(HTTP1_SOCKET_TEST)

HTTP1_SANITIZE_COMMON := \
	$(HTTP1_C_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(HTTP0_C_SOURCE) \
	$(C_RUNTIME_SOURCES) $(MVC0_ASM_OBJ) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB)

$(HTTP1_CORE_SANITIZE_TEST): $(HTTP1_CORE_TEST_SOURCE) $(HTTP1_ABI_OBJ) $(HTTP1_SANITIZE_COMMON) $(HTTP1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(HTTP1_CORE_TEST_SOURCE) $(HTTP1_C_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(HTTP1_ABI_OBJ) $(MVC0_ASM_OBJ) \
		$(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(HTTP1_ADVERSARIAL_SANITIZE_TEST): $(HTTP1_ADVERSARIAL_TEST_SOURCE) $(HTTP1_SANITIZE_COMMON) $(HTTP1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(HTTP1_ADVERSARIAL_TEST_SOURCE) $(HTTP1_C_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(MVC0_ASM_OBJ) $(HTTP0_HEADER_ASM_OBJ) \
		$(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) $(ARBORCORE_C_LDFLAGS) \
		-fsanitize=address,undefined -o $@

$(HTTP1_INTEGRATION_SANITIZE_TEST): $(HTTP1_INTEGRATION_TEST_SOURCE) $(HTTP1_SANITIZE_COMMON) $(HTTP1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(HTTP1_INTEGRATION_TEST_SOURCE) $(HTTP1_C_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(MVC0_ASM_OBJ) $(HTTP0_HEADER_ASM_OBJ) \
		$(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) $(ARBORCORE_C_LDFLAGS) \
		-fsanitize=address,undefined -o $@

$(HTTP1_SOCKET_SANITIZE_TEST): $(HTTP1_SOCKET_TEST_SOURCE) $(HTTP1_SANITIZE_COMMON) $(HTTP1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(HTTP1_SOCKET_TEST_SOURCE) $(HTTP1_C_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) \
		$(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(MVC0_ASM_OBJ) $(HTTP0_HEADER_ASM_OBJ) \
		$(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) $(ARBORCORE_C_LDFLAGS) \
		-fsanitize=address,undefined -o $@

http1-sanitize: $(HTTP1_CORE_SANITIZE_TEST) $(HTTP1_ADVERSARIAL_SANITIZE_TEST) $(HTTP1_INTEGRATION_SANITIZE_TEST) $(HTTP1_SOCKET_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(HTTP1_CORE_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(HTTP1_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(HTTP1_INTEGRATION_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(HTTP1_SOCKET_SANITIZE_TEST)
	@echo "PASS: HTTP1 ASan/UBSan core, adversarial, MVC/HTTP integration and socket qualification"

$(HTTP1_BENCH): $(HTTP1_BENCH_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(HTTP1_BENCH_OBJ) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) \
		$(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

http1-af1-retrofit-verify: $(HTTP1_AF1_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_AF1_VERIFY)
http1-baseline-verify: $(HTTP1_BASELINE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_BASELINE_VERIFY)
http1-contract-verify: $(HTTP1_CONTRACT) $(HTTP1_CONTRACT_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_CONTRACT_VERIFY)
http1-native-verify: $(HTTP1_NATIVE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_NATIVE_VERIFY)
http1-abi-verify: $(HTTP1_ABI_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_ABI_VERIFY)
http1-scope-verify: $(HTTP1_SCOPE_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_SCOPE_VERIFY)
http1-reproducibility-verify: $(HTTP1_REPRO_VERIFY)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_REPRO_VERIFY)
http1-benchmark-run: $(HTTP1_BENCH_RUN)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_BENCH_RUN)
http1-gate: $(HTTP1_GATE)
	@ARBORCORE_ROOT=$(ROOT_DIR) bash $(HTTP1_GATE)
# ============================================================
# VIEW0 C1 — bounded view output primitive
# ============================================================
VIEW0_C1_CONTRACT := view/arborcore-view-core-1.contract
VIEW0_C1_HEADER := include/arborcore/view.h
VIEW0_C1_SOURCE := src/c/view.c
VIEW0_C1_TEST_SOURCE := tests/c/view0_c1_test.c
VIEW0_C1_ADVERSARIAL_SOURCE := tests/c/view0_c1_adversarial_test.c
VIEW0_C1_BUILD_DIR := $(BUILD_DIR)/view0-c1
VIEW0_C1_OBJ := $(VIEW0_C1_BUILD_DIR)/view.o
VIEW0_C1_TEST_OBJ := $(VIEW0_C1_BUILD_DIR)/view0_c1_test.o
VIEW0_C1_ADVERSARIAL_OBJ := $(VIEW0_C1_BUILD_DIR)/view0_c1_adversarial_test.o
VIEW0_C1_LIB := $(BUILD_DIR)/libarborcore_view.a
VIEW0_C1_TEST := $(BUILD_DIR)/view0-c1-test
VIEW0_C1_ADVERSARIAL_TEST := $(BUILD_DIR)/view0-c1-adversarial-test
VIEW0_C1_SANITIZE_TEST := $(BUILD_DIR)/view0-c1-sanitize-test
VIEW0_C1_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/view0-c1-adversarial-sanitize-test

.PHONY: view0-c1-library view0-c1-core-test view0-c1-adversarial-test view0-c1-sanitize
.PHONY: view0-c1-gate

$(VIEW0_C1_BUILD_DIR):
	mkdir -p $@

$(VIEW0_C1_OBJ): $(VIEW0_C1_SOURCE) $(VIEW0_C1_HEADER) include/arborcore/arborcore.h include/arborcore/assembly_abi.h | $(VIEW0_C1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C1_TEST_OBJ): $(VIEW0_C1_TEST_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_C1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C1_ADVERSARIAL_OBJ): $(VIEW0_C1_ADVERSARIAL_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_C1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C1_LIB): $(VIEW0_C1_OBJ) | $(BUILD_DIR)
	$(AR) rcsD $@ $(VIEW0_C1_OBJ)

view0-c1-library: $(VIEW0_C1_LIB)

$(VIEW0_C1_TEST): $(VIEW0_C1_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_C1_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-c1-core-test: $(VIEW0_C1_TEST)
	@$(VIEW0_C1_TEST)

$(VIEW0_C1_ADVERSARIAL_TEST): $(VIEW0_C1_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_C1_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-c1-adversarial-test: $(VIEW0_C1_ADVERSARIAL_TEST)
	@$(VIEW0_C1_ADVERSARIAL_TEST)

$(VIEW0_C1_SANITIZE_TEST): $(VIEW0_C1_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_C1_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(VIEW0_C1_ADVERSARIAL_SANITIZE_TEST): $(VIEW0_C1_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_C1_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

view0-c1-sanitize: $(VIEW0_C1_SANITIZE_TEST) $(VIEW0_C1_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_C1_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_C1_ADVERSARIAL_SANITIZE_TEST)
	@echo "PASS: VIEW0 C1 ASan/UBSan core and adversarial qualification"

view0-c1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_c1_gate.sh
# ============================================================
# VIEW0 C2 — ordinary HTML Data/text-context escaping
# ============================================================
VIEW0_C2_TEST_SOURCE := tests/c/view0_c2_test.c
VIEW0_C2_ADVERSARIAL_SOURCE := tests/c/view0_c2_adversarial_test.c
VIEW0_C2_BUILD_DIR := $(BUILD_DIR)/view0-c2
VIEW0_C2_TEST_OBJ := $(VIEW0_C2_BUILD_DIR)/view0_c2_test.o
VIEW0_C2_ADVERSARIAL_OBJ := $(VIEW0_C2_BUILD_DIR)/view0_c2_adversarial_test.o
VIEW0_C2_TEST := $(BUILD_DIR)/view0-c2-test
VIEW0_C2_ADVERSARIAL_TEST := $(BUILD_DIR)/view0-c2-adversarial-test
VIEW0_C2_SANITIZE_TEST := $(BUILD_DIR)/view0-c2-sanitize-test
VIEW0_C2_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/view0-c2-adversarial-sanitize-test

.PHONY: view0-c2-core-test view0-c2-adversarial-test view0-c2-sanitize view0-c2-gate

$(VIEW0_C2_BUILD_DIR):
	mkdir -p $@

$(VIEW0_C2_TEST_OBJ): $(VIEW0_C2_TEST_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_C2_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C2_ADVERSARIAL_OBJ): $(VIEW0_C2_ADVERSARIAL_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_C2_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C2_TEST): $(VIEW0_C2_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_C2_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-c2-core-test: $(VIEW0_C2_TEST)
	@$(VIEW0_C2_TEST)

$(VIEW0_C2_ADVERSARIAL_TEST): $(VIEW0_C2_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_C2_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-c2-adversarial-test: $(VIEW0_C2_ADVERSARIAL_TEST)
	@$(VIEW0_C2_ADVERSARIAL_TEST)

$(VIEW0_C2_SANITIZE_TEST): $(VIEW0_C2_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_C2_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(VIEW0_C2_ADVERSARIAL_SANITIZE_TEST): $(VIEW0_C2_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_C2_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

view0-c2-sanitize: $(VIEW0_C2_SANITIZE_TEST) $(VIEW0_C2_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_C2_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_C2_ADVERSARIAL_SANITIZE_TEST)
	@echo "PASS: VIEW0 C2 ASan/UBSan core and adversarial qualification"

view0-c2-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_c2_gate.sh

# ============================================================
# VIEW0 C3 — native C compiled-view path
# ============================================================
VIEW0_C3_TEST_SOURCE := tests/c/view0_c3_test.c
VIEW0_C3_ADVERSARIAL_SOURCE := tests/c/view0_c3_adversarial_test.c
VIEW0_C3_BUILD_DIR := $(BUILD_DIR)/view0-c3
VIEW0_C3_TEST_OBJ := $(VIEW0_C3_BUILD_DIR)/view0_c3_test.o
VIEW0_C3_ADVERSARIAL_OBJ := $(VIEW0_C3_BUILD_DIR)/view0_c3_adversarial_test.o
VIEW0_C3_TEST := $(BUILD_DIR)/view0-c3-test
VIEW0_C3_ADVERSARIAL_TEST := $(BUILD_DIR)/view0-c3-adversarial-test
VIEW0_C3_SANITIZE_TEST := $(BUILD_DIR)/view0-c3-sanitize-test
VIEW0_C3_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/view0-c3-adversarial-sanitize-test

.PHONY: view0-c3-core-test view0-c3-adversarial-test view0-c3-sanitize view0-c3-gate

$(VIEW0_C3_BUILD_DIR):
	mkdir -p $@

$(VIEW0_C3_TEST_OBJ): $(VIEW0_C3_TEST_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_C3_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C3_ADVERSARIAL_OBJ): $(VIEW0_C3_ADVERSARIAL_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_C3_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C3_TEST): $(VIEW0_C3_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_C3_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-c3-core-test: $(VIEW0_C3_TEST)
	@$(VIEW0_C3_TEST)

$(VIEW0_C3_ADVERSARIAL_TEST): $(VIEW0_C3_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_C3_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-c3-adversarial-test: $(VIEW0_C3_ADVERSARIAL_TEST)
	@$(VIEW0_C3_ADVERSARIAL_TEST)

$(VIEW0_C3_SANITIZE_TEST): $(VIEW0_C3_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_C3_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(VIEW0_C3_ADVERSARIAL_SANITIZE_TEST): $(VIEW0_C3_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_C3_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

view0-c3-sanitize: $(VIEW0_C3_SANITIZE_TEST) $(VIEW0_C3_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_C3_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_C3_ADVERSARIAL_SANITIZE_TEST)
	@echo "PASS: VIEW0 C3 ASan/UBSan typed compiled-view qualification"

view0-c3-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_c3_gate.sh

# ============================================================
# VIEW0 C4 — real NASM -> existing VIEW C API SysV qualification
# ============================================================
VIEW0_C4_ASM_SOURCE := tests/asm/view0_c4_abi_test.asm
VIEW0_C4_TEST_SOURCE := tests/c/view0_c4_test.c
VIEW0_C4_ADVERSARIAL_SOURCE := tests/c/view0_c4_adversarial_test.c
VIEW0_C4_BUILD_DIR := $(BUILD_DIR)/view0-c4
VIEW0_C4_ASM_OBJ := $(VIEW0_C4_BUILD_DIR)/view0_c4_abi_test.o
VIEW0_C4_TEST_OBJ := $(VIEW0_C4_BUILD_DIR)/view0_c4_test.o
VIEW0_C4_ADVERSARIAL_OBJ := $(VIEW0_C4_BUILD_DIR)/view0_c4_adversarial_test.o
VIEW0_C4_TEST := $(BUILD_DIR)/view0-c4-test
VIEW0_C4_ADVERSARIAL_TEST := $(BUILD_DIR)/view0-c4-adversarial-test
VIEW0_C4_SANITIZE_TEST := $(BUILD_DIR)/view0-c4-sanitize-test
VIEW0_C4_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/view0-c4-adversarial-sanitize-test

.PHONY: view0-c4-core-test view0-c4-adversarial-test view0-c4-sanitize view0-c4-gate

$(VIEW0_C4_BUILD_DIR):
	mkdir -p $@

$(VIEW0_C4_ASM_OBJ): $(VIEW0_C4_ASM_SOURCE) | $(VIEW0_C4_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(VIEW0_C4_TEST_OBJ): $(VIEW0_C4_TEST_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_C4_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C4_ADVERSARIAL_OBJ): $(VIEW0_C4_ADVERSARIAL_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_C4_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_C4_TEST): $(VIEW0_C4_TEST_OBJ) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_C4_TEST_OBJ) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-c4-core-test: $(VIEW0_C4_TEST)
	@$(VIEW0_C4_TEST)

$(VIEW0_C4_ADVERSARIAL_TEST): $(VIEW0_C4_ADVERSARIAL_OBJ) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_C4_ADVERSARIAL_OBJ) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-c4-adversarial-test: $(VIEW0_C4_ADVERSARIAL_TEST)
	@$(VIEW0_C4_ADVERSARIAL_TEST)

$(VIEW0_C4_SANITIZE_TEST): $(VIEW0_C4_TEST_SOURCE) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_C4_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(VIEW0_C4_ASM_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(VIEW0_C4_ADVERSARIAL_SANITIZE_TEST): $(VIEW0_C4_ADVERSARIAL_SOURCE) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_C4_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(VIEW0_C4_ASM_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

view0-c4-sanitize: $(VIEW0_C4_SANITIZE_TEST) $(VIEW0_C4_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_C4_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_C4_ADVERSARIAL_SANITIZE_TEST)
	@echo "PASS: VIEW0 C4 ASan/UBSan real NASM -> C VIEW qualification"

view0-c4-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_c4_gate.sh

# ============================================================
# VIEW0 T1 — minimal prepared HTML template mechanism
# ============================================================
VIEW0_T1_TEST_SOURCE := tests/c/view0_t1_test.c
VIEW0_T1_ADVERSARIAL_SOURCE := tests/c/view0_t1_adversarial_test.c
VIEW0_T1_BUILD_DIR := $(BUILD_DIR)/view0-t1
VIEW0_T1_TEST_OBJ := $(VIEW0_T1_BUILD_DIR)/view0_t1_test.o
VIEW0_T1_ADVERSARIAL_OBJ := $(VIEW0_T1_BUILD_DIR)/view0_t1_adversarial_test.o
VIEW0_T1_TEST := $(BUILD_DIR)/view0-t1-test
VIEW0_T1_ADVERSARIAL_TEST := $(BUILD_DIR)/view0-t1-adversarial-test
VIEW0_T1_SANITIZE_TEST := $(BUILD_DIR)/view0-t1-sanitize-test
VIEW0_T1_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/view0-t1-adversarial-sanitize-test

.PHONY: view0-t1-core-test view0-t1-adversarial-test view0-t1-sanitize view0-t1-gate

$(VIEW0_T1_BUILD_DIR):
	mkdir -p $@

$(VIEW0_T1_TEST_OBJ): $(VIEW0_T1_TEST_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_T1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_T1_ADVERSARIAL_OBJ): $(VIEW0_T1_ADVERSARIAL_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_T1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_T1_TEST): $(VIEW0_T1_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_T1_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-t1-core-test: $(VIEW0_T1_TEST)
	@$(VIEW0_T1_TEST)

$(VIEW0_T1_ADVERSARIAL_TEST): $(VIEW0_T1_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_T1_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-t1-adversarial-test: $(VIEW0_T1_ADVERSARIAL_TEST)
	@$(VIEW0_T1_ADVERSARIAL_TEST)

$(VIEW0_T1_SANITIZE_TEST): $(VIEW0_T1_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_T1_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(VIEW0_T1_ADVERSARIAL_SANITIZE_TEST): $(VIEW0_T1_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_T1_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

view0-t1-sanitize: $(VIEW0_T1_SANITIZE_TEST) $(VIEW0_T1_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_T1_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_T1_ADVERSARIAL_SANITIZE_TEST)
	@echo "PASS: VIEW0 T1 ASan/UBSan prepared-template qualification"

view0-t1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_t1_gate.sh

# ============================================================
# VIEW0 M1 — MVC0 presenter + HTTP1 + UTF-8 HTML integration
# ============================================================
VIEW0_M1_UTF8_TEST_SOURCE := tests/c/view0_m1_utf8_test.c
VIEW0_M1_ADVERSARIAL_SOURCE := tests/c/view0_m1_adversarial_test.c
VIEW0_M1_INTEGRATION_SOURCE := tests/c/view0_m1_integration_test.c
VIEW0_M1_BUILD_DIR := $(BUILD_DIR)/view0-m1
VIEW0_M1_UTF8_TEST_OBJ := $(VIEW0_M1_BUILD_DIR)/view0_m1_utf8_test.o
VIEW0_M1_ADVERSARIAL_OBJ := $(VIEW0_M1_BUILD_DIR)/view0_m1_adversarial_test.o
VIEW0_M1_INTEGRATION_OBJ := $(VIEW0_M1_BUILD_DIR)/view0_m1_integration_test.o
VIEW0_M1_UTF8_TEST := $(BUILD_DIR)/view0-m1-utf8-test
VIEW0_M1_ADVERSARIAL_TEST := $(BUILD_DIR)/view0-m1-adversarial-test
VIEW0_M1_INTEGRATION_TEST := $(BUILD_DIR)/view0-m1-integration-test
VIEW0_M1_UTF8_SANITIZE_TEST := $(BUILD_DIR)/view0-m1-utf8-sanitize-test
VIEW0_M1_ADVERSARIAL_SANITIZE_TEST := $(BUILD_DIR)/view0-m1-adversarial-sanitize-test
VIEW0_M1_INTEGRATION_SANITIZE_TEST := $(BUILD_DIR)/view0-m1-integration-sanitize-test

.PHONY: view0-m1-utf8-test view0-m1-adversarial-test view0-m1-integration-test view0-m1-sanitize view0-m1-gate

$(VIEW0_M1_BUILD_DIR):
	mkdir -p $@

$(VIEW0_M1_UTF8_TEST_OBJ): $(VIEW0_M1_UTF8_TEST_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_M1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_M1_ADVERSARIAL_OBJ): $(VIEW0_M1_ADVERSARIAL_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_M1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_M1_INTEGRATION_OBJ): $(VIEW0_M1_INTEGRATION_SOURCE) $(VIEW0_C1_HEADER) $(HTTP1_HEADER) $(MVC0_HEADER) | $(VIEW0_M1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_M1_UTF8_TEST): $(VIEW0_M1_UTF8_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_M1_UTF8_TEST_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-m1-utf8-test: $(VIEW0_M1_UTF8_TEST)
	@$(VIEW0_M1_UTF8_TEST)

$(VIEW0_M1_ADVERSARIAL_TEST): $(VIEW0_M1_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_M1_ADVERSARIAL_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-m1-adversarial-test: $(VIEW0_M1_ADVERSARIAL_TEST)
	@$(VIEW0_M1_ADVERSARIAL_TEST)

$(VIEW0_M1_INTEGRATION_TEST): $(VIEW0_M1_INTEGRATION_OBJ) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_LIB) $(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) $(APPLICATION_FOUNDATION_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_M1_INTEGRATION_OBJ) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_LIB) \
		$(HTTP1_LIB) $(MVC0_LIB) $(HTTP0_LIB) $(APPLICATION_FOUNDATION_LIB) \
		$(C_RUNTIME_LIB) $(STATIC_LIB)

view0-m1-integration-test: $(VIEW0_M1_INTEGRATION_TEST)
	@$(VIEW0_M1_INTEGRATION_TEST)

$(VIEW0_M1_UTF8_SANITIZE_TEST): $(VIEW0_M1_UTF8_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_M1_UTF8_TEST_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(VIEW0_M1_ADVERSARIAL_SANITIZE_TEST): $(VIEW0_M1_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_C1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_M1_ADVERSARIAL_SOURCE) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

$(VIEW0_M1_INTEGRATION_SANITIZE_TEST): $(VIEW0_M1_INTEGRATION_SOURCE) $(VIEW0_C1_SOURCE) $(HTTP1_C_SOURCE) $(MVC0_C_SOURCE) $(APPLICATION_FOUNDATION_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) $(VIEW0_C4_ASM_OBJ) $(MVC0_ASM_OBJ) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) $(VIEW0_C1_HEADER) $(HTTP1_HEADER)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_M1_INTEGRATION_SOURCE) $(VIEW0_C1_SOURCE) $(HTTP1_C_SOURCE) $(MVC0_C_SOURCE) \
		$(APPLICATION_FOUNDATION_SOURCE) $(HTTP0_C_SOURCE) $(C_RUNTIME_SOURCES) \
		$(VIEW0_C4_ASM_OBJ) $(MVC0_ASM_OBJ) $(HTTP0_HEADER_ASM_OBJ) $(HTTP0_RESPONSE_ASM_OBJ) $(STATIC_LIB) \
		$(ARBORCORE_C_LDFLAGS) -fsanitize=address,undefined -o $@

view0-m1-sanitize: $(VIEW0_M1_UTF8_SANITIZE_TEST) $(VIEW0_M1_ADVERSARIAL_SANITIZE_TEST) $(VIEW0_M1_INTEGRATION_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_M1_UTF8_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_M1_ADVERSARIAL_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_M1_INTEGRATION_SANITIZE_TEST)
	@echo "PASS: VIEW0 M1 ASan/UBSan UTF-8 and real HTTP integration qualification"

view0-m1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_m1_gate.sh

# ============================================================
# VIEW0 V1 — development-time whole-document HTML conformance
# ============================================================
VIEW0_V1_RENDER_SOURCE := tests/c/view0_v1_render_artifacts.c
VIEW0_V1_BUILD_DIR := $(BUILD_DIR)/view0-v1
VIEW0_V1_RENDER_OBJ := $(VIEW0_V1_BUILD_DIR)/view0_v1_render_artifacts.o
VIEW0_V1_RENDER_TEST := $(VIEW0_V1_BUILD_DIR)/view0_v1_render_artifacts
VIEW0_V1_DOCUMENT_DIR := $(VIEW0_V1_BUILD_DIR)/documents

.PHONY: view0-v1-render-artifacts view0-v1-conformance view0-v1-gate

$(VIEW0_V1_BUILD_DIR):
	mkdir -p $@

$(VIEW0_V1_DOCUMENT_DIR): | $(VIEW0_V1_BUILD_DIR)
	mkdir -p $@

$(VIEW0_V1_RENDER_OBJ): $(VIEW0_V1_RENDER_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_V1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1_RENDER_TEST): $(VIEW0_V1_RENDER_OBJ) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1_RENDER_OBJ) $(VIEW0_C4_ASM_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-v1-render-artifacts: $(VIEW0_V1_RENDER_TEST) | $(VIEW0_V1_DOCUMENT_DIR)
	@$(VIEW0_V1_RENDER_TEST)

view0-v1-conformance:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1_conformance_verify.sh

view0-v1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1_gate.sh

# ============================================================
# VIEW0 V1N0 — native-C HTML parse/conformance foundation
# ============================================================
VIEW0_V1N0_PRIVATE_INCLUDE := tools/include
VIEW0_V1N0_SOURCE_DIR := tools/c/view0_conformance
VIEW0_V1N0_NATIVE_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/native.c
VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/lexbor_adapter.c
VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g03_provenance.c
VIEW0_V1N1_G03_C0_PROVENANCE_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g03_provenance_internal.h
VIEW0_V1N1_G03_R1A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g03_r1a.c
VIEW0_V1N1_G03_R1A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g03_r1a.h
VIEW0_V1N1_G03_R2A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g03_r2a.c
VIEW0_V1N1_G03_R2A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g03_r2a.h
VIEW0_V1N1_G03_R3A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g03_r3a.c
VIEW0_V1N1_G03_R3A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g03_r3a.h
VIEW0_V1N1_G03_R4A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g03_r4a.c
VIEW0_V1N1_G03_R4A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g03_r4a.h
VIEW0_V1N1_G03_R5A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g03_r5a.c
VIEW0_V1N1_G03_R5A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g03_r5a.h
VIEW0_V1N1_G03_R7A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g03_r7a.c
VIEW0_V1N1_G03_R7A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g03_r7a.h
VIEW0_V1N1_G04_R1A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g04_r1a.c
VIEW0_V1N1_G04_R1A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g04_r1a.h
VIEW0_V1N1_G04_R2A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g04_r2a.c
VIEW0_V1N1_G04_R2A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g04_r2a.h
VIEW0_V1N1_G05_C0_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g05_c0.c
VIEW0_V1N1_G05_C0_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g05_c0.h
VIEW0_V1N1_G05_R1A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g05_r1a.c
VIEW0_V1N1_G05_R1A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g05_r1a.h
VIEW0_V1N1_G05_R2A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g05_r2a.c
VIEW0_V1N1_G05_R2A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g05_r2a.h
VIEW0_V1N1_G05_R3A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g05_r3a.c
VIEW0_V1N1_G05_R3A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g05_r3a.h
VIEW0_V1N1_G05_R4A_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g05_r4a.c
VIEW0_V1N1_G05_R4A_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g05_r4a.h
VIEW0_V1N1_G06_C0_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g06_c0.c
VIEW0_V1N1_G06_C0_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g06_c0.h
VIEW0_V1N1_G06_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g06.c
VIEW0_V1N1_G06_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g06.h
VIEW0_V1N2_C0_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/v1n2_c0.c
VIEW0_V1N2_C0_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/v1n2_c0.h
VIEW0_V1N2_G07_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g07.c
VIEW0_V1N2_G07_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g07.h
VIEW0_V1N2_G08_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g08.c
VIEW0_V1N2_G08_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g08.h
VIEW0_V1N2_G09_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g09.c
VIEW0_V1N2_G09_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g09.h
VIEW0_V1N2_G10_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g10.c
VIEW0_V1N2_G10_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g10.h
VIEW0_V1N2_G11_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g11.c
VIEW0_V1N2_G11_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/g11.h
VIEW0_V1N3_C0_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/v1n3_c0.c
VIEW0_V1N3_C0_INTERNAL := $(VIEW0_V1N0_SOURCE_DIR)/v1n3_c0.h
VIEW0_V1N3_ECMA_SOURCES := $(VIEW0_V1N0_SOURCE_DIR)/ecma_early_errors.c $(VIEW0_V1N0_SOURCE_DIR)/ecma_frontend.c $(VIEW0_V1N0_SOURCE_DIR)/ecma_lexer.c $(VIEW0_V1N0_SOURCE_DIR)/ecma_parser.c $(VIEW0_V1N0_SOURCE_DIR)/ecma_pattern.c $(VIEW0_V1N0_SOURCE_DIR)/ecma_unicode.c $(VIEW0_V1N0_SOURCE_DIR)/ecma_unicode_tables.c
VIEW0_V1N3_G12_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g12.c
VIEW0_V1N3_G13_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g13.c
VIEW0_V1N3_G14_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g14.c
VIEW0_V1N3_G15_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g15.c
VIEW0_V1N3_G16_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/g16.c
VIEW0_V1N0_MAIN_SOURCE := $(VIEW0_V1N0_SOURCE_DIR)/main.c
VIEW0_V1N0_HEADER := $(VIEW0_V1N0_PRIVATE_INCLUDE)/arborcore/view0_conformance/native.h
VIEW0_V1N0_TEST_SOURCE := tests/c/view0_v1_native_foundation_test.c
VIEW0_V1N0_ADVERSARIAL_SOURCE := tests/c/view0_v1_native_foundation_adversarial_test.c
VIEW0_V1N0_NATIVE_DIR := $(VIEW0_V1_BUILD_DIR)/native
VIEW0_V1N0_LEXBOR_SOURCE := $(VIEW0_V1N0_NATIVE_DIR)/lexbor-src
VIEW0_V1N0_LEXBOR_RELEASE_BUILD := $(VIEW0_V1N0_NATIVE_DIR)/lexbor-build-release
VIEW0_V1N0_LEXBOR_SANITIZE_BUILD := $(VIEW0_V1N0_NATIVE_DIR)/lexbor-build-sanitize
VIEW0_V1N0_LEXBOR_RELEASE_LIB := $(VIEW0_V1N0_LEXBOR_RELEASE_BUILD)/liblexbor_static.a
VIEW0_V1N0_LEXBOR_SANITIZE_LIB := $(VIEW0_V1N0_LEXBOR_SANITIZE_BUILD)/liblexbor_static.a
VIEW0_V1N0_NATIVE_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/native.o
VIEW0_V1N0_ADAPTER_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/lexbor_adapter.o
VIEW0_V1N1_G03_C0_PROVENANCE_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_c0_provenance.o
VIEW0_V1N1_G03_R1A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r1a.o
VIEW0_V1N1_G03_R2A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r2a.o
VIEW0_V1N1_G03_R3A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r3a.o
VIEW0_V1N1_G03_R4A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r4a.o
VIEW0_V1N1_G03_R5A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r5a.o
VIEW0_V1N1_G03_R7A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r7a.o
VIEW0_V1N1_G04_R1A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r1a.o
VIEW0_V1N1_G04_R2A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r2a.o
VIEW0_V1N1_G05_C0_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_c0.o
VIEW0_V1N1_G05_R1A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r1a.o
VIEW0_V1N1_G05_R2A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r2a.o
VIEW0_V1N1_G05_R3A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r3a.o
VIEW0_V1N1_G05_R4A_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r4a.o
VIEW0_V1N1_G06_C0_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g06_c0.o
VIEW0_V1N1_G06_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g06.o
VIEW0_V1N2_C0_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_c0.o
VIEW0_V1N2_G07_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g07.o
VIEW0_V1N2_G08_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g08.o
VIEW0_V1N2_G09_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g09.o
VIEW0_V1N2_G10_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g10.o
VIEW0_V1N2_G11_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g11.o
VIEW0_V1N3_C0_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n3_c0.o
VIEW0_V1N3_ECMA_OBJS := $(VIEW0_V1N0_NATIVE_DIR)/ecma_early_errors.o $(VIEW0_V1N0_NATIVE_DIR)/ecma_frontend.o $(VIEW0_V1N0_NATIVE_DIR)/ecma_lexer.o $(VIEW0_V1N0_NATIVE_DIR)/ecma_parser.o $(VIEW0_V1N0_NATIVE_DIR)/ecma_pattern.o $(VIEW0_V1N0_NATIVE_DIR)/ecma_unicode.o $(VIEW0_V1N0_NATIVE_DIR)/ecma_unicode_tables.o
VIEW0_V1N3_G12_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g12.o
VIEW0_V1N3_G13_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g13.o
VIEW0_V1N3_G14_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g14.o
VIEW0_V1N3_G15_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g15.o
VIEW0_V1N3_G16_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g16.o
VIEW0_V1N1_G03_C0_WRAP_LDFLAG := -Wl,--wrap=lxb_html_interface_create -Wl,--wrap=lxb_html_tree_insert_foreign_element
VIEW0_V1N0_NATIVE_RULE_OBJS := $(VIEW0_V1N0_NATIVE_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) $(VIEW0_V1N1_G03_R1A_OBJ) $(VIEW0_V1N1_G03_R2A_OBJ) $(VIEW0_V1N1_G03_R3A_OBJ) $(VIEW0_V1N1_G03_R4A_OBJ) $(VIEW0_V1N1_G03_R5A_OBJ) $(VIEW0_V1N1_G03_R7A_OBJ) $(VIEW0_V1N1_G04_R1A_OBJ) $(VIEW0_V1N1_G04_R2A_OBJ) $(VIEW0_V1N1_G05_C0_OBJ) $(VIEW0_V1N1_G05_R1A_OBJ) $(VIEW0_V1N1_G05_R2A_OBJ) $(VIEW0_V1N1_G05_R3A_OBJ) $(VIEW0_V1N1_G05_R4A_OBJ) $(VIEW0_V1N1_G06_C0_OBJ) $(VIEW0_V1N1_G06_OBJ) $(VIEW0_V1N2_C0_OBJ) $(VIEW0_V1N2_G07_OBJ) $(VIEW0_V1N2_G08_OBJ) $(VIEW0_V1N2_G09_OBJ) $(VIEW0_V1N2_G10_OBJ) $(VIEW0_V1N2_G11_OBJ) $(VIEW0_V1N3_C0_OBJ) $(VIEW0_V1N3_ECMA_OBJS) $(VIEW0_V1N3_G12_OBJ) $(VIEW0_V1N3_G13_OBJ) $(VIEW0_V1N3_G14_OBJ) $(VIEW0_V1N3_G15_OBJ) $(VIEW0_V1N3_G16_OBJ)
VIEW0_V1N0_NATIVE_RULE_SOURCES := $(VIEW0_V1N0_NATIVE_SOURCE) $(VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE) $(VIEW0_V1N1_G03_R1A_SOURCE) $(VIEW0_V1N1_G03_R2A_SOURCE) $(VIEW0_V1N1_G03_R3A_SOURCE) $(VIEW0_V1N1_G03_R4A_SOURCE) $(VIEW0_V1N1_G03_R5A_SOURCE) $(VIEW0_V1N1_G03_R7A_SOURCE) $(VIEW0_V1N1_G04_R1A_SOURCE) $(VIEW0_V1N1_G04_R2A_SOURCE) $(VIEW0_V1N1_G05_C0_SOURCE) $(VIEW0_V1N1_G05_R1A_SOURCE) $(VIEW0_V1N1_G05_R2A_SOURCE) $(VIEW0_V1N1_G05_R3A_SOURCE) $(VIEW0_V1N1_G05_R4A_SOURCE) $(VIEW0_V1N1_G06_C0_SOURCE) $(VIEW0_V1N1_G06_SOURCE) $(VIEW0_V1N2_C0_SOURCE) $(VIEW0_V1N2_G07_SOURCE) $(VIEW0_V1N2_G08_SOURCE) $(VIEW0_V1N2_G09_SOURCE) $(VIEW0_V1N2_G10_SOURCE) $(VIEW0_V1N2_G11_SOURCE) $(VIEW0_V1N3_C0_SOURCE) $(VIEW0_V1N3_ECMA_SOURCES) $(VIEW0_V1N3_G12_SOURCE) $(VIEW0_V1N3_G13_SOURCE) $(VIEW0_V1N3_G14_SOURCE) $(VIEW0_V1N3_G15_SOURCE) $(VIEW0_V1N3_G16_SOURCE)
VIEW0_V1N0_MAIN_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/main.o
VIEW0_V1N0_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/foundation_test.o
VIEW0_V1N0_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/foundation_adversarial_test.o
VIEW0_V1N0_TOOL := $(VIEW0_V1N0_NATIVE_DIR)/arborcore-view0-html-check
VIEW0_V1N0_TEST := $(VIEW0_V1N0_NATIVE_DIR)/foundation-test
VIEW0_V1N0_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/foundation-adversarial-test
VIEW0_V1N0_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/foundation-sanitize-test
VIEW0_V1N0_CPPFLAGS := $(ARBORCORE_C_CPPFLAGS) -I$(VIEW0_V1N0_PRIVATE_INCLUDE) -isystem $(VIEW0_V1N0_LEXBOR_SOURCE)/source

.PHONY: view0-v1n0-lexbor-release view0-v1n0-lexbor-sanitize
.PHONY: view0-v1n0-tool view0-v1n0-test view0-v1n0-adversarial-test view0-v1n0-sanitize view0-v1n0-gate

$(VIEW0_V1N0_NATIVE_DIR):
	mkdir -p $@

view0-v1n0-lexbor-release:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1_lexbor_build.sh release

view0-v1n0-lexbor-sanitize:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1_lexbor_build.sh sanitize

$(VIEW0_V1N0_LEXBOR_RELEASE_LIB): tools/view0_v1_lexbor_acquire.sh tools/view0_v1_lexbor_build.sh
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1_lexbor_build.sh release >/dev/null

$(VIEW0_V1N0_LEXBOR_SANITIZE_LIB): tools/view0_v1_lexbor_acquire.sh tools/view0_v1_lexbor_build.sh
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1_lexbor_build.sh sanitize >/dev/null

$(VIEW0_V1N0_NATIVE_OBJ): $(VIEW0_V1N0_NATIVE_SOURCE) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N1_G03_R1A_INTERNAL) $(VIEW0_V1N1_G03_R2A_INTERNAL) $(VIEW0_V1N1_G03_R3A_INTERNAL) $(VIEW0_V1N1_G03_R4A_INTERNAL) $(VIEW0_V1N1_G03_R5A_INTERNAL) $(VIEW0_V1N1_G03_R7A_INTERNAL) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N1_G04_R2A_INTERNAL) $(VIEW0_V1N1_G05_R1A_INTERNAL) $(VIEW0_V1N1_G05_R2A_INTERNAL) $(VIEW0_V1N1_G05_R3A_INTERNAL) $(VIEW0_V1N1_G05_R4A_INTERNAL) $(VIEW0_V1N1_G06_INTERNAL) $(VIEW0_V1N2_C0_INTERNAL) $(VIEW0_V1N2_G07_INTERNAL) $(VIEW0_V1N2_G08_INTERNAL) $(VIEW0_V1N2_G09_INTERNAL) $(VIEW0_V1N2_G10_INTERNAL) $(VIEW0_V1N2_G11_INTERNAL) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N0_ADAPTER_OBJ): $(VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N1_G03_C0_PROVENANCE_INTERNAL) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R1A_OBJ): $(VIEW0_V1N1_G03_R1A_SOURCE) $(VIEW0_V1N1_G03_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R2A_OBJ): $(VIEW0_V1N1_G03_R2A_SOURCE) $(VIEW0_V1N1_G03_R2A_INTERNAL) $(VIEW0_V1N1_G06_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R3A_OBJ): $(VIEW0_V1N1_G03_R3A_SOURCE) $(VIEW0_V1N1_G03_R3A_INTERNAL) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N1_G06_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R4A_OBJ): $(VIEW0_V1N1_G03_R4A_SOURCE) $(VIEW0_V1N1_G03_R4A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R5A_OBJ): $(VIEW0_V1N1_G03_R5A_SOURCE) $(VIEW0_V1N1_G03_R5A_INTERNAL) $(VIEW0_V1N1_G03_R1A_INTERNAL) $(VIEW0_V1N1_G03_R2A_INTERNAL) $(VIEW0_V1N1_G03_R3A_INTERNAL) $(VIEW0_V1N1_G03_R4A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N0_MAIN_OBJ): $(VIEW0_V1N0_MAIN_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N0_TEST_OBJ): $(VIEW0_V1N0_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N0_ADVERSARIAL_OBJ): $(VIEW0_V1N0_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@


$(VIEW0_V1N1_G03_R7A_OBJ): $(VIEW0_V1N1_G03_R7A_SOURCE) $(VIEW0_V1N1_G03_R7A_INTERNAL) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R1A_OBJ): $(VIEW0_V1N1_G04_R1A_SOURCE) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N1_G03_R1A_INTERNAL) $(VIEW0_V1N1_G03_R2A_INTERNAL) $(VIEW0_V1N1_G03_R3A_INTERNAL) $(VIEW0_V1N1_G03_R4A_INTERNAL) $(VIEW0_V1N1_G03_R5A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R2A_OBJ): $(VIEW0_V1N1_G04_R2A_SOURCE) $(VIEW0_V1N1_G04_R2A_INTERNAL) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_C0_OBJ): $(VIEW0_V1N1_G05_C0_SOURCE) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R1A_OBJ): $(VIEW0_V1N1_G05_R1A_SOURCE) $(VIEW0_V1N1_G05_R1A_INTERNAL) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R2A_OBJ): $(VIEW0_V1N1_G05_R2A_SOURCE) $(VIEW0_V1N1_G05_R2A_INTERNAL) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R3A_OBJ): $(VIEW0_V1N1_G05_R3A_SOURCE) $(VIEW0_V1N1_G05_R3A_INTERNAL) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G06_C0_OBJ): $(VIEW0_V1N1_G06_C0_SOURCE) $(VIEW0_V1N1_G06_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G06_OBJ): $(VIEW0_V1N1_G06_SOURCE) $(VIEW0_V1N1_G06_INTERNAL) $(VIEW0_V1N1_G06_C0_INTERNAL) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_C0_OBJ): $(VIEW0_V1N2_C0_SOURCE) $(VIEW0_V1N2_C0_INTERNAL) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G07_OBJ): $(VIEW0_V1N2_G07_SOURCE) $(VIEW0_V1N2_G07_INTERNAL) $(VIEW0_V1N2_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G08_OBJ): $(VIEW0_V1N2_G08_SOURCE) $(VIEW0_V1N2_G08_INTERNAL) $(VIEW0_V1N2_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G09_OBJ): $(VIEW0_V1N2_G09_SOURCE) $(VIEW0_V1N2_G09_INTERNAL) $(VIEW0_V1N2_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G10_OBJ): $(VIEW0_V1N2_G10_SOURCE) $(VIEW0_V1N2_G10_INTERNAL) $(VIEW0_V1N2_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G11_OBJ): $(VIEW0_V1N2_G11_SOURCE) $(VIEW0_V1N2_G11_INTERNAL) $(VIEW0_V1N2_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N3_C0_OBJ): $(VIEW0_V1N3_C0_SOURCE) $(VIEW0_V1N3_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N0_NATIVE_DIR)/ecma_%.o: $(VIEW0_V1N0_SOURCE_DIR)/ecma_%.c $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N3_G12_OBJ): $(VIEW0_V1N3_G12_SOURCE) $(VIEW0_V1N3_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N3_G13_OBJ): $(VIEW0_V1N3_G13_SOURCE) $(VIEW0_V1N3_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N3_G14_OBJ): $(VIEW0_V1N3_G14_SOURCE) $(VIEW0_V1N3_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N3_G15_OBJ): $(VIEW0_V1N3_G15_SOURCE) $(VIEW0_V1N3_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N3_G16_OBJ): $(VIEW0_V1N3_G16_SOURCE) $(VIEW0_V1N3_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N0_TOOL): $(VIEW0_V1N0_MAIN_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N0_MAIN_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n0-tool: $(VIEW0_V1N0_TOOL)

$(VIEW0_V1N0_TEST): $(VIEW0_V1N0_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N0_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n0-test: $(VIEW0_V1N0_TEST)
	@$(VIEW0_V1N0_TEST)

$(VIEW0_V1N0_ADVERSARIAL_TEST): $(VIEW0_V1N0_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N0_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n0-adversarial-test: $(VIEW0_V1N0_ADVERSARIAL_TEST)
	@$(VIEW0_V1N0_ADVERSARIAL_TEST)

$(VIEW0_V1N0_SANITIZE_TEST): $(VIEW0_V1N0_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N0_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n0-sanitize: $(VIEW0_V1N0_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N0_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N0 ASan/UBSan native adapter and sanitized Lexbor qualification"

view0-v1n0-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1_native_foundation_gate.sh

# ============================================================
# VIEW0 V1N1 C0 — private Lexbor-independent document facts
# ============================================================
VIEW0_V1N1_C0_TEST_SOURCE := tests/c/view0_v1n1_c0_facts_test.c
VIEW0_V1N1_C0_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_c0_facts_adversarial_test.c
VIEW0_V1N1_C0_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/c0_facts_test.o
VIEW0_V1N1_C0_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/c0_facts_adversarial_test.o
VIEW0_V1N1_C0_TEST := $(VIEW0_V1N0_NATIVE_DIR)/c0-facts-test
VIEW0_V1N1_C0_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/c0-facts-adversarial-test
VIEW0_V1N1_C0_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/c0-facts-sanitize-test

.PHONY: view0-v1n1-c0-test view0-v1n1-c0-adversarial-test view0-v1n1-c0-sanitize view0-v1n1-c0-gate

$(VIEW0_V1N1_C0_TEST_OBJ): $(VIEW0_V1N1_C0_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_C0_ADVERSARIAL_OBJ): $(VIEW0_V1N1_C0_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_C0_TEST): $(VIEW0_V1N1_C0_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_C0_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-c0-test: $(VIEW0_V1N1_C0_TEST)
	@$(VIEW0_V1N1_C0_TEST)

$(VIEW0_V1N1_C0_ADVERSARIAL_TEST): $(VIEW0_V1N1_C0_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_C0_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-c0-adversarial-test: $(VIEW0_V1N1_C0_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_C0_ADVERSARIAL_TEST)

$(VIEW0_V1N1_C0_SANITIZE_TEST): $(VIEW0_V1N1_C0_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_C0_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-c0-sanitize: $(VIEW0_V1N1_C0_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_C0_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 C0 ASan/UBSan facts adapter and sanitized Lexbor qualification"

view0-v1n1-c0-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_c0_gate.sh

# ============================================================
# VIEW0 V1N1 G02 R1 — required document DOCTYPE only
# ============================================================
VIEW0_V1N1_G02_R1_TEST_SOURCE := tests/c/view0_v1n1_g02_r1_doctype_required_test.c
VIEW0_V1N1_G02_R1_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g02_r1_doctype_required_adversarial_test.c
VIEW0_V1N1_G02_R1_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r1_doctype_required_test.o
VIEW0_V1N1_G02_R1_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r1_doctype_required_adversarial_test.o
VIEW0_V1N1_G02_R1_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r1-doctype-required-test
VIEW0_V1N1_G02_R1_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r1-doctype-required-adversarial-test
VIEW0_V1N1_G02_R1_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r1-doctype-required-sanitize-test

.PHONY: view0-v1n1-g02-r1-test view0-v1n1-g02-r1-adversarial-test view0-v1n1-g02-r1-sanitize view0-v1n1-g02-r1-gate

$(VIEW0_V1N1_G02_R1_TEST_OBJ): $(VIEW0_V1N1_G02_R1_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R1_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G02_R1_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R1_TEST): $(VIEW0_V1N1_G02_R1_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G02_R1_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g02-r1-test: $(VIEW0_V1N1_G02_R1_TEST)
	@$(VIEW0_V1N1_G02_R1_TEST)

$(VIEW0_V1N1_G02_R1_ADVERSARIAL_TEST): $(VIEW0_V1N1_G02_R1_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G02_R1_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g02-r1-adversarial-test: $(VIEW0_V1N1_G02_R1_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G02_R1_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G02_R1_SANITIZE_TEST): $(VIEW0_V1N1_G02_R1_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G02_R1_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g02-r1-sanitize: $(VIEW0_V1N1_G02_R1_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G02_R1_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G02 R1 ASan/UBSan required-doctype rule + sanitized Lexbor qualification"

view0-v1n1-g02-r1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g02_r1_gate.sh

# ============================================================
# VIEW0 V1N1 G02 R2 — DOCTYPE authoring syntax only
# ============================================================
VIEW0_V1N1_G02_R2_TEST_SOURCE := tests/c/view0_v1n1_g02_r2_doctype_syntax_test.c
VIEW0_V1N1_G02_R2_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g02_r2_doctype_syntax_adversarial_test.c
VIEW0_V1N1_G02_R2_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r2_doctype_syntax_test.o
VIEW0_V1N1_G02_R2_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r2_doctype_syntax_adversarial_test.o
VIEW0_V1N1_G02_R2_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r2-doctype-syntax-test
VIEW0_V1N1_G02_R2_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r2-doctype-syntax-adversarial-test
VIEW0_V1N1_G02_R2_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r2-doctype-syntax-sanitize-test

.PHONY: view0-v1n1-g02-r2-test view0-v1n1-g02-r2-adversarial-test view0-v1n1-g02-r2-sanitize view0-v1n1-g02-r2-gate

$(VIEW0_V1N1_G02_R2_TEST_OBJ): $(VIEW0_V1N1_G02_R2_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R2_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G02_R2_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R2_TEST): $(VIEW0_V1N1_G02_R2_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G02_R2_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g02-r2-test: $(VIEW0_V1N1_G02_R2_TEST)
	@$(VIEW0_V1N1_G02_R2_TEST)

$(VIEW0_V1N1_G02_R2_ADVERSARIAL_TEST): $(VIEW0_V1N1_G02_R2_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G02_R2_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g02-r2-adversarial-test: $(VIEW0_V1N1_G02_R2_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G02_R2_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G02_R2_SANITIZE_TEST): $(VIEW0_V1N1_G02_R2_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G02_R2_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g02-r2-sanitize: $(VIEW0_V1N1_G02_R2_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G02_R2_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G02 R2 ASan/UBSan DOCTYPE syntax rule + sanitized Lexbor qualification"

view0-v1n1-g02-r2-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g02_r2_gate.sh


VIEW0_V1N1_G02_R3_TEST_SOURCE := tests/c/view0_v1n1_g02_r3_doctype_legacy_discouraged_test.c
VIEW0_V1N1_G02_R3_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g02_r3_doctype_legacy_discouraged_adversarial_test.c
VIEW0_V1N1_G02_R3_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r3_doctype_legacy_discouraged_test.o
VIEW0_V1N1_G02_R3_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r3_doctype_legacy_discouraged_adversarial_test.o
VIEW0_V1N1_G02_R3_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r3-doctype-legacy-discouraged-test
VIEW0_V1N1_G02_R3_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r3-doctype-legacy-discouraged-adversarial-test
VIEW0_V1N1_G02_R3_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r3-doctype-legacy-discouraged-sanitize-test

.PHONY: view0-v1n1-g02-r3-test view0-v1n1-g02-r3-adversarial-test view0-v1n1-g02-r3-sanitize view0-v1n1-g02-r3-gate

$(VIEW0_V1N1_G02_R3_TEST_OBJ): $(VIEW0_V1N1_G02_R3_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R3_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G02_R3_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R3_TEST): $(VIEW0_V1N1_G02_R3_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(VIEW0_V1N1_G02_R3_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $@

view0-v1n1-g02-r3-test: $(VIEW0_V1N1_G02_R3_TEST)
	@$(VIEW0_V1N1_G02_R3_TEST)

$(VIEW0_V1N1_G02_R3_ADVERSARIAL_TEST): $(VIEW0_V1N1_G02_R3_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(VIEW0_V1N1_G02_R3_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $@

view0-v1n1-g02-r3-adversarial-test: $(VIEW0_V1N1_G02_R3_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G02_R3_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G02_R3_SANITIZE_TEST): $(VIEW0_V1N1_G02_R3_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -O1 -g -fno-omit-frame-pointer \
		-fsanitize=address,undefined \
		$(VIEW0_V1N1_G02_R3_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g02-r3-sanitize: $(VIEW0_V1N1_G02_R3_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G02_R3_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G02 R3 ASan/UBSan legacy-warning rule + sanitized Lexbor qualification"

view0-v1n1-g02-r3-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g02_r3_gate.sh

# ============================================================
# VIEW0 V1N1 G02 R4 — standalone head/title cardinality
# ============================================================
VIEW0_V1N1_G02_R4_TEST_SOURCE := tests/c/view0_v1n1_g02_r4_head_title_cardinality_test.c
VIEW0_V1N1_G02_R4_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g02_r4_head_title_cardinality_adversarial_test.c
VIEW0_V1N1_G02_R4_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r4_head_title_cardinality_test.o
VIEW0_V1N1_G02_R4_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r4_head_title_cardinality_adversarial_test.o
VIEW0_V1N1_G02_R4_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r4-head-title-cardinality-test
VIEW0_V1N1_G02_R4_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r4-head-title-cardinality-adversarial-test
VIEW0_V1N1_G02_R4_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r4-head-title-cardinality-sanitize-test

.PHONY: view0-v1n1-g02-r4-test view0-v1n1-g02-r4-adversarial-test view0-v1n1-g02-r4-sanitize view0-v1n1-g02-r4-gate

$(VIEW0_V1N1_G02_R4_TEST_OBJ): $(VIEW0_V1N1_G02_R4_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R4_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G02_R4_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R4_TEST): $(VIEW0_V1N1_G02_R4_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(VIEW0_V1N1_G02_R4_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $@

view0-v1n1-g02-r4-test: $(VIEW0_V1N1_G02_R4_TEST)
	@$(VIEW0_V1N1_G02_R4_TEST)

$(VIEW0_V1N1_G02_R4_ADVERSARIAL_TEST): $(VIEW0_V1N1_G02_R4_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(VIEW0_V1N1_G02_R4_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $@

view0-v1n1-g02-r4-adversarial-test: $(VIEW0_V1N1_G02_R4_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G02_R4_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G02_R4_SANITIZE_TEST): $(VIEW0_V1N1_G02_R4_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -O1 -g -fno-omit-frame-pointer \
		-fsanitize=address,undefined \
		$(VIEW0_V1N1_G02_R4_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g02-r4-sanitize: $(VIEW0_V1N1_G02_R4_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G02_R4_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G02 R4 ASan/UBSan head-title-cardinality rule + sanitized Lexbor qualification"

view0-v1n1-g02-r4-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g02_r4_gate.sh

# ============================================================
# VIEW0 V1N1 G02 R5 — head/base maximum cardinality
# ============================================================
VIEW0_V1N1_G02_R5_TEST_SOURCE := tests/c/view0_v1n1_g02_r5_head_base_cardinality_test.c
VIEW0_V1N1_G02_R5_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g02_r5_head_base_cardinality_adversarial_test.c
VIEW0_V1N1_G02_R5_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r5_head_base_cardinality_test.o
VIEW0_V1N1_G02_R5_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r5_head_base_cardinality_adversarial_test.o
VIEW0_V1N1_G02_R5_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r5-head-base-cardinality-test
VIEW0_V1N1_G02_R5_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r5-head-base-cardinality-adversarial-test
VIEW0_V1N1_G02_R5_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r5-head-base-cardinality-sanitize-test

.PHONY: view0-v1n1-g02-r5-test view0-v1n1-g02-r5-adversarial-test view0-v1n1-g02-r5-sanitize view0-v1n1-g02-r5-gate

$(VIEW0_V1N1_G02_R5_TEST_OBJ): $(VIEW0_V1N1_G02_R5_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R5_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G02_R5_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R5_TEST): $(VIEW0_V1N1_G02_R5_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(VIEW0_V1N1_G02_R5_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $@

view0-v1n1-g02-r5-test: $(VIEW0_V1N1_G02_R5_TEST)
	@$(VIEW0_V1N1_G02_R5_TEST)

$(VIEW0_V1N1_G02_R5_ADVERSARIAL_TEST): $(VIEW0_V1N1_G02_R5_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(VIEW0_V1N1_G02_R5_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $@

view0-v1n1-g02-r5-adversarial-test: $(VIEW0_V1N1_G02_R5_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G02_R5_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G02_R5_SANITIZE_TEST): $(VIEW0_V1N1_G02_R5_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -O1 -g -fno-omit-frame-pointer \
		-fsanitize=address,undefined \
		$(VIEW0_V1N1_G02_R5_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g02-r5-sanitize: $(VIEW0_V1N1_G02_R5_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G02_R5_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G02 R5 ASan/UBSan head-base-cardinality rule + sanitized Lexbor qualification"

view0-v1n1-g02-r5-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g02_r5_gate.sh

# ============================================================
# VIEW0 V1N1 G02 R6 — logical body singleton / parser repair
# ============================================================
VIEW0_V1N1_G02_R6_TEST_SOURCE := tests/c/view0_v1n1_g02_r6_body_singleton_test.c
VIEW0_V1N1_G02_R6_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g02_r6_body_singleton_adversarial_test.c
VIEW0_V1N1_G02_R6_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r6_body_singleton_test.o
VIEW0_V1N1_G02_R6_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g02_r6_body_singleton_adversarial_test.o
VIEW0_V1N1_G02_R6_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r6-body-singleton-test
VIEW0_V1N1_G02_R6_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r6-body-singleton-adversarial-test
VIEW0_V1N1_G02_R6_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g02-r6-body-singleton-sanitize-test

.PHONY: view0-v1n1-g02-r6-test view0-v1n1-g02-r6-adversarial-test view0-v1n1-g02-r6-sanitize view0-v1n1-g02-r6-gate

$(VIEW0_V1N1_G02_R6_TEST_OBJ): $(VIEW0_V1N1_G02_R6_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R6_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G02_R6_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G02_R6_TEST): $(VIEW0_V1N1_G02_R6_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(VIEW0_V1N1_G02_R6_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $@

view0-v1n1-g02-r6-test: $(VIEW0_V1N1_G02_R6_TEST)
	@$(VIEW0_V1N1_G02_R6_TEST)

$(VIEW0_V1N1_G02_R6_ADVERSARIAL_TEST): $(VIEW0_V1N1_G02_R6_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(VIEW0_V1N1_G02_R6_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $@

view0-v1n1-g02-r6-adversarial-test: $(VIEW0_V1N1_G02_R6_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G02_R6_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G02_R6_SANITIZE_TEST): $(VIEW0_V1N1_G02_R6_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(CPPFLAGS) $(VIEW0_V1N0_CPPFLAGS) $(CFLAGS) -O1 -g -fno-omit-frame-pointer \
		-fsanitize=address,undefined \
		$(VIEW0_V1N1_G02_R6_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g02-r6-sanitize: $(VIEW0_V1N1_G02_R6_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G02_R6_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G02 R6 ASan/UBSan body-singleton rule + sanitized Lexbor qualification"

view0-v1n1-g02-r6-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g02_r6_gate.sh

# ============================================================
# VIEW0 V1N1 G03 C0 — neutral DOM observation + source provenance
# ============================================================
VIEW0_V1N1_G03_C0_TEST_SOURCE := tests/c/view0_v1n1_g03_c0_observation_test.c
VIEW0_V1N1_G03_C0_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_c0_observation_adversarial_test.c
VIEW0_V1N1_G03_C0_NOWRAP_SOURCE := tests/c/view0_v1n1_g03_c0_nowrap_test.c
VIEW0_V1N1_G03_C0_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_c0_observation_test.o
VIEW0_V1N1_G03_C0_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_c0_observation_adversarial_test.o
VIEW0_V1N1_G03_C0_NOWRAP_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_c0_nowrap_test.o
VIEW0_V1N1_G03_C0_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-observation-test
VIEW0_V1N1_G03_C0_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-observation-adversarial-test
VIEW0_V1N1_G03_C0_NOWRAP_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-nowrap-test
VIEW0_V1N1_G03_C0_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-observation-sanitize-test

.PHONY: view0-v1n1-g03-c0-test view0-v1n1-g03-c0-adversarial-test view0-v1n1-g03-c0-nowrap-test view0-v1n1-g03-c0-sanitize view0-v1n1-g03-c0-gate

$(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ): $(VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_INTERNAL) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_C0_TEST_OBJ): $(VIEW0_V1N1_G03_C0_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_C0_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_C0_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_C0_NOWRAP_OBJ): $(VIEW0_V1N1_G03_C0_NOWRAP_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_C0_TEST): $(VIEW0_V1N1_G03_C0_TEST_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -o $@ \
		$(VIEW0_V1N1_G03_C0_TEST_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) -lm

view0-v1n1-g03-c0-test: $(VIEW0_V1N1_G03_C0_TEST)
	@$(VIEW0_V1N1_G03_C0_TEST)

$(VIEW0_V1N1_G03_C0_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_C0_ADVERSARIAL_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -o $@ \
		$(VIEW0_V1N1_G03_C0_ADVERSARIAL_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) -lm

view0-v1n1-g03-c0-adversarial-test: $(VIEW0_V1N1_G03_C0_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_C0_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_C0_NOWRAP_TEST): $(VIEW0_V1N1_G03_C0_NOWRAP_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_C0_NOWRAP_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) -lm

view0-v1n1-g03-c0-nowrap-test: $(VIEW0_V1N1_G03_C0_NOWRAP_TEST)
	@$(VIEW0_V1N1_G03_C0_NOWRAP_TEST)

$(VIEW0_V1N1_G03_C0_SANITIZE_TEST): $(VIEW0_V1N1_G03_C0_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_INTERNAL) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_C0_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-c0-sanitize: $(VIEW0_V1N1_G03_C0_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_C0_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 C0 ASan/UBSan neutral observation + sanitized Lexbor qualification"

view0-v1n1-g03-c0-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_c0_gate.sh

# ============================================================
# VIEW0 V1N1 G03 C0-L1 — neutral DFS traversal lifecycle retrofit
# ============================================================
VIEW0_V1N1_G03_C0_L1_TEST_SOURCE := tests/c/view0_v1n1_g03_c0_lifecycle_test.c
VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_c0_lifecycle_adversarial_test.c
VIEW0_V1N1_G03_C0_L1_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_c0_lifecycle_test.o
VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_c0_lifecycle_adversarial_test.o
VIEW0_V1N1_G03_C0_L1_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-lifecycle-test
VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-lifecycle-adversarial-test
VIEW0_V1N1_G03_C0_L1_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-lifecycle-sanitize-test

.PHONY: view0-v1n1-g03-c0-l1-test view0-v1n1-g03-c0-l1-adversarial-test view0-v1n1-g03-c0-l1-sanitize view0-v1n1-g03-c0-l1-gate

$(VIEW0_V1N1_G03_C0_L1_TEST_OBJ): $(VIEW0_V1N1_G03_C0_L1_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_C0_L1_TEST): $(VIEW0_V1N1_G03_C0_L1_TEST_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -o $@ \
		$(VIEW0_V1N1_G03_C0_L1_TEST_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) -lm

view0-v1n1-g03-c0-l1-test: $(VIEW0_V1N1_G03_C0_L1_TEST)
	@$(VIEW0_V1N1_G03_C0_L1_TEST)

$(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -o $@ \
		$(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) -lm

view0-v1n1-g03-c0-l1-adversarial-test: $(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_C0_L1_SANITIZE_TEST): $(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_INTERNAL) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_C0_L1_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-c0-l1-sanitize: $(VIEW0_V1N1_G03_C0_L1_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_C0_L1_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 C0-L1 ASan/UBSan neutral lifecycle + sanitized Lexbor qualification"

view0-v1n1-g03-c0-l1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_c0_l1_gate.sh

# ============================================================
# VIEW0 V1N1 G03 C0-SR1 — neutral single-parser source/repair context
# ============================================================
VIEW0_V1N1_G03_C0_SR1_TEST_SOURCE := tests/c/view0_v1n1_g03_c0_sr1_source_repair_test.c
VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_c0_sr1_source_repair_adversarial_test.c
VIEW0_V1N1_G03_C0_SR1_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_c0_sr1_source_repair_test.o
VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_c0_sr1_source_repair_adversarial_test.o
VIEW0_V1N1_G03_C0_SR1_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-sr1-source-repair-test
VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-sr1-source-repair-adversarial-test
VIEW0_V1N1_G03_C0_SR1_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-c0-sr1-source-repair-sanitize-test

.PHONY: view0-v1n1-g03-c0-sr1-test view0-v1n1-g03-c0-sr1-adversarial-test view0-v1n1-g03-c0-sr1-sanitize view0-v1n1-g03-c0-sr1-gate

$(VIEW0_V1N1_G03_C0_SR1_TEST_OBJ): $(VIEW0_V1N1_G03_C0_SR1_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_C0_SR1_TEST): $(VIEW0_V1N1_G03_C0_SR1_TEST_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -o $@ \
		$(VIEW0_V1N1_G03_C0_SR1_TEST_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) -lm

view0-v1n1-g03-c0-sr1-test: $(VIEW0_V1N1_G03_C0_SR1_TEST)
	@$(VIEW0_V1N1_G03_C0_SR1_TEST)

$(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -o $@ \
		$(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_OBJ) $(VIEW0_V1N0_ADAPTER_OBJ) $(VIEW0_V1N1_G03_C0_PROVENANCE_OBJ) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) -lm

view0-v1n1-g03-c0-sr1-adversarial-test: $(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_C0_SR1_SANITIZE_TEST): $(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_INTERNAL) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_C0_SR1_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_LEXBOR_ADAPTER_SOURCE) $(VIEW0_V1N1_G03_C0_PROVENANCE_SOURCE) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-c0-sr1-sanitize: $(VIEW0_V1N1_G03_C0_SR1_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_C0_SR1_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 C0-SR1 ASan/UBSan source/repair-context qualification"

view0-v1n1-g03-c0-sr1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_c0_sr1_gate.sh


# ============================================================
# VIEW0 V1N1 G03 R1A — partial structural ELEMENT_CONTEXT evaluator
# ============================================================
VIEW0_V1N1_G03_R1A_TEST_SOURCE := tests/c/view0_v1n1_g03_r1a_element_context_test.c
VIEW0_V1N1_G03_R1A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_r1a_element_context_adversarial_test.c
VIEW0_V1N1_G03_R1A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r1a_element_context_test.o
VIEW0_V1N1_G03_R1A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r1a_element_context_adversarial_test.o
VIEW0_V1N1_G03_R1A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r1a-element-context-test
VIEW0_V1N1_G03_R1A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r1a-element-context-adversarial-test
VIEW0_V1N1_G03_R1A_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r1a-element-context-sanitize-test

.PHONY: view0-v1n1-g03-r1a-test view0-v1n1-g03-r1a-adversarial-test view0-v1n1-g03-r1a-sanitize view0-v1n1-g03-r1a-gate

$(VIEW0_V1N1_G03_R1A_TEST_OBJ): $(VIEW0_V1N1_G03_R1A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R1A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_R1A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R1A_TEST): $(VIEW0_V1N1_G03_R1A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R1A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r1a-test: $(VIEW0_V1N1_G03_R1A_TEST)
	@$(VIEW0_V1N1_G03_R1A_TEST)

$(VIEW0_V1N1_G03_R1A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_R1A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R1A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r1a-adversarial-test: $(VIEW0_V1N1_G03_R1A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_R1A_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_R1A_SANITIZE_TEST): $(VIEW0_V1N1_G03_R1A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_R1A_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-r1a-sanitize: $(VIEW0_V1N1_G03_R1A_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_R1A_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 R1A ASan/UBSan partial ELEMENT_CONTEXT + sanitized Lexbor qualification"

view0-v1n1-g03-r1a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_r1a_gate.sh


# ============================================================
# VIEW0 V1N1 G03 R2A — partial residual CONTENT_MODEL evaluator
# ============================================================
VIEW0_V1N1_G03_R2A_TEST_SOURCE := tests/c/view0_v1n1_g03_r2a_content_model_test.c
VIEW0_V1N1_G03_R2A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_r2a_content_model_adversarial_test.c
VIEW0_V1N1_G03_R2A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r2a_content_model_test.o
VIEW0_V1N1_G03_R2A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r2a_content_model_adversarial_test.o
VIEW0_V1N1_G03_R2A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r2a-content-model-test
VIEW0_V1N1_G03_R2A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r2a-content-model-adversarial-test
VIEW0_V1N1_G03_R2A_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r2a-content-model-sanitize-test

.PHONY: view0-v1n1-g03-r2a-test view0-v1n1-g03-r2a-adversarial-test view0-v1n1-g03-r2a-sanitize view0-v1n1-g03-r2a-gate

$(VIEW0_V1N1_G03_R2A_TEST_OBJ): $(VIEW0_V1N1_G03_R2A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R2A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_R2A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R2A_TEST): $(VIEW0_V1N1_G03_R2A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R2A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r2a-test: $(VIEW0_V1N1_G03_R2A_TEST)
	@$(VIEW0_V1N1_G03_R2A_TEST)

$(VIEW0_V1N1_G03_R2A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_R2A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R2A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r2a-adversarial-test: $(VIEW0_V1N1_G03_R2A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_R2A_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_R2A_SANITIZE_TEST): $(VIEW0_V1N1_G03_R2A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_R2A_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-r2a-sanitize: $(VIEW0_V1N1_G03_R2A_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_R2A_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 R2A ASan/UBSan partial CONTENT_MODEL + sanitized Lexbor qualification"

view0-v1n1-g03-r2a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_r2a_gate.sh


# ============================================================
# VIEW0 V1N1 G03 R3A — partial DESCENDANT_EXCLUSIONS evaluator
# ============================================================
VIEW0_V1N1_G03_R3A_TEST_SOURCE := tests/c/view0_v1n1_g03_r3a_descendant_exclusions_test.c
VIEW0_V1N1_G03_R3A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_r3a_descendant_exclusions_adversarial_test.c
VIEW0_V1N1_G03_R3A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r3a_descendant_exclusions_test.o
VIEW0_V1N1_G03_R3A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r3a_descendant_exclusions_adversarial_test.o
VIEW0_V1N1_G03_R3A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r3a-descendant-exclusions-test
VIEW0_V1N1_G03_R3A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r3a-descendant-exclusions-adversarial-test
VIEW0_V1N1_G03_R3A_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r3a-descendant-exclusions-sanitize-test

.PHONY: view0-v1n1-g03-r3a-test view0-v1n1-g03-r3a-adversarial-test view0-v1n1-g03-r3a-sanitize view0-v1n1-g03-r3a-gate

$(VIEW0_V1N1_G03_R3A_TEST_OBJ): $(VIEW0_V1N1_G03_R3A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R3A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_R3A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R3A_TEST): $(VIEW0_V1N1_G03_R3A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R3A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r3a-test: $(VIEW0_V1N1_G03_R3A_TEST)
	@$(VIEW0_V1N1_G03_R3A_TEST)

$(VIEW0_V1N1_G03_R3A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_R3A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R3A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r3a-adversarial-test: $(VIEW0_V1N1_G03_R3A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_R3A_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_R3A_SANITIZE_TEST): $(VIEW0_V1N1_G03_R3A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_R3A_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-r3a-sanitize: $(VIEW0_V1N1_G03_R3A_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_R3A_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 R3A ASan/UBSan partial DESCENDANT_EXCLUSIONS + sanitized Lexbor qualification"

view0-v1n1-g03-r3a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_r3a_gate.sh


# ============================================================
# VIEW0 V1N1 G03 R4A — partial NOTHING_MODEL evaluator
# ============================================================
VIEW0_V1N1_G03_R4A_TEST_SOURCE := tests/c/view0_v1n1_g03_r4a_nothing_model_test.c
VIEW0_V1N1_G03_R4A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_r4a_nothing_model_adversarial_test.c
VIEW0_V1N1_G03_R4A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r4a_nothing_model_test.o
VIEW0_V1N1_G03_R4A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r4a_nothing_model_adversarial_test.o
VIEW0_V1N1_G03_R4A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r4a-nothing-model-test
VIEW0_V1N1_G03_R4A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r4a-nothing-model-adversarial-test
VIEW0_V1N1_G03_R4A_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r4a-nothing-model-sanitize-test

.PHONY: view0-v1n1-g03-r4a-test view0-v1n1-g03-r4a-adversarial-test view0-v1n1-g03-r4a-sanitize view0-v1n1-g03-r4a-gate

$(VIEW0_V1N1_G03_R4A_TEST_OBJ): $(VIEW0_V1N1_G03_R4A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R4A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_R4A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R4A_TEST): $(VIEW0_V1N1_G03_R4A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R4A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r4a-test: $(VIEW0_V1N1_G03_R4A_TEST)
	@$(VIEW0_V1N1_G03_R4A_TEST)

$(VIEW0_V1N1_G03_R4A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_R4A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R4A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r4a-adversarial-test: $(VIEW0_V1N1_G03_R4A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_R4A_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_R4A_SANITIZE_TEST): $(VIEW0_V1N1_G03_R4A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_R4A_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-r4a-sanitize: $(VIEW0_V1N1_G03_R4A_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_R4A_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 R4A ASan/UBSan partial NOTHING_MODEL + sanitized Lexbor qualification"

view0-v1n1-g03-r4a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_r4a_gate.sh


# ============================================================
# VIEW0 V1N1 G03 R5A — partial EXPLICIT_HTML_ELEMENT_ALLOWANCE evaluator
# ============================================================
VIEW0_V1N1_G03_R5A_TEST_SOURCE := tests/c/view0_v1n1_g03_r5a_explicit_html_element_allowance_test.c
VIEW0_V1N1_G03_R5A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_r5a_explicit_html_element_allowance_adversarial_test.c
VIEW0_V1N1_G03_R5A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r5a_explicit_html_element_allowance_test.o
VIEW0_V1N1_G03_R5A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r5a_explicit_html_element_allowance_adversarial_test.o
VIEW0_V1N1_G03_R5A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r5a-explicit-html-element-allowance-test
VIEW0_V1N1_G03_R5A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r5a-explicit-html-element-allowance-adversarial-test
VIEW0_V1N1_G03_R5A_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r5a-explicit-html-element-allowance-sanitize-test

.PHONY: view0-v1n1-g03-r5a-test view0-v1n1-g03-r5a-adversarial-test view0-v1n1-g03-r5a-sanitize view0-v1n1-g03-r5a-gate

$(VIEW0_V1N1_G03_R5A_TEST_OBJ): $(VIEW0_V1N1_G03_R5A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R5A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_R5A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R5A_TEST): $(VIEW0_V1N1_G03_R5A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R5A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r5a-test: $(VIEW0_V1N1_G03_R5A_TEST)
	@$(VIEW0_V1N1_G03_R5A_TEST)

$(VIEW0_V1N1_G03_R5A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_R5A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R5A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r5a-adversarial-test: $(VIEW0_V1N1_G03_R5A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_R5A_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_R5A_SANITIZE_TEST): $(VIEW0_V1N1_G03_R5A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_R5A_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-r5a-sanitize: $(VIEW0_V1N1_G03_R5A_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_R5A_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 R5A ASan/UBSan partial EXPLICIT_HTML_ELEMENT_ALLOWANCE + sanitized Lexbor qualification"

view0-v1n1-g03-r5a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_r5a_gate.sh

# VIEW0 V1N1 G03 R6A — SCALAR_VALUE_TEXT retained-owner integration (no R6 evaluator)
VIEW0_V1N1_G03_R6A_TEST_SOURCE := tests/c/view0_v1n1_g03_r6a_scalar_value_text_retention_test.c
VIEW0_V1N1_G03_R6A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_r6a_scalar_value_text_retention_adversarial_test.c
VIEW0_V1N1_G03_R6A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r6a_scalar_value_text_retention_test.o
VIEW0_V1N1_G03_R6A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r6a_scalar_value_text_retention_adversarial_test.o
VIEW0_V1N1_G03_R6A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r6a-scalar-value-text-retention-test
VIEW0_V1N1_G03_R6A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r6a-scalar-value-text-retention-adversarial-test

.PHONY: view0-v1n1-g03-r6a-test view0-v1n1-g03-r6a-adversarial-test view0-v1n1-g03-r6a-gate

$(VIEW0_V1N1_G03_R6A_TEST_OBJ): $(VIEW0_V1N1_G03_R6A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R6A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_R6A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R6A_TEST): $(VIEW0_V1N1_G03_R6A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R6A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r6a-test: $(VIEW0_V1N1_G03_R6A_TEST)
	@$(VIEW0_V1N1_G03_R6A_TEST)

$(VIEW0_V1N1_G03_R6A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_R6A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ \
		$(VIEW0_V1N1_G03_R6A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) \
		$(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r6a-adversarial-test: $(VIEW0_V1N1_G03_R6A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_R6A_ADVERSARIAL_TEST)

view0-v1n1-g03-r6a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_r6a_gate.sh


# VIEW0 V1N1 G03 R7A palpable-content warning evaluator
VIEW0_V1N1_G03_R7A_TEST_SOURCE := tests/c/view0_v1n1_g03_r7a_palpable_content_test.c
VIEW0_V1N1_G03_R7A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g03_r7a_palpable_content_adversarial_test.c
VIEW0_V1N1_G03_R7A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r7a_palpable_content_test.o
VIEW0_V1N1_G03_R7A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r7a_palpable_content_adversarial_test.o
VIEW0_V1N1_G03_R7A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r7a-palpable-content-test
VIEW0_V1N1_G03_R7A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r7a-palpable-content-adversarial-test
VIEW0_V1N1_G03_R7A_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r7a-palpable-content-sanitize-test
VIEW0_V1N1_G03_R7A_ATOMICITY_SOURCE := tests/c/view0_v1n1_g03_r7a_global_failure_atomicity_test.c
VIEW0_V1N1_G03_R7A_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g03_r7a_global_failure_atomicity_test.o
VIEW0_V1N1_G03_R7A_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g03-r7a-global-failure-atomicity-test
VIEW0_V1N1_G03_R7A_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_g03_r7a_collect_anchors -Wl,--wrap=arbor_view0_native_lexbor_collect_exact -Wl,--wrap=lxb_html_parser_create

.PHONY: view0-v1n1-g03-r7a-test view0-v1n1-g03-r7a-adversarial-test view0-v1n1-g03-r7a-global-failure-atomicity-test view0-v1n1-g03-r7a-sanitize view0-v1n1-g03-r7a-gate

$(VIEW0_V1N1_G03_R7A_TEST_OBJ): $(VIEW0_V1N1_G03_R7A_TEST_SOURCE) $(VIEW0_V1N1_G03_R7A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R7A_TEST): $(VIEW0_V1N1_G03_R7A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G03_R7A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r7a-test: $(VIEW0_V1N1_G03_R7A_TEST)
	@$(VIEW0_V1N1_G03_R7A_TEST)

$(VIEW0_V1N1_G03_R7A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G03_R7A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R7A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G03_R7A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G03_R7A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g03-r7a-adversarial-test: $(VIEW0_V1N1_G03_R7A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G03_R7A_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G03_R7A_ATOMICITY_OBJ): $(VIEW0_V1N1_G03_R7A_ATOMICITY_SOURCE) $(VIEW0_V1N1_G03_R1A_INTERNAL) $(VIEW0_V1N1_G03_R2A_INTERNAL) $(VIEW0_V1N1_G03_R3A_INTERNAL) $(VIEW0_V1N1_G03_R4A_INTERNAL) $(VIEW0_V1N1_G03_R5A_INTERNAL) $(VIEW0_V1N1_G03_R7A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G03_R7A_ATOMICITY_TEST): $(VIEW0_V1N1_G03_R7A_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G03_R7A_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N1_G03_R7A_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n1-g03-r7a-global-failure-atomicity-test: $(VIEW0_V1N1_G03_R7A_ATOMICITY_TEST)
	@$(VIEW0_V1N1_G03_R7A_ATOMICITY_TEST)

$(VIEW0_V1N1_G03_R7A_SANITIZE_TEST): $(VIEW0_V1N1_G03_R7A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G03_R7A_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g03-r7a-sanitize: $(VIEW0_V1N1_G03_R7A_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G03_R7A_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G03 R7A ASan/UBSan partial PALPABLE_PHRASING_NONEMPTY + sanitized Lexbor qualification"

view0-v1n1-g03-r7a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g03_r7a_gate.sh

# ============================================================
# VIEW0 V1N1 G04 R1A — partial transparent parent-model evaluator
# ============================================================
VIEW0_V1N1_G04_R1A_TEST_SOURCE := tests/c/view0_v1n1_g04_r1a_transparent_parent_model_test.c
VIEW0_V1N1_G04_R1A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g04_r1a_transparent_parent_model_adversarial_test.c
VIEW0_V1N1_G04_R1A_ATOMICITY_SOURCE := tests/c/view0_v1n1_g04_r1a_global_failure_atomicity_test.c
VIEW0_V1N1_G04_R1A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r1a_transparent_parent_model_test.o
VIEW0_V1N1_G04_R1A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r1a_transparent_parent_model_adversarial_test.o
VIEW0_V1N1_G04_R1A_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r1a_global_failure_atomicity_test.o
VIEW0_V1N1_G04_R1A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1a-transparent-parent-model-test
VIEW0_V1N1_G04_R1A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1a-transparent-parent-model-adversarial-test
VIEW0_V1N1_G04_R1A_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1a-global-failure-atomicity-test
VIEW0_V1N1_G04_R1A_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1a-transparent-parent-model-sanitize-test
VIEW0_V1N1_G04_R1A_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_g04_r1a_collect_anchors -Wl,--wrap=arbor_view0_native_lexbor_collect_exact -Wl,--wrap=lxb_html_parser_create

.PHONY: view0-v1n1-g04-r1a-test view0-v1n1-g04-r1a-adversarial-test view0-v1n1-g04-r1a-global-failure-atomicity-test view0-v1n1-g04-r1a-sanitize view0-v1n1-g04-r1a-gate

$(VIEW0_V1N1_G04_R1A_TEST_OBJ): $(VIEW0_V1N1_G04_R1A_TEST_SOURCE) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R1A_TEST): $(VIEW0_V1N1_G04_R1A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R1A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g04-r1a-test: $(VIEW0_V1N1_G04_R1A_TEST)
	@$(VIEW0_V1N1_G04_R1A_TEST)

$(VIEW0_V1N1_G04_R1A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G04_R1A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R1A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G04_R1A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R1A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g04-r1a-adversarial-test: $(VIEW0_V1N1_G04_R1A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G04_R1A_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G04_R1A_ATOMICITY_OBJ): $(VIEW0_V1N1_G04_R1A_ATOMICITY_SOURCE) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R1A_ATOMICITY_TEST): $(VIEW0_V1N1_G04_R1A_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R1A_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N1_G04_R1A_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n1-g04-r1a-global-failure-atomicity-test: $(VIEW0_V1N1_G04_R1A_ATOMICITY_TEST)
	@$(VIEW0_V1N1_G04_R1A_ATOMICITY_TEST)

$(VIEW0_V1N1_G04_R1A_SANITIZE_TEST): $(VIEW0_V1N1_G04_R1A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G04_R1A_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g04-r1a-sanitize: $(VIEW0_V1N1_G04_R1A_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G04_R1A_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G04 R1A ASan/UBSan partial transparent parent-model + sanitized Lexbor qualification"

view0-v1n1-g04-r1a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g04_r1a_gate.sh


# ============================================================
# VIEW0 V1N1 G04 R1B — option/select-source closure over R1A
# ============================================================
VIEW0_V1N1_G04_R1B_TEST_SOURCE := tests/c/view0_v1n1_g04_r1b_transparent_parent_model_test.c
VIEW0_V1N1_G04_R1B_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g04_r1b_transparent_parent_model_adversarial_test.c
VIEW0_V1N1_G04_R1B_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r1b_transparent_parent_model_test.o
VIEW0_V1N1_G04_R1B_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r1b_transparent_parent_model_adversarial_test.o
VIEW0_V1N1_G04_R1B_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1b-transparent-parent-model-test
VIEW0_V1N1_G04_R1B_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1b-transparent-parent-model-adversarial-test
VIEW0_V1N1_G04_R1B_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1b-transparent-parent-model-sanitize-test

.PHONY: view0-v1n1-g04-r1b-test view0-v1n1-g04-r1b-adversarial-test view0-v1n1-g04-r1b-sanitize view0-v1n1-g04-r1b-gate

$(VIEW0_V1N1_G04_R1B_TEST_OBJ): $(VIEW0_V1N1_G04_R1B_TEST_SOURCE) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R1B_TEST): $(VIEW0_V1N1_G04_R1B_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R1B_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g04-r1b-test: $(VIEW0_V1N1_G04_R1B_TEST)
	@$(VIEW0_V1N1_G04_R1B_TEST)

$(VIEW0_V1N1_G04_R1B_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G04_R1B_ADVERSARIAL_SOURCE) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R1B_ADVERSARIAL_TEST): $(VIEW0_V1N1_G04_R1B_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R1B_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g04-r1b-adversarial-test: $(VIEW0_V1N1_G04_R1B_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G04_R1B_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G04_R1B_SANITIZE_TEST): $(VIEW0_V1N1_G04_R1B_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G04_R1B_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g04-r1b-sanitize: $(VIEW0_V1N1_G04_R1B_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G04_R1B_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G04 R1B ASan/UBSan option/select-source closure + sanitized Lexbor qualification"

view0-v1n1-g04-r1b-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g04_r1b_gate.sh

# ============================================================
# VIEW0 V1N1 G04 R1C — explicit scripting-disabled noscript closure
# ============================================================
VIEW0_V1N1_G04_R1C_TEST_SOURCE := tests/c/view0_v1n1_g04_r1c_noscript_transparent_test.c
VIEW0_V1N1_G04_R1C_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g04_r1c_noscript_transparent_adversarial_test.c
VIEW0_V1N1_G04_R1C_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r1c_noscript_transparent_test.o
VIEW0_V1N1_G04_R1C_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r1c_noscript_transparent_adversarial_test.o
VIEW0_V1N1_G04_R1C_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1c-noscript-transparent-test
VIEW0_V1N1_G04_R1C_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1c-noscript-transparent-adversarial-test
VIEW0_V1N1_G04_R1C_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r1c-noscript-transparent-sanitize-test

.PHONY: view0-v1n1-g04-r1c-test view0-v1n1-g04-r1c-adversarial-test view0-v1n1-g04-r1c-sanitize view0-v1n1-g04-r1c-gate

$(VIEW0_V1N1_G04_R1C_TEST_OBJ): $(VIEW0_V1N1_G04_R1C_TEST_SOURCE) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R1C_TEST): $(VIEW0_V1N1_G04_R1C_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R1C_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g04-r1c-test: $(VIEW0_V1N1_G04_R1C_TEST)
	@$(VIEW0_V1N1_G04_R1C_TEST)

$(VIEW0_V1N1_G04_R1C_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G04_R1C_ADVERSARIAL_SOURCE) $(VIEW0_V1N1_G04_R1A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R1C_ADVERSARIAL_TEST): $(VIEW0_V1N1_G04_R1C_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R1C_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g04-r1c-adversarial-test: $(VIEW0_V1N1_G04_R1C_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G04_R1C_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G04_R1C_SANITIZE_TEST): $(VIEW0_V1N1_G04_R1C_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G04_R1C_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g04-r1c-sanitize: $(VIEW0_V1N1_G04_R1C_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G04_R1C_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G04 R1C ASan/UBSan explicit scripting-disabled noscript closure"

view0-v1n1-g04-r1c-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g04_r1c_gate.sh

# ============================================================
# VIEW0 V1N1 G04 R2 — explicit fragment-model parentless fallback
# ============================================================
VIEW0_V1N1_G04_R2_TEST_SOURCE := tests/c/view0_v1n1_g04_r2_parentless_flow_test.c
VIEW0_V1N1_G04_R2_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g04_r2_parentless_flow_adversarial_test.c
VIEW0_V1N1_G04_R2_ATOMICITY_SOURCE := tests/c/view0_v1n1_g04_r2_global_failure_atomicity_test.c
VIEW0_V1N1_G04_R2_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r2_parentless_flow_test.o
VIEW0_V1N1_G04_R2_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r2_parentless_flow_adversarial_test.o
VIEW0_V1N1_G04_R2_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g04_r2_global_failure_atomicity_test.o
VIEW0_V1N1_G04_R2_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r2-parentless-flow-test
VIEW0_V1N1_G04_R2_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r2-parentless-flow-adversarial-test
VIEW0_V1N1_G04_R2_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r2-global-failure-atomicity-test
VIEW0_V1N1_G04_R2_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g04-r2-parentless-flow-sanitize-test
VIEW0_V1N1_G04_R2_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_g04_r2a_collect_fragment_anchors -Wl,--wrap=arbor_view0_native_lexbor_fragment_collect_exact

.PHONY: view0-v1n1-g04-r2-test view0-v1n1-g04-r2-adversarial-test view0-v1n1-g04-r2-global-failure-atomicity-test view0-v1n1-g04-r2-sanitize view0-v1n1-g04-r2-gate

$(VIEW0_V1N1_G04_R2_TEST_OBJ): $(VIEW0_V1N1_G04_R2_TEST_SOURCE) $(VIEW0_V1N1_G04_R2A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R2_TEST): $(VIEW0_V1N1_G04_R2_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R2_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g04-r2-test: $(VIEW0_V1N1_G04_R2_TEST)
	@$(VIEW0_V1N1_G04_R2_TEST)

$(VIEW0_V1N1_G04_R2_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G04_R2_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R2_ADVERSARIAL_TEST): $(VIEW0_V1N1_G04_R2_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R2_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g04-r2-adversarial-test: $(VIEW0_V1N1_G04_R2_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G04_R2_ADVERSARIAL_TEST)

$(VIEW0_V1N1_G04_R2_ATOMICITY_OBJ): $(VIEW0_V1N1_G04_R2_ATOMICITY_SOURCE) $(VIEW0_V1N1_G04_R2A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G04_R2_ATOMICITY_TEST): $(VIEW0_V1N1_G04_R2_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G04_R2_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N1_G04_R2_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n1-g04-r2-global-failure-atomicity-test: $(VIEW0_V1N1_G04_R2_ATOMICITY_TEST)
	@$(VIEW0_V1N1_G04_R2_ATOMICITY_TEST)

$(VIEW0_V1N1_G04_R2_SANITIZE_TEST): $(VIEW0_V1N1_G04_R2_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_G04_R2_ADVERSARIAL_SOURCE) \
		$(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-g04-r2-sanitize: $(VIEW0_V1N1_G04_R2_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_G04_R2_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 G04 R2 ASan/UBSan explicit fragment parentless flow qualification"

view0-v1n1-g04-r2-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g04_r2_gate.sh

# ============================================================
# VIEW0 V1N1 G05 C0 — applicability authority/anchor foundation only
# ============================================================
VIEW0_V1N1_G05_C0_CATALOG_SOURCE := tests/c/view0_v1n1_g05_c0_catalog_test.c
VIEW0_V1N1_G05_C0_ANCHOR_SOURCE := tests/c/view0_v1n1_g05_c0_source_attribute_anchor_test.c
VIEW0_V1N1_G05_C0_CATALOG_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_c0_catalog_test.o
VIEW0_V1N1_G05_C0_ANCHOR_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_c0_anchor_test.o
VIEW0_V1N1_G05_C0_CATALOG_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-c0-catalog-test
VIEW0_V1N1_G05_C0_ANCHOR_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-c0-anchor-test
VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_SOURCE := tests/c/view0_v1n1_g05_c0_sr1_input_state_test.c
VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_c0_sr1_input_state_test.o
VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-c0-sr1-input-state-test

.PHONY: view0-v1n1-g05-c0-catalog-test view0-v1n1-g05-c0-anchor-test view0-v1n1-g05-c0-sr1-input-state-test view0-v1n1-g05-c0-gate

$(VIEW0_V1N1_G05_R4A_OBJ): $(VIEW0_V1N1_G05_R4A_SOURCE) $(VIEW0_V1N1_G05_R4A_INTERNAL) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N1_G05_C0_INTERNAL) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_C0_CATALOG_OBJ): $(VIEW0_V1N1_G05_C0_CATALOG_SOURCE) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_C0_ANCHOR_OBJ): $(VIEW0_V1N1_G05_C0_ANCHOR_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_OBJ): $(VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_SOURCE) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_C0_CATALOG_TEST): $(VIEW0_V1N1_G05_C0_CATALOG_OBJ) $(VIEW0_V1N1_G05_C0_OBJ)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

$(VIEW0_V1N1_G05_C0_ANCHOR_TEST): $(VIEW0_V1N1_G05_C0_ANCHOR_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_C0_ANCHOR_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

$(VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_TEST): $(VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_OBJ) $(VIEW0_V1N1_G05_C0_OBJ)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $^

view0-v1n1-g05-c0-catalog-test: $(VIEW0_V1N1_G05_C0_CATALOG_TEST)
	@$(VIEW0_V1N1_G05_C0_CATALOG_TEST)

view0-v1n1-g05-c0-anchor-test: $(VIEW0_V1N1_G05_C0_ANCHOR_TEST)
	@$(VIEW0_V1N1_G05_C0_ANCHOR_TEST)

view0-v1n1-g05-c0-sr1-input-state-test: $(VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_TEST)
	@$(VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_TEST)

view0-v1n1-g05-c0-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g05_c0_gate.sh

VIEW0_V1N1_G05_R1A_TEST_SOURCE := tests/c/view0_v1n1_g05_r1a_global_attribute_test.c
VIEW0_V1N1_G05_R1A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g05_r1a_global_attribute_adversarial_test.c
VIEW0_V1N1_G05_R1A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r1a_test.o
VIEW0_V1N1_G05_R1A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r1a_adversarial_test.o
VIEW0_V1N1_G05_R1A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r1a-test
VIEW0_V1N1_G05_R1A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r1a-adversarial-test

.PHONY: view0-v1n1-g05-r1a-test view0-v1n1-g05-r1a-adversarial-test

$(VIEW0_V1N1_G05_R1A_TEST_OBJ): $(VIEW0_V1N1_G05_R1A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R1A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G05_R1A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R1A_TEST): $(VIEW0_V1N1_G05_R1A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R1A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

$(VIEW0_V1N1_G05_R1A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G05_R1A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R1A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g05-r1a-test: $(VIEW0_V1N1_G05_R1A_TEST)
	@$(VIEW0_V1N1_G05_R1A_TEST)

view0-v1n1-g05-r1a-adversarial-test: $(VIEW0_V1N1_G05_R1A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G05_R1A_ADVERSARIAL_TEST)


VIEW0_V1N1_G05_R2A_TEST_SOURCE := tests/c/view0_v1n1_g05_r2a_element_attribute_test.c
VIEW0_V1N1_G05_R2A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g05_r2a_element_attribute_adversarial_test.c
VIEW0_V1N1_G05_R2A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r2a_test.o
VIEW0_V1N1_G05_R2A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r2a_adversarial_test.o
VIEW0_V1N1_G05_R2A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r2a-test
VIEW0_V1N1_G05_R2A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r2a-adversarial-test

.PHONY: view0-v1n1-g05-r2a-test view0-v1n1-g05-r2a-adversarial-test

$(VIEW0_V1N1_G05_R2A_TEST_OBJ): $(VIEW0_V1N1_G05_R2A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R2A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G05_R2A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R2A_TEST): $(VIEW0_V1N1_G05_R2A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R2A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

$(VIEW0_V1N1_G05_R2A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G05_R2A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R2A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-g05-r2a-test: $(VIEW0_V1N1_G05_R2A_TEST)
	@$(VIEW0_V1N1_G05_R2A_TEST)

view0-v1n1-g05-r2a-adversarial-test: $(VIEW0_V1N1_G05_R2A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G05_R2A_ADVERSARIAL_TEST)

.PHONY: view0-v1n1-g05-c0-sr1-gate
view0-v1n1-g05-c0-sr1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g05_c0_sr1_gate.sh

# VIEW0 V1N1 G05 R3A conditional attribute applicability qualification.
VIEW0_V1N1_G05_R3A_TEST_SOURCE := tests/c/view0_v1n1_g05_r3a_conditional_applicability_test.c
VIEW0_V1N1_G05_R3A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g05_r3a_conditional_applicability_adversarial_test.c
VIEW0_V1N1_G05_R3A_MATRIX_SOURCE := tests/c/view0_v1n1_g05_r3a_conditional_matrix_test.c
VIEW0_V1N1_G05_R3A_ATOMICITY_SOURCE := tests/c/view0_v1n1_g05_r3a_global_failure_atomicity_test.c
VIEW0_V1N1_G05_R3A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r3a_test.o
VIEW0_V1N1_G05_R3A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r3a_adversarial_test.o
VIEW0_V1N1_G05_R3A_MATRIX_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r3a_matrix_test.o
VIEW0_V1N1_G05_R3A_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r3a_global_failure_atomicity_test.o
VIEW0_V1N1_G05_R3A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r3a-test
VIEW0_V1N1_G05_R3A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r3a-adversarial-test
VIEW0_V1N1_G05_R3A_MATRIX_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r3a-matrix-test
VIEW0_V1N1_G05_R3A_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r3a-global-failure-atomicity-test
VIEW0_V1N1_G05_R3A_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_g05_r3a_collect_anchors -Wl,--wrap=arbor_view0_native_lexbor_collect_exact -Wl,--wrap=lxb_html_parser_create

.PHONY: view0-v1n1-g05-r3a-test view0-v1n1-g05-r3a-adversarial-test view0-v1n1-g05-r3a-matrix-test view0-v1n1-g05-r3a-global-failure-atomicity-test view0-v1n1-g05-r3a-gate

$(VIEW0_V1N1_G05_R3A_TEST_OBJ): $(VIEW0_V1N1_G05_R3A_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R3A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G05_R3A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R3A_MATRIX_OBJ): $(VIEW0_V1N1_G05_R3A_MATRIX_SOURCE) $(VIEW0_V1N1_G05_R3A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R3A_ATOMICITY_OBJ): $(VIEW0_V1N1_G05_R3A_ATOMICITY_SOURCE) $(VIEW0_V1N1_G05_R3A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G05_R3A_TEST): $(VIEW0_V1N1_G05_R3A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R3A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

$(VIEW0_V1N1_G05_R3A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G05_R3A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R3A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

$(VIEW0_V1N1_G05_R3A_MATRIX_TEST): $(VIEW0_V1N1_G05_R3A_MATRIX_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R3A_MATRIX_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

$(VIEW0_V1N1_G05_R3A_ATOMICITY_TEST): $(VIEW0_V1N1_G05_R3A_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R3A_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N1_G05_R3A_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n1-g05-r3a-test: $(VIEW0_V1N1_G05_R3A_TEST)
	@$(VIEW0_V1N1_G05_R3A_TEST)

view0-v1n1-g05-r3a-adversarial-test: $(VIEW0_V1N1_G05_R3A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G05_R3A_ADVERSARIAL_TEST)

view0-v1n1-g05-r3a-matrix-test: $(VIEW0_V1N1_G05_R3A_MATRIX_TEST)
	@$(VIEW0_V1N1_G05_R3A_MATRIX_TEST)

view0-v1n1-g05-r3a-global-failure-atomicity-test: $(VIEW0_V1N1_G05_R3A_ATOMICITY_TEST)
	@$(VIEW0_V1N1_G05_R3A_ATOMICITY_TEST)

view0-v1n1-g05-r3a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g05_r3a_gate.sh


# VIEW0 V1N1 G05 R4A body Window-event attribute applicability qualification.
VIEW0_V1N1_G05_R4A_TEST_SOURCE := tests/c/view0_v1n1_g05_r4a_body_window_event_test.c
VIEW0_V1N1_G05_R4A_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g05_r4a_body_window_event_adversarial_test.c
VIEW0_V1N1_G05_R4A_ATOMICITY_SOURCE := tests/c/view0_v1n1_g05_r4a_global_failure_atomicity_test.c
VIEW0_V1N1_G05_R4A_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r4a_test.o
VIEW0_V1N1_G05_R4A_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r4a_adversarial_test.o
VIEW0_V1N1_G05_R4A_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g05_r4a_global_failure_atomicity_test.o
VIEW0_V1N1_G05_R4A_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r4a-test
VIEW0_V1N1_G05_R4A_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r4a-adversarial-test
VIEW0_V1N1_G05_R4A_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g05-r4a-global-failure-atomicity-test
VIEW0_V1N1_G05_R4A_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_g05_r4a_collect_anchors -Wl,--wrap=arbor_view0_native_lexbor_collect_exact -Wl,--wrap=lxb_html_parser_create

.PHONY: view0-v1n1-g05-r4a-test view0-v1n1-g05-r4a-adversarial-test view0-v1n1-g05-r4a-global-failure-atomicity-test view0-v1n1-g05-r4a-gate

$(VIEW0_V1N1_G05_R4A_TEST_OBJ): $(VIEW0_V1N1_G05_R4A_TEST_SOURCE) $(VIEW0_V1N1_G05_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N1_G05_R4A_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G05_R4A_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N1_G05_R4A_ATOMICITY_OBJ): $(VIEW0_V1N1_G05_R4A_ATOMICITY_SOURCE) $(VIEW0_V1N1_G05_R4A_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N1_G05_R4A_TEST): $(VIEW0_V1N1_G05_R4A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R4A_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N1_G05_R4A_ADVERSARIAL_TEST): $(VIEW0_V1N1_G05_R4A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R4A_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N1_G05_R4A_ATOMICITY_TEST): $(VIEW0_V1N1_G05_R4A_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G05_R4A_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N1_G05_R4A_ATOMICITY_WRAP_LDFLAG) -lm
view0-v1n1-g05-r4a-test: $(VIEW0_V1N1_G05_R4A_TEST)
	@$(VIEW0_V1N1_G05_R4A_TEST)
view0-v1n1-g05-r4a-adversarial-test: $(VIEW0_V1N1_G05_R4A_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G05_R4A_ADVERSARIAL_TEST)
view0-v1n1-g05-r4a-global-failure-atomicity-test: $(VIEW0_V1N1_G05_R4A_ATOMICITY_TEST)
	@$(VIEW0_V1N1_G05_R4A_ATOMICITY_TEST)
view0-v1n1-g05-r4a-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g05_r4a_gate.sh

# VIEW0 V1N1 G06 C0 bounded microsyntax foundation (zero G06 diagnostics).
VIEW0_V1N1_G06_C0_TEST_SOURCE := tests/c/view0_v1n1_g06_c0_validator_test.c
VIEW0_V1N1_G06_C0_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g06_c0_validator_test.o
VIEW0_V1N1_G06_C0_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g06-c0-validator-test

.PHONY: view0-v1n1-g06-c0-test view0-v1n1-g06-c0-gate

$(VIEW0_V1N1_G06_C0_TEST_OBJ): $(VIEW0_V1N1_G06_C0_TEST_SOURCE) $(VIEW0_V1N1_G06_C0_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G06_C0_TEST): $(VIEW0_V1N1_G06_C0_TEST_OBJ) $(VIEW0_V1N1_G06_C0_OBJ)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G06_C0_TEST_OBJ) $(VIEW0_V1N1_G06_C0_OBJ)

view0-v1n1-g06-c0-test: $(VIEW0_V1N1_G06_C0_TEST)
	@$(VIEW0_V1N1_G06_C0_TEST)

view0-v1n1-g06-c0-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g06_c0_gate.sh

# VIEW0 V1N2 C0 exact authority/resource foundation (zero V1N2 diagnostics).
VIEW0_V1N2_C0_TEST_SOURCE := tests/c/view0_v1n2_c0_foundation_test.c
VIEW0_V1N2_C0_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_c0_foundation_test.o
VIEW0_V1N2_C0_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-c0-foundation-test

.PHONY: view0-v1n2-c0-test view0-v1n2-c0-gate

$(VIEW0_V1N2_C0_TEST_OBJ): $(VIEW0_V1N2_C0_TEST_SOURCE) $(VIEW0_V1N2_C0_INTERNAL) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_C0_TEST): $(VIEW0_V1N2_C0_TEST_OBJ) $(VIEW0_V1N2_C0_OBJ)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_C0_TEST_OBJ) $(VIEW0_V1N2_C0_OBJ)

view0-v1n2-c0-test: $(VIEW0_V1N2_C0_TEST)
	@$(VIEW0_V1N2_C0_TEST)

view0-v1n2-c0-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n2_c0_gate.sh

# VIEW0 V1N2 G07 — five static link-semantics diagnostics.
VIEW0_V1N2_G07_TEST_SOURCE := tests/c/view0_v1n2_g07_test.c
VIEW0_V1N2_G07_ADVERSARIAL_SOURCE := tests/c/view0_v1n2_g07_adversarial_test.c
VIEW0_V1N2_G07_ATOMICITY_SOURCE := tests/c/view0_v1n2_g07_global_failure_atomicity_test.c
VIEW0_V1N2_G07_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g07_test.o
VIEW0_V1N2_G07_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g07_adversarial_test.o
VIEW0_V1N2_G07_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g07_global_failure_atomicity_test.o
VIEW0_V1N2_G07_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g07-test
VIEW0_V1N2_G07_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g07-adversarial-test
VIEW0_V1N2_G07_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g07-global-failure-atomicity-test
VIEW0_V1N2_G07_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g07-sanitize-test
VIEW0_V1N2_G07_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_v1n2_g07_collect_anchors -Wl,--wrap=arbor_view0_native_lexbor_collect_exact

.PHONY: view0-v1n2-g07-test view0-v1n2-g07-adversarial-test view0-v1n2-g07-global-failure-atomicity-test view0-v1n2-g07-sanitize view0-v1n2-g07-gate

$(VIEW0_V1N2_G07_TEST_OBJ): $(VIEW0_V1N2_G07_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G07_ADVERSARIAL_OBJ): $(VIEW0_V1N2_G07_ADVERSARIAL_SOURCE) $(VIEW0_V1N2_G07_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G07_ATOMICITY_OBJ): $(VIEW0_V1N2_G07_ATOMICITY_SOURCE) $(VIEW0_V1N2_G07_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G07_TEST): $(VIEW0_V1N2_G07_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G07_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G07_ADVERSARIAL_TEST): $(VIEW0_V1N2_G07_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G07_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G07_ATOMICITY_TEST): $(VIEW0_V1N2_G07_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G07_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N2_G07_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n2-g07-test: $(VIEW0_V1N2_G07_TEST)
	@$(VIEW0_V1N2_G07_TEST)
view0-v1n2-g07-adversarial-test: $(VIEW0_V1N2_G07_ADVERSARIAL_TEST)
	@$(VIEW0_V1N2_G07_ADVERSARIAL_TEST)
view0-v1n2-g07-global-failure-atomicity-test: $(VIEW0_V1N2_G07_ATOMICITY_TEST)
	@$(VIEW0_V1N2_G07_ATOMICITY_TEST)

$(VIEW0_V1N2_G07_SANITIZE_TEST): $(VIEW0_V1N2_G07_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N2_G07_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n2-g07-sanitize: $(VIEW0_V1N2_G07_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N2_G07_SANITIZE_TEST)
	@echo "VIEW0_V1N2_G07_SANITIZE=PASS"
view0-v1n2-g07-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n2_g07_gate.sh

# VIEW0 V1N2 G08 — twelve static embedded-content diagnostics.
VIEW0_V1N2_G08_TEST_SOURCE := tests/c/view0_v1n2_g08_test.c
VIEW0_V1N2_G08_ADVERSARIAL_SOURCE := tests/c/view0_v1n2_g08_adversarial_test.c
VIEW0_V1N2_G08_ATOMICITY_SOURCE := tests/c/view0_v1n2_g08_global_failure_atomicity_test.c
VIEW0_V1N2_G08_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g08_test.o
VIEW0_V1N2_G08_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g08_adversarial_test.o
VIEW0_V1N2_G08_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g08_global_failure_atomicity_test.o
VIEW0_V1N2_G08_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g08-test
VIEW0_V1N2_G08_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g08-adversarial-test
VIEW0_V1N2_G08_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g08-global-failure-atomicity-test
VIEW0_V1N2_G08_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g08-sanitize-test
VIEW0_V1N2_G08_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_v1n2_g08_collect_anchors -Wl,--wrap=arbor_view0_native_lexbor_collect_exact

.PHONY: view0-v1n2-g08-test view0-v1n2-g08-adversarial-test view0-v1n2-g08-global-failure-atomicity-test view0-v1n2-g08-sanitize view0-v1n2-g08-gate

$(VIEW0_V1N2_G08_TEST_OBJ): $(VIEW0_V1N2_G08_TEST_SOURCE) $(VIEW0_V1N2_G08_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G08_ADVERSARIAL_OBJ): $(VIEW0_V1N2_G08_ADVERSARIAL_SOURCE) $(VIEW0_V1N2_G08_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G08_ATOMICITY_OBJ): $(VIEW0_V1N2_G08_ATOMICITY_SOURCE) $(VIEW0_V1N2_G08_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G08_TEST): $(VIEW0_V1N2_G08_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G08_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G08_ADVERSARIAL_TEST): $(VIEW0_V1N2_G08_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G08_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G08_ATOMICITY_TEST): $(VIEW0_V1N2_G08_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G08_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N2_G08_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n2-g08-test: $(VIEW0_V1N2_G08_TEST)
	@$(VIEW0_V1N2_G08_TEST)
view0-v1n2-g08-adversarial-test: $(VIEW0_V1N2_G08_ADVERSARIAL_TEST)
	@$(VIEW0_V1N2_G08_ADVERSARIAL_TEST)
view0-v1n2-g08-global-failure-atomicity-test: $(VIEW0_V1N2_G08_ATOMICITY_TEST)
	@$(VIEW0_V1N2_G08_ATOMICITY_TEST)

$(VIEW0_V1N2_G08_SANITIZE_TEST): $(VIEW0_V1N2_G08_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N2_G08_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n2-g08-sanitize: $(VIEW0_V1N2_G08_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N2_G08_SANITIZE_TEST)
	@echo "VIEW0_V1N2_G08_SANITIZE=PASS"
view0-v1n2-g08-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n2_g08_gate.sh

# VIEW0 V1N2 G09 — six static table-semantics diagnostics.
VIEW0_V1N2_G09_TEST_SOURCE := tests/c/view0_v1n2_g09_test.c
VIEW0_V1N2_G09_ADVERSARIAL_SOURCE := tests/c/view0_v1n2_g09_adversarial_test.c
VIEW0_V1N2_G09_ATOMICITY_SOURCE := tests/c/view0_v1n2_g09_global_failure_atomicity_test.c
VIEW0_V1N2_G09_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g09_test.o
VIEW0_V1N2_G09_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g09_adversarial_test.o
VIEW0_V1N2_G09_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g09_global_failure_atomicity_test.o
VIEW0_V1N2_G09_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g09-test
VIEW0_V1N2_G09_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g09-adversarial-test
VIEW0_V1N2_G09_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g09-global-failure-atomicity-test
VIEW0_V1N2_G09_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g09-sanitize-test
VIEW0_V1N2_G09_SANITIZE_NATIVE_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g09-sanitize-native.o
VIEW0_V1N2_G09_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_v1n2_g09_support_calloc -Wl,--wrap=arbor_view0_native_v1n2_g09_collect_anchors

.PHONY: view0-v1n2-g09-test view0-v1n2-g09-adversarial-test view0-v1n2-g09-global-failure-atomicity-test view0-v1n2-g09-sanitize view0-v1n2-g09-gate

$(VIEW0_V1N2_G09_TEST_OBJ): $(VIEW0_V1N2_G09_TEST_SOURCE) $(VIEW0_V1N2_G09_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G09_ADVERSARIAL_OBJ): $(VIEW0_V1N2_G09_ADVERSARIAL_SOURCE) $(VIEW0_V1N2_G09_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G09_ATOMICITY_OBJ): $(VIEW0_V1N2_G09_ATOMICITY_SOURCE) $(VIEW0_V1N2_G09_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G09_TEST): $(VIEW0_V1N2_G09_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G09_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G09_ADVERSARIAL_TEST): $(VIEW0_V1N2_G09_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G09_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G09_ATOMICITY_TEST): $(VIEW0_V1N2_G09_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G09_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N2_G09_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n2-g09-test: $(VIEW0_V1N2_G09_TEST)
	@$(VIEW0_V1N2_G09_TEST)
view0-v1n2-g09-adversarial-test: $(VIEW0_V1N2_G09_ADVERSARIAL_TEST)
	@$(VIEW0_V1N2_G09_ADVERSARIAL_TEST)
view0-v1n2-g09-global-failure-atomicity-test: $(VIEW0_V1N2_G09_ATOMICITY_TEST)
	@$(VIEW0_V1N2_G09_ATOMICITY_TEST)

$(VIEW0_V1N2_G09_SANITIZE_NATIVE_OBJ): $(VIEW0_V1N0_NATIVE_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer -c $< -o $@

$(VIEW0_V1N2_G09_SANITIZE_TEST): $(VIEW0_V1N2_G09_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_V1N2_G09_SANITIZE_NATIVE_OBJ) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N2_G09_ADVERSARIAL_SOURCE) $(filter-out $(VIEW0_V1N0_NATIVE_SOURCE),$(VIEW0_V1N0_NATIVE_RULE_SOURCES)) $(VIEW0_V1N2_G09_SANITIZE_NATIVE_OBJ) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n2-g09-sanitize: $(VIEW0_V1N2_G09_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=$${ARBORCORE_ASAN_DETECT_LEAKS:-1}:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N2_G09_SANITIZE_TEST)
	@echo "VIEW0_V1N2_G09_SANITIZE=PASS"
view0-v1n2-g09-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n2_g09_gate.sh

# VIEW0 V1N2 G10 — thirteen static form-semantics diagnostics.
VIEW0_V1N2_G10_TEST_SOURCE := tests/c/view0_v1n2_g10_test.c
VIEW0_V1N2_G10_ADVERSARIAL_SOURCE := tests/c/view0_v1n2_g10_adversarial_test.c
VIEW0_V1N2_G10_ATOMICITY_SOURCE := tests/c/view0_v1n2_g10_global_failure_atomicity_test.c
VIEW0_V1N2_G10_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g10_test.o
VIEW0_V1N2_G10_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g10_adversarial_test.o
VIEW0_V1N2_G10_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g10_global_failure_atomicity_test.o
VIEW0_V1N2_G10_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g10-test
VIEW0_V1N2_G10_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g10-adversarial-test
VIEW0_V1N2_G10_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g10-global-failure-atomicity-test
VIEW0_V1N2_G10_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g10-sanitize-test
VIEW0_V1N2_G10_SANITIZE_NATIVE_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g10-sanitize-native.o
VIEW0_V1N2_G10_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_v1n2_g10_support_calloc -Wl,--wrap=arbor_view0_native_v1n2_g10_collect_anchors -Wl,--wrap=arbor_view0_native_v1n2_g10_measure

.PHONY: view0-v1n2-g10-test view0-v1n2-g10-adversarial-test view0-v1n2-g10-global-failure-atomicity-test view0-v1n2-g10-sanitize view0-v1n2-g10-gate

$(VIEW0_V1N2_G10_TEST_OBJ): $(VIEW0_V1N2_G10_TEST_SOURCE) $(VIEW0_V1N2_G10_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G10_ADVERSARIAL_OBJ): $(VIEW0_V1N2_G10_ADVERSARIAL_SOURCE) $(VIEW0_V1N2_G10_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G10_ATOMICITY_OBJ): $(VIEW0_V1N2_G10_ATOMICITY_SOURCE) $(VIEW0_V1N2_G10_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G10_TEST): $(VIEW0_V1N2_G10_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G10_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G10_ADVERSARIAL_TEST): $(VIEW0_V1N2_G10_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G10_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G10_ATOMICITY_TEST): $(VIEW0_V1N2_G10_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G10_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N2_G10_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n2-g10-test: $(VIEW0_V1N2_G10_TEST)
	@$(VIEW0_V1N2_G10_TEST)
view0-v1n2-g10-adversarial-test: $(VIEW0_V1N2_G10_ADVERSARIAL_TEST)
	@$(VIEW0_V1N2_G10_ADVERSARIAL_TEST)
view0-v1n2-g10-global-failure-atomicity-test: $(VIEW0_V1N2_G10_ATOMICITY_TEST)
	@$(VIEW0_V1N2_G10_ATOMICITY_TEST)

$(VIEW0_V1N2_G10_SANITIZE_NATIVE_OBJ): $(VIEW0_V1N0_NATIVE_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer -c $< -o $@

$(VIEW0_V1N2_G10_SANITIZE_TEST): $(VIEW0_V1N2_G10_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_V1N2_G10_SANITIZE_NATIVE_OBJ) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N2_G10_ADVERSARIAL_SOURCE) $(filter-out $(VIEW0_V1N0_NATIVE_SOURCE),$(VIEW0_V1N0_NATIVE_RULE_SOURCES)) $(VIEW0_V1N2_G10_SANITIZE_NATIVE_OBJ) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n2-g10-sanitize: $(VIEW0_V1N2_G10_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=$${ARBORCORE_ASAN_DETECT_LEAKS:-1}:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N2_G10_SANITIZE_TEST)
	@echo "VIEW0_V1N2_G10_SANITIZE=PASS"
view0-v1n2-g10-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n2_g10_gate.sh

# VIEW0 V1N2 G11 — two static interactive-element diagnostics.
VIEW0_V1N2_G11_TEST_SOURCE := tests/c/view0_v1n2_g11_test.c
VIEW0_V1N2_G11_ADVERSARIAL_SOURCE := tests/c/view0_v1n2_g11_adversarial_test.c
VIEW0_V1N2_G11_ATOMICITY_SOURCE := tests/c/view0_v1n2_g11_global_failure_atomicity_test.c
VIEW0_V1N2_G11_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g11_test.o
VIEW0_V1N2_G11_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g11_adversarial_test.o
VIEW0_V1N2_G11_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2_g11_global_failure_atomicity_test.o
VIEW0_V1N2_G11_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g11-test
VIEW0_V1N2_G11_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g11-adversarial-test
VIEW0_V1N2_G11_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g11-global-failure-atomicity-test
VIEW0_V1N2_G11_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g11-sanitize-test
VIEW0_V1N2_G11_SANITIZE_NATIVE_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n2-g11-sanitize-native.o
VIEW0_V1N2_G11_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_v1n2_g11_support_calloc -Wl,--wrap=arbor_view0_native_v1n2_g11_collect_anchors

.PHONY: view0-v1n2-g11-test view0-v1n2-g11-adversarial-test view0-v1n2-g11-global-failure-atomicity-test view0-v1n2-g11-sanitize view0-v1n2-g11-gate

$(VIEW0_V1N2_G11_TEST_OBJ): $(VIEW0_V1N2_G11_TEST_SOURCE) $(VIEW0_V1N2_G11_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G11_ADVERSARIAL_OBJ): $(VIEW0_V1N2_G11_ADVERSARIAL_SOURCE) $(VIEW0_V1N2_G11_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N2_G11_ATOMICITY_OBJ): $(VIEW0_V1N2_G11_ATOMICITY_SOURCE) $(VIEW0_V1N2_G11_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N2_G11_TEST): $(VIEW0_V1N2_G11_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G11_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G11_ADVERSARIAL_TEST): $(VIEW0_V1N2_G11_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G11_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N2_G11_ATOMICITY_TEST): $(VIEW0_V1N2_G11_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N2_G11_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N2_G11_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n2-g11-test: $(VIEW0_V1N2_G11_TEST)
	@$(VIEW0_V1N2_G11_TEST)
view0-v1n2-g11-adversarial-test: $(VIEW0_V1N2_G11_ADVERSARIAL_TEST)
	@$(VIEW0_V1N2_G11_ADVERSARIAL_TEST)
view0-v1n2-g11-global-failure-atomicity-test: $(VIEW0_V1N2_G11_ATOMICITY_TEST)
	@$(VIEW0_V1N2_G11_ATOMICITY_TEST)

$(VIEW0_V1N2_G11_SANITIZE_NATIVE_OBJ): $(VIEW0_V1N0_NATIVE_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS_BASE) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -c $< -o $@
$(VIEW0_V1N2_G11_SANITIZE_TEST): $(VIEW0_V1N2_G11_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_V1N2_G11_SANITIZE_NATIVE_OBJ) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS_BASE) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer $(VIEW0_V1N2_G11_ADVERSARIAL_SOURCE) $(filter-out $(VIEW0_V1N0_NATIVE_SOURCE),$(VIEW0_V1N0_NATIVE_RULE_SOURCES)) $(VIEW0_V1N2_G11_SANITIZE_NATIVE_OBJ) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) $(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@
view0-v1n2-g11-sanitize: $(VIEW0_V1N2_G11_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=$${ARBORCORE_ASAN_DETECT_LEAKS:-1}:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N2_G11_SANITIZE_TEST)
	@echo "VIEW0_V1N2_G11_SANITIZE=PASS"
view0-v1n2-g11-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n2_g11_gate.sh

# VIEW0 V1N1 G06 R1-R17 internally checkpointed diagnostic wave.
VIEW0_V1N1_G06_WAVE_TEST_SOURCE := tests/c/view0_v1n1_g06_wave_test.c
VIEW0_V1N1_G06_WAVE_ADVERSARIAL_SOURCE := tests/c/view0_v1n1_g06_wave_adversarial_test.c
VIEW0_V1N1_G06_WAVE_ATOMICITY_SOURCE := tests/c/view0_v1n1_g06_wave_global_failure_atomicity_test.c
VIEW0_V1N1_G06_WAVE_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g06_wave_test.o
VIEW0_V1N1_G06_WAVE_ADVERSARIAL_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g06_wave_adversarial_test.o
VIEW0_V1N1_G06_WAVE_ATOMICITY_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/g06_wave_global_failure_atomicity_test.o
VIEW0_V1N1_G06_WAVE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g06-wave-test
VIEW0_V1N1_G06_WAVE_ADVERSARIAL_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g06-wave-adversarial-test
VIEW0_V1N1_G06_WAVE_ATOMICITY_TEST := $(VIEW0_V1N0_NATIVE_DIR)/g06-wave-global-failure-atomicity-test
VIEW0_V1N1_G06_WAVE_ATOMICITY_WRAP_LDFLAG := -Wl,--wrap=arbor_view0_native_g06_collect_anchors -Wl,--wrap=arbor_view0_native_lexbor_collect_exact -Wl,--wrap=lxb_html_parser_create

.PHONY: view0-v1n1-g06-wave-test view0-v1n1-g06-wave-adversarial-test view0-v1n1-g06-wave-global-failure-atomicity-test view0-v1n1-g06-wave-gate

$(VIEW0_V1N1_G06_WAVE_TEST_OBJ): $(VIEW0_V1N1_G06_WAVE_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N1_G06_WAVE_ADVERSARIAL_OBJ): $(VIEW0_V1N1_G06_WAVE_ADVERSARIAL_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@
$(VIEW0_V1N1_G06_WAVE_ATOMICITY_OBJ): $(VIEW0_V1N1_G06_WAVE_ATOMICITY_SOURCE) $(VIEW0_V1N1_G06_INTERNAL) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_G06_WAVE_TEST): $(VIEW0_V1N1_G06_WAVE_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G06_WAVE_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N1_G06_WAVE_ADVERSARIAL_TEST): $(VIEW0_V1N1_G06_WAVE_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G06_WAVE_ADVERSARIAL_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm
$(VIEW0_V1N1_G06_WAVE_ATOMICITY_TEST): $(VIEW0_V1N1_G06_WAVE_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_G06_WAVE_ATOMICITY_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) $(VIEW0_V1N1_G06_WAVE_ATOMICITY_WRAP_LDFLAG) -lm

view0-v1n1-g06-wave-test: $(VIEW0_V1N1_G06_WAVE_TEST)
	@$(VIEW0_V1N1_G06_WAVE_TEST)
view0-v1n1-g06-wave-adversarial-test: $(VIEW0_V1N1_G06_WAVE_ADVERSARIAL_TEST)
	@$(VIEW0_V1N1_G06_WAVE_ADVERSARIAL_TEST)
view0-v1n1-g06-wave-global-failure-atomicity-test: $(VIEW0_V1N1_G06_WAVE_ATOMICITY_TEST)
	@$(VIEW0_V1N1_G06_WAVE_ATOMICITY_TEST)
view0-v1n1-g06-wave-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_g06_wave_gate.sh

# VIEW0 V1N1 RC1 — exact G02-G06 dependency reconciliation.
VIEW0_V1N1_RC1_TEST_SOURCE := tests/c/view0_v1n1_rc1_dependency_reconciliation_test.c
VIEW0_V1N1_RC1_TEST_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/rc1_dependency_reconciliation_test.o
VIEW0_V1N1_RC1_TEST := $(VIEW0_V1N0_NATIVE_DIR)/rc1-dependency-reconciliation-test
VIEW0_V1N1_RC1_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/rc1-dependency-reconciliation-sanitize-test

.PHONY: view0-v1n1-rc1-test view0-v1n1-rc1-sanitize view0-v1n1-rc1-gate

$(VIEW0_V1N1_RC1_TEST_OBJ): $(VIEW0_V1N1_RC1_TEST_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_V1N1_RC1_TEST): $(VIEW0_V1N1_RC1_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_V1N1_RC1_TEST_OBJ) $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm

view0-v1n1-rc1-test: $(VIEW0_V1N1_RC1_TEST)
	@$(VIEW0_V1N1_RC1_TEST)

$(VIEW0_V1N1_RC1_SANITIZE_TEST): $(VIEW0_V1N1_RC1_TEST_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_HEADER) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(filter-out -O2,$(ARBORCORE_C_CFLAGS)) -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(VIEW0_V1N1_RC1_TEST_SOURCE) $(VIEW0_V1N0_NATIVE_RULE_SOURCES) \
		$(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) \
		$(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@

view0-v1n1-rc1-sanitize: $(VIEW0_V1N1_RC1_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(VIEW0_V1N1_RC1_SANITIZE_TEST)
	@echo "PASS: VIEW0 V1N1 RC1 ASan/UBSan dependency reconciliation"

view0-v1n1-rc1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n1_rc1_gate.sh

# VIEW0 V1N3 C1 — isolated thirty-rule native-C construction.
VIEW0_V1N3_TEST_STEMS := \
	c0_foundation \
	ecma_frontend ecma_frontend_adversarial ecma_frontend_global_failure_atomicity ecma_unicode_generation \
	g12 g12_adversarial g12_global_failure_atomicity \
	g13 g13_adversarial g13_global_failure_atomicity \
	g14 g14_adversarial g14_global_failure_atomicity \
	g15 g15_adversarial g15_global_failure_atomicity \
	g16 g16_adversarial g16_global_failure_atomicity
VIEW0_V1N3_TEST_BINS := $(addprefix $(VIEW0_V1N0_NATIVE_DIR)/v1n3-,$(subst _,-,$(VIEW0_V1N3_TEST_STEMS)))
VIEW0_V1N3_SANITIZE_NATIVE_OBJ := $(VIEW0_V1N0_NATIVE_DIR)/v1n3-c1-sanitize-native.o
VIEW0_V1N3_SANITIZE_TEST := $(VIEW0_V1N0_NATIVE_DIR)/v1n3-c1-sanitize-test

.PHONY: view0-v1n3-c0-test view0-v1n3-ecma-test view0-v1n3-g12-test view0-v1n3-g13-test view0-v1n3-g14-test view0-v1n3-g15-test view0-v1n3-g16-test view0-v1n3-c1-test view0-v1n3-c1-sanitize view0-v1n3-c1-gate

define VIEW0_V1N3_TEST_template
$(VIEW0_V1N0_NATIVE_DIR)/v1n3-$(subst _,-,$(1)): tests/c/view0_v1n3_$(1)_test.c $(VIEW0_V1N0_NATIVE_RULE_OBJS) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_RELEASE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$$(CC) $$(VIEW0_V1N0_CPPFLAGS) -I$$(VIEW0_V1N0_SOURCE_DIR) $$(ARBORCORE_C_CFLAGS) $$< $$(VIEW0_V1N0_NATIVE_RULE_OBJS) $$(VIEW0_C1_LIB) $$(C_RUNTIME_LIB) $$(STATIC_LIB) $$(VIEW0_V1N0_LEXBOR_RELEASE_LIB) $$(ARBORCORE_C_LDFLAGS) $$(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -lm -o $$@
endef
$(foreach stem,$(VIEW0_V1N3_TEST_STEMS),$(eval $(call VIEW0_V1N3_TEST_template,$(stem))))

view0-v1n3-c0-test: $(VIEW0_V1N0_NATIVE_DIR)/v1n3-c0-foundation
	@$<
view0-v1n3-ecma-test: $(VIEW0_V1N0_NATIVE_DIR)/v1n3-ecma-frontend $(VIEW0_V1N0_NATIVE_DIR)/v1n3-ecma-frontend-adversarial $(VIEW0_V1N0_NATIVE_DIR)/v1n3-ecma-frontend-global-failure-atomicity $(VIEW0_V1N0_NATIVE_DIR)/v1n3-ecma-unicode-generation
	@for test in $^; do $$test; done
view0-v1n3-g12-test: $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g12 $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g12-adversarial $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g12-global-failure-atomicity
	@for test in $^; do $$test; done
view0-v1n3-g13-test: $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g13 $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g13-adversarial $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g13-global-failure-atomicity
	@for test in $^; do $$test; done
view0-v1n3-g14-test: $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g14 $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g14-adversarial $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g14-global-failure-atomicity
	@for test in $^; do $$test; done
view0-v1n3-g15-test: $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g15 $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g15-adversarial $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g15-global-failure-atomicity
	@for test in $^; do $$test; done
view0-v1n3-g16-test: $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g16 $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g16-adversarial $(VIEW0_V1N0_NATIVE_DIR)/v1n3-g16-global-failure-atomicity
	@for test in $^; do $$test; done
view0-v1n3-c1-test: view0-v1n3-c0-test view0-v1n3-ecma-test view0-v1n3-g12-test view0-v1n3-g13-test view0-v1n3-g14-test view0-v1n3-g15-test view0-v1n3-g16-test
	@echo "VIEW0_V1N3_C1_RULES=30_OF_30"
	@echo "PASS: VIEW0 V1N3 C1 complete isolated native-C test wave"

$(VIEW0_V1N3_SANITIZE_NATIVE_OBJ): $(VIEW0_V1N0_NATIVE_SOURCE) $(VIEW0_V1N0_HEADER) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) $(ARBORCORE_C_CFLAGS_BASE) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -c $< -o $@
$(VIEW0_V1N3_SANITIZE_TEST): tests/c/view0_v1n3_g16_adversarial_test.c $(VIEW0_V1N0_NATIVE_RULE_SOURCES) $(VIEW0_V1N3_SANITIZE_NATIVE_OBJ) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) | $(VIEW0_V1N0_NATIVE_DIR)
	$(CC) $(VIEW0_V1N0_CPPFLAGS) -I$(VIEW0_V1N0_SOURCE_DIR) $(ARBORCORE_C_CFLAGS_BASE) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer $< $(filter-out $(VIEW0_V1N0_NATIVE_SOURCE),$(VIEW0_V1N0_NATIVE_RULE_SOURCES)) $(VIEW0_V1N3_SANITIZE_NATIVE_OBJ) $(VIEW0_C1_SOURCE) $(C_RUNTIME_SOURCES) $(STATIC_LIB) $(VIEW0_V1N0_LEXBOR_SANITIZE_LIB) $(ARBORCORE_C_LDFLAGS) $(VIEW0_V1N1_G03_C0_WRAP_LDFLAG) -fsanitize=address,undefined -lm -o $@
view0-v1n3-c1-sanitize: $(VIEW0_V1N3_SANITIZE_TEST)
	@ASAN_OPTIONS=detect_leaks=$${ARBORCORE_ASAN_DETECT_LEAKS:-1}:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $<
	@echo "VIEW0_V1N3_C1_SANITIZE=PASS"
view0-v1n3-c1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_v1n3_c0_gate.sh

# ============================================================
# VIEW0 D1 — post-V1N4 manuals, runnable examples, documentation consistency
# ============================================================
VIEW0_D1_EXAMPLE_DIR := examples/view0
VIEW0_D1_BUILD_DIR := $(BUILD_DIR)/view0-d1
VIEW0_D1_DOCUMENT_DIR := $(VIEW0_D1_BUILD_DIR)/documents
VIEW0_D1_RENDER_SOURCE := $(VIEW0_D1_EXAMPLE_DIR)/render.c
VIEW0_D1_ASM_SOURCE := $(VIEW0_D1_EXAMPLE_DIR)/nasm_view.asm
VIEW0_D1_TEMPLATE_SOURCE := $(VIEW0_D1_EXAMPLE_DIR)/page.html
VIEW0_D1_RENDER_OBJ := $(VIEW0_D1_BUILD_DIR)/render.o
VIEW0_D1_ASM_OBJ := $(VIEW0_D1_BUILD_DIR)/nasm_view.o
VIEW0_D1_EXAMPLE := $(VIEW0_D1_BUILD_DIR)/render-example
VIEW0_D1_TEMPLATE_DOCUMENT := $(VIEW0_D1_DOCUMENT_DIR)/template.html
VIEW0_D1_NATIVE_C_DOCUMENT := $(VIEW0_D1_DOCUMENT_DIR)/native-c.html
VIEW0_D1_NASM_DOCUMENT := $(VIEW0_D1_DOCUMENT_DIR)/nasm.html

.PHONY: view0-d1-example view0-d1-example-documents view0-d1-gate

$(VIEW0_D1_BUILD_DIR):
	mkdir -p $@

$(VIEW0_D1_DOCUMENT_DIR):
	mkdir -p $@

$(VIEW0_D1_RENDER_OBJ): $(VIEW0_D1_RENDER_SOURCE) $(VIEW0_C1_HEADER) | $(VIEW0_D1_BUILD_DIR)
	$(CC) $(ARBORCORE_C_CPPFLAGS) $(ARBORCORE_C_CFLAGS) -c $< -o $@

$(VIEW0_D1_ASM_OBJ): $(VIEW0_D1_ASM_SOURCE) | $(VIEW0_D1_BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(VIEW0_D1_EXAMPLE): $(VIEW0_D1_RENDER_OBJ) $(VIEW0_D1_ASM_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)
	$(CC) $(ARBORCORE_C_LDFLAGS) -o $@ $(VIEW0_D1_RENDER_OBJ) $(VIEW0_D1_ASM_OBJ) $(VIEW0_C1_LIB) $(C_RUNTIME_LIB) $(STATIC_LIB)

view0-d1-example: $(VIEW0_D1_EXAMPLE)

view0-d1-example-documents: $(VIEW0_D1_EXAMPLE) | $(VIEW0_D1_DOCUMENT_DIR)
	@$(VIEW0_D1_EXAMPLE) \
		$(VIEW0_D1_TEMPLATE_SOURCE) \
		$(VIEW0_D1_TEMPLATE_DOCUMENT) \
		$(VIEW0_D1_NATIVE_C_DOCUMENT) \
		$(VIEW0_D1_NASM_DOCUMENT)

view0-d1-gate:
	@ARBORCORE_ROOT=$(ROOT_DIR) bash tools/view0_d1_gate.sh
