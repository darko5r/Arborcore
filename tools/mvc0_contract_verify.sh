#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"
CONTRACT='mvc/arborcore-mvc-core-transport-1.contract'
MVC_HEADER='include/arborcore/mvc.h'
TRANSPORT_HEADER='include/arborcore/application_transport.h'
MVC_SOURCE='src/c/mvc.c'
TRANSPORT_SOURCE='src/c/application_transport.c'
ASM_SOURCE='src/asm/application_transport.asm'
fail(){ printf 'FAIL: %s
' "$*" >&2; exit 1; }
must(){ grep -Fqx "$1" "$CONTRACT" || fail "contract marker missing: $1"; }
for m in 'MVC0_PHASE=CORE_MVC_PLUS_PARALLEL_RICH_APPLICATION_TRANSPORT' 'MVC0_LEGACY_SERVER_PATH=PRESERVED_BYTE_EXACT' 'MVC0_FROZEN_ASSEMBLY_ABI_V1_REOPEN=NO' 'MVC0_AF0_AF4_REOPEN=NO' 'MVC0_RICH_TRANSPORT_MODEL=PARALLEL_HIGHER_LAYER_ASSEMBLY_TRANSPORT' 'MVC0_RICH_TRANSPORT_PUBLIC_ABI_V1_EXTENSION=NO' 'MVC0_TRANSPORT_EAGAIN_RESUME=REQUIRED' 'MVC0_TRANSPORT_KEEP_ALIVE_FALSE=WRITE_COMPLETE_RESPONSE_THEN_CLOSE' 'MVC0_TRANSPORT_STORAGE_BACKINGS=CALLER_OWNED_AND_MUTUALLY_DISJOINT' 'MVC0_TRANSPORT_APPLICATION_CONTEXT_ANCHOR_OVERLAP=REJECT_WHEN_MACHINE_DETECTABLE' 'MVC0_RESPONSE_BODY_OUTPUT_METADATA_OR_BACKING_OVERLAP=REJECT' 'MVC0_ROUTE_MODEL=IMMUTABLE_CATALOG_FIRST_MATCH_ORDER' 'MVC0_METHOD_MATCH=EXACT_CASE_SENSITIVE' 'MVC0_PATTERN_MATCH=REUSE_QUALIFIED_ARBOR_ROUTE_MATCH' 'MVC0_DUPLICATE_EXACT_METHOD_PATTERN=REJECT' 'MVC0_PREPARE_WORKSPACE_SELF_ALIAS=REJECT' 'MVC0_MIDDLEWARE_AFTER_ORDER=REVERSE_ENTERED_LIST' 'MVC0_CONTROLLER_AF3_BINDING_POLICY=COMPOSITION_RESOLVE_THEN_CACHE_IN_CALLER_CONTEXT' 'MVC0_PER_REQUEST_AF2_LOOKUP=PROHIBITED' 'MVC0_AF4_UOW_PORT_EVENT_POLICY=REUSE_FROM_TYPED_SERVICE_OR_CONTROLLER_CODE_NO_WRAPPER' 'MVC0_CONTROLLER_RESULT_SIZE_X86_64=24' 'MVC0_MIDDLEWARE_BEFORE_RESULT_SIZE_X86_64=40' 'MVC0_ROUTE_SIZE_X86_64=80' 'MVC0_CATALOG_SIZE_X86_64=48' 'MVC0_APPLICATION_SIZE_X86_64=40' 'MVC0_REQUEST_SIZE_X86_64=40' 'MVC0_CALLBACK_OUTPUT_PUBLICATION=VALIDATE_THEN_PUBLISH' 'MVC0_APPLICATION_PREPARED_MAX_PARAM_GUARD=BITWISE_COMPLEMENT' 'MVC0_REQUEST_HOT_PATH_APPLICATION_CHECK=SHALLOW_PREPARED_INTEGRITY' 'MVC0_MACHINE_DETECTABLE_FRAMEWORK_STACK_BODY_ESCAPE=REJECT' 'MVC0_PUBLIC_FUNCTION_COUNT=8' 'MVC0_HIDDEN_HEAP=NO' 'MVC0_MUTABLE_GLOBAL_REGISTRY=NO' 'MVC0_INTERNAL_LOCKING=NO' 'MVC0_BROWSER_RENDERER_MODIFICATION=PROHIBITED'; do must "$m"; done

tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
CFLAGS=(-Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
cc "${CFLAGS[@]}" -c "$MVC_SOURCE" -o "$tmp/mvc.o"
cc "${CFLAGS[@]}" -c "$TRANSPORT_SOURCE" -o "$tmp/transport.o"
PUBLIC_COUNT=$(nm -g --defined-only "$tmp/mvc.o" "$tmp/transport.o" | awk '$2=="T" && $3 ~ /^arbor_/ {c++} END{print c+0}')
[[ "$PUBLIC_COUNT" == 8 ]] || fail "public MVC0 symbol count is $PUBLIC_COUNT, expected 8"
MUTABLE=$(nm -g --defined-only "$tmp/mvc.o" "$tmp/transport.o" | awk '$2 ~ /^[BbCcDdGgSsVvWw]$/ {c++} END{print c+0}')
[[ "$MUTABLE" == 0 ]] || fail "mutable production-global count $MUTABLE"
DECL_COUNT=$({ grep -hE '^arbor_status arbor_' "$MVC_HEADER" "$TRANSPORT_HEADER" || true; } | wc -l)
[[ "$DECL_COUNT" == 8 ]] || fail "public header declaration count $DECL_COUNT != 8"
if grep -nE '(^|[^[:alnum:]_])(malloc|calloc|realloc|free|aligned_alloc|posix_memalign)[[:space:]]*\(' "$MVC_SOURCE" "$TRANSPORT_SOURCE"; then fail 'hidden heap primitive found'; fi
if grep -nE '(^|[^[:alnum:]_])(pthread_|mtx_|atomic_|futex|mutex|spinlock|refcount)' "$MVC_SOURCE" "$TRANSPORT_SOURCE"; then fail 'hidden lock/refcount primitive found'; fi
if grep -niE 'arbor_.*execute[[:space:]]*\([^)]*void[[:space:]]*\*[^)]*void[[:space:]]*\*' "$MVC_HEADER" "$TRANSPORT_HEADER" "$MVC_SOURCE" "$TRANSPORT_SOURCE"; then fail 'generic untyped business execute found'; fi
[[ "$(grep -Ec '^global ' "$ASM_SOURCE")" == 1 ]] || fail 'rich transport Assembly exports more than its internal entry'
grep -Fqx 'global application_transport_handle_once:function' "$ASM_SOURCE" || fail 'rich transport internal symbol missing'
if grep -Rqs 'application_transport_handle_once' abi/arborcore-1.*; then fail 'MVC0 internal transport leaked into Assembly ABI v1 files'; fi
for sym in server_handle_http_once arbor_server_step; do grep -Rqs "$sym" src/asm/server.asm include/arborcore/arborcore.h || fail "legacy symbol missing: $sym"; done
printf 'MVC0_PUBLIC_SYMBOL_COUNT=%s
' "$PUBLIC_COUNT"
printf 'MVC0_MUTABLE_PRODUCTION_GLOBAL_COUNT=0
'
printf 'MVC0_HIDDEN_HEAP_COUNT=0
'
printf 'MVC0_INTERNAL_LOCK_REFCOUNT_COUNT=0
'
printf 'MVC0_GENERIC_UNTYPED_EXECUTE_COUNT=0
'
printf 'MVC0_ASSEMBLY_ABI_V1_EXTENSION_COUNT=0
'
printf 'PASS: MVC0 core/transport contract and public production surface
'
