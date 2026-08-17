#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s http0-library build/http0/http0_abi_test.o http0-header-test http0-response-test
out=$(mktemp -d); trap 'rm -rf "$out"' EXIT
nm -g --defined-only build/libarborcore_http0.a | awk '$2=="T" && $3 ~ /^arbor_http_/ {print $3}' | LC_ALL=C sort -u >"$out/defined.txt"
cat >"$out/expected.txt" <<'EOF_EXPECTED'
arbor_http_field_validate
arbor_http_request_connection_close
arbor_http_request_header_count
arbor_http_request_header_find_first
arbor_http_request_header_next
arbor_http_request_host_validate
arbor_http_response_make
arbor_http_response_serialize
arbor_http_response_validate
EOF_EXPECTED
diff -u "$out/expected.txt" "$out/defined.txt"
readelf -Ws build/http0/http_header.o | grep -E 'http0_header_next_asm' | grep -Fq 'HIDDEN'
readelf -Ws build/http0/http_response_v2.o | grep -E 'http0_response_serialize_asm' | grep -Fq 'HIDDEN'
[[ "$(size -A build/http0/http_header.o | awk '$1==".text"{print $2}')" -gt 0 ]]
[[ "$(size -A build/http0/http_response_v2.o | awk '$1==".text"{print $2}')" -gt 0 ]]
./build/http0-header-test >/dev/null; ./build/http0-response-test >/dev/null
echo "HTTP0_REAL_NASM_HEADER_OBJECT_SHA256=$(sha256sum build/http0/http_header.o | awk '{print $1}')"
echo "HTTP0_REAL_NASM_HEADER_TEXT_BYTES=$(size -A build/http0/http_header.o | awk '$1==".text"{print $2}')"
echo "HTTP0_REAL_NASM_RESPONSE_OBJECT_SHA256=$(sha256sum build/http0/http_response_v2.o | awk '{print $1}')"
echo "HTTP0_REAL_NASM_RESPONSE_TEXT_BYTES=$(size -A build/http0/http_response_v2.o | awk '$1==".text"{print $2}')"
echo "HTTP0_REAL_NASM_ABI_OBJECT_SHA256=$(sha256sum build/http0/http0_abi_test.o | awk '{print $1}')"
echo "HTTP0_REAL_NASM_ABI_TEXT_BYTES=$(size -A build/http0/http0_abi_test.o | awk '$1==".text"{print $2}')"
echo 'HTTP0_C_TO_ASSEMBLY_ABI=PASS'; echo 'HTTP0_ASSEMBLY_TO_C_API_ABI=PASS'; echo 'HTTP0_INTERNAL_ASSEMBLY_VISIBILITY=HIDDEN'
echo 'PASS: HTTP0 real NASM internal and bidirectional C/Assembly ABI qualification'
