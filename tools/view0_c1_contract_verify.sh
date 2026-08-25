#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
contract=view/arborcore-view-core-1.contract
header=include/arborcore/view.h
source=src/c/view.c
doc=docs/VIEW_CORE_VIEW0.md
for item in \
 'VIEW0_C1_CONTRACT_REVISION=0.1-VIEW0-C1-R1' \
 'VIEW0_C1_SCOPE=BOUNDED_VIEW_OUTPUT_PRIMITIVE' \
 'VIEW0_C1_PUBLIC_FUNCTION_COUNT=5' \
 'VIEW0_C1_BODY_STORAGE=REQUEST_ARENA' \
 'VIEW0_C1_HIDDEN_HEAP=ZERO' \
 'VIEW0_C1_CHECKED_SIZE_ARITHMETIC=ASSEMBLY_U64_ADD_CHECKED' \
 'VIEW0_C1_ARENA_TRANSACTION=ASSEMBLY_ARENA_MARK_REWIND' \
 'VIEW0_C1_BOUNDED_APPEND=ASSEMBLY_BUFFER_APPEND' \
 'VIEW0_C1_EXACT_LENGTH_COMMIT=REQUIRED' \
 'VIEW0_C1_APPEND_FAILURE_AFTER_VALID_ACTIVE_STATE=AUTOMATIC_REWIND' \
 'VIEW0_C1_COMMIT_FAILURE_AFTER_VALID_ACTIVE_STATE=AUTOMATIC_REWIND' \
 'VIEW0_C1_VALIDATION_FAILURE_BEFORE_SAFE_REWIND=FAIL_CLOSED_NO_AUTOMATIC_REWIND' \
 'VIEW0_C1_OUTPUT_METADATA_FIELDS=FRAMEWORK_PRIVATE_BY_CONTRACT' \
 'VIEW0_C1_CALLER_MUTATION_WHILE_ACTIVE=PROHIBITED' \
 'VIEW0_C1_POINTER_SPAN_PRECONDITION=VALID_LIVE_READABLE_WRITABLE_AS_APPLICABLE' \
 'VIEW0_C1_INTERNAL_THREAD_SYNCHRONIZATION=NONE' \
 'VIEW0_C1_PARTIAL_BODY_PUBLICATION_ON_FAILURE=NO' \
 'VIEW0_C1_TEMPLATE_PARSER=NOT_IN_C1' \
 'VIEW0_C1_HTML_ESCAPING=NOT_IN_C1' \
 'VIEW0_C1_DATABASE_DEPENDENCY=NONE'; do
  grep -Fxq "$item" "$contract" || { echo "FAIL: missing C1 contract marker: $item" >&2; exit 1; }
done
for fn in \
  arbor_view_measure_add \
  arbor_view_output_begin \
  arbor_view_output_append \
  arbor_view_output_commit \
  arbor_view_output_abort; do
  grep -Fq "arbor_status $fn(" "$header" || { echo "FAIL: missing VIEW0 C1 function: $fn" >&2; exit 1; }
done
! grep -Eq '\b(malloc|calloc|realloc|free|pthread_|mtx_|atomic_)\b' "$source"
grep -Fq 'u64_add_checked' "$source"
grep -Fq 'arena_alloc' "$source"
grep -Fq 'arena_rewind' "$source"
grep -Fq 'buffer_append' "$source"
grep -Fq 'VIEW is the presentation boundary; it is not synonymous with a template engine.' "$doc"
echo 'VIEW0_C1_REQUIRED_PUBLIC_FUNCTION_COUNT=5'
echo 'VIEW0_C1_HIDDEN_HEAP_COUNT=0'
echo 'VIEW0_C1_INTERNAL_LOCK_ATOMIC_COUNT=0'
echo 'PASS: VIEW0 C1 contract and required public surface preserved under cumulative VIEW0 extension'
