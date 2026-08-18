#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
CONTRACT=http/arborcore-http-mvc-adapter-1.contract
[[ -s "$CONTRACT" ]]
required=(
'ARBORCORE_HTTP_MVC_ADAPTER_VERSION=0.1-HTTP1-R5'
'HTTP1_AF1_CONTROLLED_SEMANTIC_REOPEN=YES'
'HTTP1_AF1_RESPONSE_PLAN_LAYOUT=UNCHANGED_32_BYTES'
'HTTP1_AF1_FINAL_STATUS_RANGE=200_599'
'HTTP1_AF1_LEGACY_SERIALIZER_SUBSET=200_201_204_400_404_500'
'HTTP1_PUBLIC_FUNCTION_COUNT=7'
'HTTP1_APPLICATION_SIZE_X86_64=80'
'HTTP1_REQUIREMENTS_SIZE_X86_64=16'
'HTTP1_PRIVATE_EXCHANGE_SIZE_X86_64=56'
'HTTP1_ARENA_PREFIX_MEASUREMENT=AUTHORITATIVE_NO_HARDCODED_CALLER_BYTES'
'HTTP1_RESPONSE_FIELD_SIDECAR=REQUEST_LOCAL_ARENA_PREFIX'
'HTTP1_SIDECAR_APPLICATION_ANCHOR=PREPARED_HTTP1_APPLICATION'
'HTTP1_REQUEST_IDENTITY_BINDING=AF1_SCOPE_REQUEST'
'HTTP1_REQUEST_IDENTITY_GUARD=PRESERVED'
'HTTP1_REQUEST_SCOPE_LIFETIME=SYNCHRONOUS_NESTED_MVC_INVOCATION_ONLY'
'HTTP1_ARENA_PREFIX_ALIGNMENT_SLACK_BYTES=7'
'HTTP1_UNALIGNED_TRANSPORT_ARENA_BACKING=SUPPORTED'
'HTTP1_APPLICATION_ARENA=SUBARENA_AFTER_SIDECAR_PREFIX'
'HTTP1_REPEATED_SET_COOKIE=PRESERVED'
'HTTP1_RESPONSE_FIELD_MARK_OUTPUT_HTTP1_APPLICATION_ALIAS=REJECT'
'HTTP1_RESPONSE_FIELD_MARK_OUTPUT_MVC_APPLICATION_ALIAS=REJECT'
'HTTP1_RESPONSE_FIELD_MARK_OUTPUT_MVC_CATALOG_ALIAS=REJECT'
'HTTP1_TRANSPORT_ENGINE=REUSE_MVC0_APPLICATION_TRANSPORT_HANDLE_ONCE'
'HTTP1_TRANSPORT_CATALOG_ALIAS_CHECK=PER_SERVER_STEP_LINEAR_IN_ROUTE_COUNT'
'HTTP1_ROUTE_SCALE_PERFORMANCE=NONBLOCKING_DIAGNOSTIC_DEBT_FOR_HELLO0_REVIEW'
'HTTP1_CURRENT_BENCHMARK_SCOPE=PREPARED_APPLICATION_VALIDATE_ONLY'
'HTTP1_SECOND_ROUTER=NO'
'HTTP1_SECOND_CONTROLLER_PIPELINE=NO'
'HTTP1_SECOND_MIDDLEWARE_PIPELINE=NO'
'HTTP1_FINAL_SERIALIZER=HTTP0_ARBOR_HTTP_RESPONSE_SERIALIZE'
'HTTP1_DATE_POLICY=DEFERRED_DATE1_BEFORE_PRODUCTION_ORIGIN_SERVER_CLAIM'
'HTTP1_ABSOLUTE_FORM=DEFERRED_TARGET1_BEFORE_PRODUCTION_HTTP11_CLAIM'
'HTTP1_DATABASE_BEFORE_HELLO0=PROHIBITED'
)
for line in "${required[@]}"; do grep -Fqx "$line" "$CONTRACT" || { echo "FAIL: missing HTTP1 contract line: $line" >&2; exit 1; }; done

tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O2 -fPIC \
  -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
  -c src/c/http_mvc.c -o "$tmp/http_mvc.o"
nm -g --defined-only "$tmp/http_mvc.o" | awk '$2=="T" && $3 ~ /^arbor_http_mvc_/ {print $3}' | LC_ALL=C sort -u >"$tmp/actual"
cat >"$tmp/expected" <<'E'
arbor_http_mvc_application_measure
arbor_http_mvc_application_prepare
arbor_http_mvc_application_validate
arbor_http_mvc_response_field_append
arbor_http_mvc_response_fields_mark
arbor_http_mvc_response_fields_rewind
arbor_http_mvc_server_step
E
diff -u "$tmp/expected" "$tmp/actual"

mutable=$(objdump -t "$tmp/http_mvc.o" | awk '
$3=="O" && ($4==".data" || $4 ~ /^\.data\.[^.]+$/ || $4==".bss" || $4 ~ /^\.bss\./ || $4==".tdata" || $4==".tbss") && $4 !~ /^\.data\.rel\.ro/ {c++}
END{print c+0}')
[[ "$mutable" -eq 0 ]]
heap=$({ grep -nE '(^|[^[:alnum:]_])(malloc|calloc|realloc|free|aligned_alloc|posix_memalign)[[:space:]]*\(' src/c/http_mvc.c || true; } | wc -l)
locks=$({ grep -nE '(^|[^[:alnum:]_])(pthread_|mtx_|futex|mutex|spinlock|refcount)' src/c/http_mvc.c || true; } | wc -l)
[[ "$heap" -eq 0 && "$locks" -eq 0 ]]
[[ "$(grep -c '^arbor_status arbor_http_mvc_' include/arborcore/http_mvc.h)" -eq 7 ]]
grep -Fq 'application_transport_handle_once' src/c/http_mvc.c
grep -Fq 'arbor_application_invoke(&application->mvc_capabilities' src/c/http_mvc.c
grep -Fq 'arbor_http_response_serialize(' src/c/http_mvc.c
grep -Fq 'exchange->request = scope.request;' src/c/http_mvc.c
grep -Fq 'const arbor_http_mvc_application *application;' src/c/http_mvc.c
grep -Fq '_Static_assert(sizeof(http1_exchange) == 56u' src/c/http_mvc.c
grep -Fq 'spans_overlap(mark_out, sizeof(*mark_out),' src/c/http_mvc.c
grep -Fq 'exchange->application, sizeof(*exchange->application)' src/c/http_mvc.c
grep -Fq 'region_overlaps_catalog(' src/c/http_mvc.c
grep -Fq 'HTTP1 mark-output/prepared HTTP1 application alias rejection' tests/c/http1_adversarial_test.c
grep -Fq 'HTTP1 mark-output/prepared MVC application alias rejection' tests/c/http1_adversarial_test.c
grep -Fq 'HTTP1 mark-output/MVC catalog alias rejection' tests/c/http1_adversarial_test.c
grep -Fq 'DATABASE_BEFORE_HELLO0=PROHIBITED' /root/Downloads/Arborcore-HTTP1-R0-design-freeze.txt 2>/dev/null || true

echo 'HTTP1_PUBLIC_SYMBOL_COUNT=7'
echo "HTTP1_MUTABLE_PRODUCTION_GLOBAL_COUNT=$mutable"
echo "HTTP1_HIDDEN_HEAP_COUNT=$heap"
echo "HTTP1_INTERNAL_LOCK_REFCOUNT_COUNT=$locks"
echo 'PASS: HTTP1 public surface, sidecar, transport reuse and resource policy'
