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

# Assembly ABI/library readiness artifacts
ARBORCORE_LIBRARY_OBJECTS := $(filter-out $(START_OBJ),$(ARBORCORE_OBJECTS))
STATIC_LIB := $(BUILD_DIR)/libarborcore.a
SHARED_LIB := $(BUILD_DIR)/libarborcore.so.1
SHARED_LIB_LINK := $(BUILD_DIR)/libarborcore.so
ABI_STATIC_CONSUMER := $(BUILD_DIR)/abi-static-consumer
ABI_SHARED_CONSUMER := $(BUILD_DIR)/abi-shared-consumer
ABI_PUBLIC_SYMBOLS := abi/arborcore-1.symbols
ABI_INTERNAL_SYMBOLS := abi/arborcore-1.internal-symbols
ABI_VERSION_SCRIPT := abi/arborcore-1.map
ABI_LAYOUT := abi/arborcore-1.layout

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
.PHONY: libarborcore-static libarborcore-shared
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

$(SHARED_LIB): $(ARBORCORE_LIBRARY_OBJECTS) $(ABI_VERSION_SCRIPT)
	$(LD) -shared -Bsymbolic-functions -z defs -z text -z relro -z now -z noexecstack \
		-soname=libarborcore.so.1 --version-script=$(ABI_VERSION_SCRIPT) \
		-o $@ $(ARBORCORE_LIBRARY_OBJECTS)

$(SHARED_LIB_LINK): $(SHARED_LIB)
	ln -sfn libarborcore.so.1 $@

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

libarborcore-static: $(STATIC_LIB) $(ABI_STATIC_CONSUMER)

libarborcore-shared: $(SHARED_LIB) $(SHARED_LIB_LINK) $(ABI_SHARED_CONSUMER)

abi-surface-verify: $(ARBORCORE_OBJECTS) $(SECURITY_OBJ) $(ABI_SURFACE_VERIFY) $(ABI_PUBLIC_SYMBOLS) $(ABI_INTERNAL_SYMBOLS)
	@bash $(ABI_SURFACE_VERIFY)

abi-dependency-verify: $(ARBORCORE_OBJECTS) $(ABI_DEPENDENCY_VERIFY) $(ABI_PUBLIC_SYMBOLS) $(ABI_INTERNAL_SYMBOLS)
	@bash $(ABI_DEPENDENCY_VERIFY)

abi-layout-verify: $(ABI_LAYOUT_VERIFY) $(ABI_LAYOUT)
	@bash $(ABI_LAYOUT_VERIFY)

security-shape-verify: $(SECURITY_OBJ) $(SECURITY_SHAPE_VERIFY)
	@bash $(SECURITY_SHAPE_VERIFY)

library-readiness: $(STATIC_LIB) $(SHARED_LIB) $(SHARED_LIB_LINK) $(ABI_STATIC_CONSUMER) $(ABI_SHARED_CONSUMER) $(LIBRARY_READINESS_VERIFY)
	@bash $(LIBRARY_READINESS_VERIFY)
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

# Cleanup
clean:
	rm -rf $(BUILD_DIR)

# Full reset including machine-qualified policy
distclean: clean
	rm -f $(MEMORY_POLICY) $(SERVER_PERF_BASELINE) $(CODEC_PERF_BASELINE)
