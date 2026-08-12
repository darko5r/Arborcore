#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PUBLIC="$ROOT/abi/arborcore-1.symbols"
INTERNAL="$ROOT/abi/arborcore-1.internal-symbols"

objects=(
  start write memory_threshold memory security bytes ascii bytes_scan parse_u64
  u64_checked range u64_format hex_codec percent_codec base64 buffer arena io net
  http_parser router event connection http_response request_target route_pattern server
)

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

for f in "$PUBLIC" "$INTERNAL"; do
  [[ -s "$f" ]] || { echo "FAIL: missing ABI classification file: $f" >&2; exit 2; }
  if [[ "$(sort -u "$f" | wc -l)" -ne "$(wc -l < "$f")" ]]; then
    echo "FAIL: duplicate symbol in $f" >&2
    exit 1
  fi
done

comm -12 <(sort "$PUBLIC") <(sort "$INTERNAL") > "$tmp/overlap"
if [[ -s "$tmp/overlap" ]]; then
  echo "FAIL: symbols classified both public and internal:" >&2
  cat "$tmp/overlap" >&2
  exit 1
fi

: > "$tmp/actual"
for stem in "${objects[@]}"; do
  obj="$ROOT/build/$stem.o"
  [[ -r "$obj" ]] || { echo "FAIL: missing production object $obj" >&2; exit 2; }
  nm -g --defined-only "$obj" | awk 'NF >= 3 {print $3}' >> "$tmp/actual"
done
sort -u -o "$tmp/actual" "$tmp/actual"
cat "$PUBLIC" "$INTERNAL" | sort -u > "$tmp/classified"

if ! diff -u "$tmp/classified" "$tmp/actual"; then
  echo "FAIL: every production ELF global must be classified exactly once." >&2
  exit 1
fi

for required in memory_secure_clear memory_equal_constant_time http_parse_request server_handle_http_once; do
  grep -qx "$required" "$PUBLIC" || { echo "FAIL: required stable ABI symbol missing: $required" >&2; exit 1; }
done
for forbidden in _start memory_copy_scalar memory_copy_qword memory_copy_rep \
                 buffer_append_prechecked_disjoint http_frame_scan \
                 io_read_retry net_socket_tcp4_flags router_dispatch write_all; do
  grep -qx "$forbidden" "$INTERNAL" || { echo "FAIL: required internal symbol misclassified: $forbidden" >&2; exit 1; }
done

echo "public_symbol_count=$(wc -l < "$PUBLIC")"
echo "internal_symbol_count=$(wc -l < "$INTERNAL")"
echo "PASS: every production ELF global has an explicit ABI-v1 classification"
