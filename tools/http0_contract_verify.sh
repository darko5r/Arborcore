#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
C=http/arborcore-http-message-semantics-1.contract; H=include/arborcore/http.h; S=src/c/http.c; A1=src/asm/http_header.asm; A2=src/asm/http_response_v2.asm
required=(
'ARBORCORE_HTTP_MESSAGE_SEMANTICS_VERSION=0.1-HTTP0-R3' 'HTTP0_ASSEMBLY_ABI_V1_REOPEN=NO' 'HTTP0_AF1_RESPONSE_PLAN_REOPEN=NO'
'HTTP0_AF0_AF4_REOPEN=NO' 'HTTP0_MVC0_PUBLIC_API_REOPEN=NO' 'HTTP0_PUBLIC_FUNCTION_COUNT=9'
'HTTP0_REQUEST_HEADERS=ZERO_COPY_BORROWED_ORDERED_FIELD_LINES' 'HTTP0_REQUEST_HEADER_DUPLICATES=PRESERVED' 'HTTP0_REQUEST_OUTPUT_ALIAS_WITH_BORROWED_REQUEST_STORAGE=REJECT' 'HTTP0_REQUEST_HOST_ZONE_ID=REJECT_OBSOLETE_RFC6874_URI_EXTENSION'
'HTTP0_REQUEST_CONNECTION_CLOSE=FORCES_CLOSE_AFTER_CURRENT_RESPONSE' 'HTTP0_MALFORMED_CONNECTION_ERROR_RESPONSE=SERIALIZABLE_AND_FORCE_CLOSE'
'HTTP0_RESPONSE_FIELDS=ORDERED_CALLER_OWNED_ARRAY' 'HTTP0_RESPONSE_REPEATED_SET_COOKIE=PRESERVED' 'HTTP0_RESPONSE_CONSTRUCTOR_SOURCE_OUTPUT_ALIAS=REJECT' 'HTTP0_SERIALIZER_CONTROL_OUTPUT_ALIAS=REJECT' 'HTTP0_RESPONSE_PREFIX_EXTENSION_ALIAS_REGION=FULL_DECLARED_STRUCT_SIZE' 'HTTP0_INTERNAL_C_ASSEMBLY_RESPONSE_ARG_OFFSETS=STATIC_ASSERTED'
'HTTP0_APPLICATION_RESERVED_FIELDS=CONTENT_LENGTH_CONNECTION_TRANSFER_ENCODING_TRAILER_UPGRADE' 'HTTP0_RESPONSE_CONNECT_2XX=REJECT_UNTIL_TUNNEL_SUPPORT' 'HTTP0_HEAD_BODY_BYTES=SUPPRESSED'
'HTTP0_204_CONTENT_LENGTH=OMIT' 'HTTP0_304_CONTENT_LENGTH=OMIT_BY_HTTP0_POLICY' 'HTTP0_HIDDEN_HEAP=ZERO'
'HTTP0_MVC_INTEGRATION=DEFERRED_HTTP1' 'HTTP0_HTTP1_PRESENTATION_CHANNEL=CONTROLLED_DESIGN_REQUIRED_DYNAMIC_HEADERS_NOT_IN_AF1_RESPONSE_PLAN' 'HTTP0_ORIGIN_SERVER_DATE_POLICY=DEFERRED_HTTP1_CLOCKED_SERVER_INTEGRATION' 'HTTP0_ABSOLUTE_FORM_REQUEST_TARGET=DEFERRED_FUTURE_HTTP_REQUEST_TARGET_COMPLIANCE' 'HTTP0_DATABASE_BEFORE_HELLO0=PROHIBITED')
for line in "${required[@]}"; do grep -Fqx "$line" "$C" || { echo "FAIL: missing HTTP0 contract line: $line" >&2; exit 1; }; done
[[ "$(grep -Ec '^arbor_status arbor_http_[a-z0-9_]+\(' "$H")" -eq 9 ]]
[[ "$(grep -Ec '^arbor_status arbor_http_[a-z0-9_]+\(' "$S")" -eq 9 ]]
if grep -nE '(^|[^[:alnum:]_])(malloc|calloc|realloc|free|aligned_alloc|posix_memalign)[[:space:]]*\(' "$S"; then echo 'FAIL: hidden heap primitive' >&2; exit 1; fi
if grep -nE '(^|[^[:alnum:]_])(pthread_|mtx_|atomic_|futex|mutex|spinlock|refcount)' "$S"; then echo 'FAIL: hidden lock/refcount primitive' >&2; exit 1; fi
tmp_obj=$(mktemp); trap 'rm -f "$tmp_obj"' EXIT
cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O2 -fdata-sections -c "$S" -o "$tmp_obj"
# Count actual mutable OBJECT symbols in writable storage sections. Const pointer
# tables can reside in .data.rel.ro.* in relocatable objects because they need
# relocation, but become read-only under RELRO and are not mutable globals.
mutable_data_count=$(objdump -t "$tmp_obj" | awk '
  $3 == "O" && ($4 == ".data" || $4 ~ /^\.data\.[^.]+$/ ||
                  $4 == ".bss"  || $4 ~ /^\.bss\./ ||
                  $4 == ".sdata" || $4 ~ /^\.sdata\./ ||
                  $4 == ".sbss" || $4 ~ /^\.sbss\./ ||
                  $4 == ".tdata" || $4 ~ /^\.tdata\./ ||
                  $4 == ".tbss" || $4 ~ /^\.tbss\./) &&
                  $4 !~ /^\.data\.rel\.ro/ {c++}
  END {print c+0}')
[[ "$mutable_data_count" -eq 0 ]] || { objdump -t "$tmp_obj" >&2; echo 'FAIL: mutable production data symbol found' >&2; exit 1; }
[[ "$(grep -c '^_Static_assert(offsetof(http0_asm_response_args,' "$S")" -eq 10 ]] || { echo 'FAIL: internal C/Assembly response-arg offset assertions incomplete' >&2; exit 1; }
[[ "$(grep -Rhs '^global http0_.*:function hidden$' "$A1" "$A2" | wc -l)" -eq 2 ]]
! grep -Fq 'http0_header_next_asm' abi/arborcore-1.symbols; ! grep -Fq 'http0_response_serialize_asm' abi/arborcore-1.symbols
python3 - <<'PY_DOC'
from pathlib import Path
text=Path('docs/HTTP_MESSAGE_SEMANTICS_HTTP0.md').read_text()
for item in ['zero-copy','Host','Connection','Set-Cookie','HEAD','presentation metadata channel','Date','absolute-form','HELLO0','MariaDB']:
    if item not in text: raise SystemExit('FAIL: HTTP0 documentation missing '+item)
PY_DOC
echo 'HTTP0_PUBLIC_SYMBOL_COUNT=9'; echo "HTTP0_MUTABLE_PRODUCTION_GLOBAL_COUNT=$mutable_data_count"; echo 'HTTP0_HIDDEN_HEAP_COUNT=0'
echo 'HTTP0_INTERNAL_LOCK_REFCOUNT_COUNT=0'; echo 'HTTP0_ASSEMBLY_ABI_V1_EXTENSION_COUNT=0'
echo 'PASS: HTTP0 contract, public surface and layer policy'
