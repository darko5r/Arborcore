#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
CONTRACT=application/arborcore-application-service-runtime-1.contract
SOURCE=src/c/application_service.c
HEADER=include/arborcore/application_service.h
ASM_TEST=tests/asm/application_service_runtime_abi_test.asm

[[ -s "$CONTRACT" && -s "$SOURCE" && -s "$HEADER" && -s "$ASM_TEST" ]]

required=(
  'ARBORCORE_APPLICATION_SERVICE_RUNTIME_VERSION=0.1-AF3-R1-CANDIDATE'
  'AF3_BASE_COMMIT=4337e18c4d76af4c2f11259cc3569244b3312a54'
  'AF3_BASE_TREE=35398e2da67ef47efe2102165ca566fd752b594c'
  'AF3_ARCHITECTURE=APPLICATION_SERVICE_RUNTIME_LIFECYCLE_AND_TYPED_USE_CASE_MODEL'
  'AF3_REBUILDS_AF2_DEPENDENCY_GRAPH=NO'
  'AF3_LIFECYCLE_UNIT=ONE_MANAGED_SERVICE_MODULE_DESCRIPTOR_PER_AF2_MODULE'
  'AF3_CONTROL_PLANE=GENERIC_FRAMEWORK_RUNTIME'
  'AF3_DATA_PLANE=BOUNDED_CONTEXT_TYPED'
  'AF3_UNIVERSAL_UNTYPED_BUSINESS_EXECUTE=PROHIBITED'
  'AF3_SERVICE_MODULE_DESCRIPTOR_SIZE_X86_64=80'
  'AF3_PROVIDER_CONTEXT=AF2_PROVIDER_OWNED_BORROWED_SEPARATE_FROM_MODULE_CONTEXT'
  'AF3_MULTI_SERVICE_EXPORT_PER_MODULE=SUPPORTED'
  'AF3_LIFECYCLE_CALLBACK_SHAPE=ALL_NULL_PASSIVE_OR_PREPARE_ROLLBACK_STOP_ALL_NON_NULL'
  'AF3_PREPARE_CURRENT_FAILURE=SELF_CLEAN_FAILURE_ATOMIC'
  'AF3_ROLLBACK_ABI=RDI_MODULE_CONTEXT_VOID_NO_FAIL'
  'AF3_STOP_CONTINUE_AFTER_FAILURE=YES'
  'AF3_STOP_VALIDATION_PRECEDENCE=VALIDATE_COMPLETE_RUNTIME_BEFORE_EBUSY_OR_EALREADY_STATE_RESULT'
  'AF3_RUNTIME_STATES=READY_1_STOPPING_2_STOPPED_3_STOP_FAILED_4'
  'AF3_ZERO_MANAGED_MODULES=VALID_READY_RUNTIME'
  'AF3_MODULE_MAP_SENTINEL=UINT64_MAX'
  'AF3_PREPARE_DEPENDENCY_AUTHORITY=REAL_AF2_CATALOG_RESOLVE'
  'AF3_PREPARE_PERSISTENT_STORAGE=UNCHANGED_ON_FAILURE'
  'AF3_PREPARE_RUNTIME_OUTPUT=UNCHANGED_ON_FAILURE'
  'AF3_READY_DISCOVERY_OUTPUT=UNCHANGED_ON_FAILURE'
  'AF3_OUTPUT_ALIAS_POLICY=MEASURE_BINDING_AND_READY_DISCOVERY_OUTPUTS_MUST_NOT_OVERWRITE_KNOWN_IMMUTABLE_OR_AF3_INTERNAL_RUNTIME_REGIONS'
  'AF3_PREFIX_EXTENT_ALIAS_POLICY=PREFIX_EXTENSIBLE_RUNTIME_AND_AF2_CATALOG_REGIONS_USE_CLAIMED_STRUCT_SIZE_FOR_DISJOINTNESS'
  'AF3_PREPARE_RESOLVE_OUTPUT_ALIAS_POLICY=BINDING_OUTPUT_DISJOINT_FROM_PREPARE_CONTEXT_WORKSPACE_RECORDS_MODULE_MAP_AND_KNOWN_IMMUTABLE_INPUTS'
  'AF3_READY_DISCOVERY_OUTPUT_ALIAS_POLICY=BINDING_OUTPUT_DISJOINT_FROM_RUNTIME_RECORDS_AND_KNOWN_IMMUTABLE_INPUTS'
  'AF3_MEASURE_OUTPUT_ALIAS_POLICY=REQUIREMENTS_OUTPUT_DISJOINT_FROM_KNOWN_IMMUTABLE_INPUTS'
  'AF3_GLOBAL_MUTABLE_SERVICE_REGISTRY=ZERO'
  'AF3_HIDDEN_HEAP_ALLOCATION=ZERO'
  'AF3_INTERNAL_LOCKING=ZERO'
  'AF3_INTERNAL_REFERENCE_COUNTING=ZERO'
  'AF3_C_TO_ASSEMBLY_TYPED_METHOD_QUALIFICATION=REQUIRED'
  'AF3_ASSEMBLY_TO_C_TYPED_METHOD_QUALIFICATION=REQUIRED'
  'AF3_REPRO_ARCHIVE_FORMAT=USTAR'
  'AF3_REPRO_ARCHIVE_MODE_NORMALIZATION=U_RW_X_GO_R_X'
  'AF3_REPRO_ARCHIVE_EXPECTED_NONEXEC_MODE=0644'
  'AF3_REPRO_ARCHIVE_EXPECTED_EXEC_MODE=0755'
  'AF3_SCOPE_PATH_COUNT=15'
  'AF3_LOWER_LAYER_RETROFIT_REQUIRED=NO'
  'AF3_MARIADB=FROZEN'
  'AF3_R=FROZEN'
  'AF3_DEPLOYMENT=FROZEN'
)
for line in "${required[@]}"; do grep -Fqx "$line" "$CONTRACT"; done

