#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"

CONTRACT='application/arborcore-application-ddd-support-1.contract'
HEADER='include/arborcore/ddd_support.h'
SOURCE='src/c/ddd_support.c'

fail(){ printf 'FAIL: %s\n' "$*" >&2; exit 1; }
must(){ grep -Fqx "$1" "$CONTRACT" || fail "contract marker missing: $1"; }

must 'AF4_PHASE=FINAL_PLANNED_APPLICATION_FOUNDATION_PHASE'
must 'AF4_PORT_MODEL=SEMANTIC_ROLE_OVER_AF2_TYPED_CAPABILITY'
must 'AF4_SECOND_CAPABILITY_REGISTRY=PROHIBITED'
must 'AF4_REPOSITORY_MODEL=BOUNDED_CONTEXT_SPECIFIC_OUTBOUND_TYPED_PORT'
must 'AF4_GENERIC_CRUD_REPOSITORY=PROHIBITED'
must 'AF4_TRANSACTION_MODEL=SINGLE_AUTHORITY_UNIT_OF_WORK'
must 'AF4_DISTRIBUTED_TRANSACTION=OUT_OF_SCOPE'
must 'AF4_TRANSACTION_INTERFACE_SIZE_X86_64=72'
must 'AF4_TRANSACTION_VIEW_SIZE_X86_64=32'
must 'AF4_OPAQUE_PROVIDER_CONTEXT_ANCHOR_OVERLAP=REJECT_WHEN_MACHINE_DETECTABLE'
must 'AF4_TRANSACTION_VIEW_KNOWN_REGION_OVERLAP=REJECT'
must 'AF4_UOW_SIZE_X86_64=64'
must 'AF4_UOW_BEGIN_REQUIRES_CANONICAL_EVENT_JOURNAL=YES'
must 'AF4_UOW_RESET_EXTERNAL_DEPENDENCY_DEREFERENCE=NO'
must 'AF4_DOMAIN_EVENT_ENGINE=CALLER_OWNED_DETERMINISTIC_JOURNAL'
must 'AF4_LINUX_EPOLL_EVENT_ENGINE_IS_DOMAIN_EVENT_MODEL=NO'
must 'AF4_EVENT_RECORD_SIZE_X86_64=32'
must 'AF4_EVENT_CHECKPOINT_SIZE_X86_64=16'
must 'AF4_EVENT_JOURNAL_SIZE_X86_64=40'
must 'AF4_EVENT_VIEW_SIZE_X86_64=40'
must 'AF4_EVENT_JOURNAL_VALIDATION=CANONICAL_PACKED_PAYLOAD_PREFIX'
must 'AF4_EVENT_REWIND_CHECKPOINT=EXACT_EVENT_BOUNDARY_REQUIRED'
must 'AF4_EVENT_CHECKPOINT_AND_REWIND_REQUIRE_CANONICAL_JOURNAL=YES'
must 'AF4_TRANSACTION_EVENT_COUPLING=CHECKPOINT_ON_BEGIN_REWIND_ON_ROLLBACK_OR_COMMIT_FAILURE'
must 'AF4_PUBLIC_FUNCTION_COUNT=14'
must 'AF4_HIDDEN_HEAP=NO'
must 'AF4_MUTABLE_GLOBAL_REGISTRY=NO'
must 'AF4_INTERNAL_LOCKING=NO'
must 'AF4_AF0_AF3_PRODUCTION_BYTES=FROZEN'

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

cc -Iinclude -D_POSIX_C_SOURCE=200809L \
  -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror \
  -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes \
  -Wmissing-prototypes -Wformat=2 -Wundef \
  -c "$SOURCE" -o "$tmp/ddd_support.o"

PUBLIC_COUNT=$(nm -g --defined-only "$tmp/ddd_support.o" |
  awk '$2=="T" && $3 ~ /^arbor_ddd_/ {c++} END{print c+0}')
[[ "$PUBLIC_COUNT" == 14 ]] || fail "public AF4 symbol count is $PUBLIC_COUNT, expected 14"

MUTABLE_GLOBAL_COUNT=$(nm -g --defined-only "$tmp/ddd_support.o" |
  awk '$2 ~ /^[BbCcDdGgSsVvWw]$/ {c++} END{print c+0}')
[[ "$MUTABLE_GLOBAL_COUNT" == 0 ]] ||
  fail "production mutable-global count is $MUTABLE_GLOBAL_COUNT"

if grep -nE '\b(malloc|calloc|realloc|free|aligned_alloc|posix_memalign)\b' "$SOURCE"; then
  fail 'hidden heap primitive found in AF4 production source'
fi
if grep -nE '\b(pthread_|mtx_|atomic_|futex|mutex|spinlock|refcount)\b' "$SOURCE"; then
  fail 'hidden lock/refcount primitive found in AF4 production source'
fi
if grep -niE 'arbor_.*repository_(get|create|update|delete|save|remove)' "$HEADER" "$SOURCE"; then
  fail 'generic CRUD repository API found'
fi
if grep -niE 'arbor_.*execute[[:space:]]*\([^)]*void[[:space:]]*\*[^)]*void[[:space:]]*\*' "$HEADER" "$SOURCE"; then
  fail 'universal untyped execute ABI found'
fi

DECL_COUNT=$(grep -Ec '^arbor_status arbor_ddd_' "$HEADER")
[[ "$DECL_COUNT" == 14 ]] || fail "header AF4 declaration count is $DECL_COUNT, expected 14"

printf 'AF4_PUBLIC_SYMBOL_COUNT=%s\n' "$PUBLIC_COUNT"
printf 'AF4_MUTABLE_PRODUCTION_GLOBAL_COUNT=%s\n' "$MUTABLE_GLOBAL_COUNT"
printf 'AF4_HIDDEN_HEAP_COUNT=0\n'
printf 'AF4_INTERNAL_LOCK_REFCOUNT_COUNT=0\n'
printf 'AF4_GENERIC_CRUD_API_COUNT=0\n'
printf 'AF4_UNTYPED_EXECUTE_COUNT=0\n'
printf 'PASS: AF4 DDD-support contract and public production surface\n'
