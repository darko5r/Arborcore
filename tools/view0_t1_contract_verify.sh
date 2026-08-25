#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
contract=view/arborcore-view-core-1.contract
header=include/arborcore/view.h
source=src/c/view.c
doc=docs/VIEW_CORE_VIEW0.md
for item in \
 'VIEW0_T1_CONTRACT_REVISION=0.1-VIEW0-T1' \
 'VIEW0_T1_SCOPE=MINIMAL_PREPARED_HTML_TEMPLATE_MECHANISM' \
 'VIEW0_T1_PUBLIC_FUNCTION_COUNT=3' \
 'VIEW0_T1_TOTAL_PUBLIC_FUNCTION_COUNT=10' \
 'VIEW0_T1_CORE_ABSTRACTION=TEMPLATE_IS_ONE_VIEW_MECHANISM' \
 'VIEW0_T1_NAME_GRAMMAR=ASCII_IDENTIFIER_CASE_SENSITIVE' \
 'VIEW0_T1_FIELD_SCHEMA=CALLER_DECLARED_ORDERED_UNIQUE_NAME_ARRAY' \
 'VIEW0_T1_DYNAMIC_CONTEXT=HTML_NORMAL_DATA_TEXT_ONLY' \
 'VIEW0_T1_TAG_ATTRIBUTE_PLACEHOLDER=REJECT' \
 'VIEW0_T1_COMMENT_PLACEHOLDER=REJECT' \
 'VIEW0_T1_RAW_RCDATA_PLACEHOLDER=REJECT' \
 'VIEW0_T1_FOREIGN_SVG_MATH_SOURCE=REJECT_IN_T1' \
 'VIEW0_T1_AMBIGUOUS_CHARACTER_REFERENCE_PREFIX=REJECT' \
 'VIEW0_T1_SOURCE_LIFETIME=PREPARATION_CALL_ONLY' \
 'VIEW0_T1_FIELD_NAME_LIFETIME=PREPARATION_CALL_ONLY' \
 'VIEW0_T1_PERSISTENT_PART_STORAGE=CALLER_OWNED_APPLICATION_LIFETIME' \
 'VIEW0_T1_PERSISTENT_LITERAL_STORAGE=CALLER_OWNED_APPLICATION_LIFETIME_COPY' \
 'VIEW0_T1_PREPARED_IMMUTABILITY=REQUIRED' \
 'VIEW0_T1_SOURCE_RETAINED_AFTER_PREPARE=NO' \
 'VIEW0_T1_FIELD_NAMES_RETAINED_AFTER_PREPARE=NO' \
 'VIEW0_T1_PREPARE_MEASUREMENT=EXACT_PART_COUNT_AND_LITERAL_BYTES' \
 'VIEW0_T1_PREPARE_FAILURE_PUBLICATION=TRANSACTIONAL_EXPECTED_FAILURES_LEAVE_OUTPUT_UNCHANGED' \
 'VIEW0_T1_PREPARE_FIELD_RESOLUTION=DETERMINISTIC_LINEAR_ARRAY_SCAN_STARTUP_ONLY' \
 'VIEW0_T1_RUNTIME_FIELD_NAME_LOOKUP=ZERO' \
 'VIEW0_T1_RUNTIME_TEMPLATE_PARSE=ZERO' \
 'VIEW0_T1_RUNTIME_HASH_TABLE=NONE' \
 'VIEW0_T1_VALUE_STABILITY_DURING_RENDER=CALLER_REQUIRED' \
 'VIEW0_T1_RENDER_MEASUREMENT=C1_C2_EXACT' \
 'VIEW0_T1_BODY_STORAGE=REQUEST_ARENA' \
 'VIEW0_T1_BODY_PUBLICATION=C1_COMMIT_ONLY' \
 'VIEW0_T1_PRIOR_SAME_ARENA_VALUE=ALLOW_NONOVERLAPPING_FUTURE_OUTPUT' \
 'VIEW0_T1_FITTING_FUTURE_BODY_INPUT_ALIAS=REJECT' \
 'VIEW0_T1_CAPACITY_PRECEDENCE=C1_ENOSPC_BEFORE_SPECULATIVE_FUTURE_ALIAS' \
 'VIEW0_T1_HIDDEN_HEAP=ZERO' \
 'VIEW0_T1_MUTABLE_PRODUCTION_GLOBALS=ZERO' \
 'VIEW0_T1_FILE_IO=NONE' \
 'VIEW0_T1_UTF8_VALIDATION=NOT_YET' \
 'VIEW0_T1_USER_FACING_HTTP_ADMISSION=NOT_YET' \
 'VIEW0_T1_DYNAMIC_ATTRIBUTE_URL_CSS_JS_XML=NOT_ADMITTED' \
 'VIEW0_T1_NATIVE_C_VIEW_PATH=PRESERVED' \
 'VIEW0_T1_REAL_NASM_VIEW_PATH=PRESERVED' \
 'VIEW0_T1_DATABASE_DEPENDENCY=NONE'; do
  grep -Fxq "$item" "$contract" || { echo "FAIL: missing T1 contract marker: $item" >&2; exit 1; }
done

for fn in \
  arbor_view_html_template_measure \
  arbor_view_html_template_prepare \
  arbor_view_html_template_render; do
  grep -Fq "arbor_status $fn(" "$header" || { echo "FAIL: missing VIEW0 T1 function: $fn" >&2; exit 1; }
done

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' "$header")
[[ "$count" -ge 10 ]] || { echo "FAIL: expected at least 10 cumulative VIEW public functions preserving T1, got $count" >&2; exit 1; }

grep -Fq 'typedef struct arbor_view_html_template_requirements' "$header"
grep -Fq 'typedef struct arbor_view_html_template_part' "$header"
grep -Fq 'typedef struct arbor_view_html_template_storage' "$header"
grep -Fq 'typedef struct arbor_view_html_template' "$header"
grep -Fq 'template_scan(' "$source"
grep -Fq 'template_field_resolve(' "$source"
grep -Fq 'template_prepared_validate(' "$source"
grep -Fq 'memory_copy(destination, source.data + start, length)' "$source"
grep -Fq 'u64_mul_checked' "$source"
! grep -Eq '\b(malloc|calloc|realloc|free|pthread_|mtx_|atomic_)\b' "$source"
grep -Fq 'T1: minimal prepared HTML templates' "$doc"
grep -Fq 'performs no source parsing and no field-name' "$doc"
grep -Fq 'does not add a hash table' "$doc"

echo 'VIEW0_T1_PUBLIC_FUNCTION_COUNT=3'
echo "VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=$count"
echo 'VIEW0_T1_HIDDEN_HEAP_COUNT=0'
echo 'VIEW0_T1_INTERNAL_LOCK_ATOMIC_COUNT=0'
echo 'PASS: VIEW0 T1 grammar, persistent preparation, runtime composition and resource contract'