grep -Fq 'arbor_capability_catalog_resolve' "$SOURCE"
grep -Fq 'arbor_capability_catalog_find' "$SOURCE"
grep -Fq 'u64_mul_checked' "$SOURCE"
grep -Fq 'u64_add_checked' "$SOURCE"
grep -Fq 'memory_copy' "$SOURCE"
grep -Fq 'memory_set' "$SOURCE"
grep -Fq 'memory_zero' "$SOURCE"
grep -Fq 'range_overlaps' "$SOURCE"
grep -Fq '_Static_assert(sizeof(arbor_application_service_module_descriptor) == 80u' "$SOURCE"
grep -Fq 'ARBOR_APPLICATION_RUNTIME_STOP_FAILED' "$SOURCE"
grep -Fq 'immutable_inputs_require_disjoint' "$SOURCE"
grep -Fq '((af3_region){catalog, (uint64_t)catalog->struct_size})' "$SOURCE"
grep -Fq '(af3_region){runtime, (uint64_t)runtime->struct_size}' "$SOURCE"
grep -Fq 'prepare_binding_output_region_validate' "$SOURCE"
grep -Fq 'runtime_output_region_validate' "$SOURCE"

if grep -E '\b(malloc|calloc|realloc|free)[[:space:]]*\(' "$SOURCE" >/dev/null; then
  echo 'FAIL: AF3 runtime must not use hidden heap allocation' >&2
  exit 1
fi
if grep -E '\b(pthread_|mtx_|atomic_)' "$SOURCE" >/dev/null; then
  echo 'FAIL: AF3 v1 must not hide internal locking/reference-counting policy' >&2
  exit 1
fi
if grep -E 'execute[[:space:]]*\([[:space:]]*void[[:space:]]*\*[[:space:]]*,[[:space:]]*void[[:space:]]*\*[[:space:]]*,[[:space:]]*void[[:space:]]*\*' "$HEADER" "$SOURCE" >/dev/null; then
  echo 'FAIL: AF3 must not add universal untyped business execute' >&2
  exit 1
fi

grep -Fq 'global af3_asm_prepare:function' "$ASM_TEST"
grep -Fq 'global af3_asm_stop:function' "$ASM_TEST"
grep -Fq 'global af3_asm_typed_method:function' "$ASM_TEST"
grep -Fq 'global af3_asm_call_c_typed:function' "$ASM_TEST"
grep -Fq 'global af3_asm_call_c_typed_preserve:function' "$ASM_TEST"
grep -Fq 'section .note.GNU-stack noalloc noexec nowrite progbits' "$ASM_TEST"
grep -Fq -- "--mode='u=rwX,go=rX'" tools/application_service_runtime_reproducibility_verify.sh
grep -Fq 'expected_mode=644' tools/application_service_runtime_reproducibility_verify.sh
grep -Fq 'expected_mode=755' tools/application_service_runtime_reproducibility_verify.sh

git diff --check

echo 'PASS: AF3 exact Application-service runtime/lifecycle/typed-data-plane contract'
