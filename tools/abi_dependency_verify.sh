#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PUBLIC="$ROOT/abi/arborcore-1.symbols"
INTERNAL="$ROOT/abi/arborcore-1.internal-symbols"
OUT="$ROOT/build/assembly-abi-dependencies.tsv"
objects=(
  start write memory_threshold memory security bytes ascii bytes_scan parse_u64
  u64_checked range u64_format hex_codec percent_codec base64 buffer arena io net
  http_parser router event connection http_response request_target route_pattern server
)

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
cat "$PUBLIC" "$INTERNAL" | sort -u > "$tmp/classified"
printf 'object\tsymbol\n' > "$OUT"

for stem in "${objects[@]}"; do
  obj="$ROOT/build/$stem.o"
  [[ -r "$obj" ]] || { echo "FAIL: missing $obj" >&2; exit 2; }
  while IFS= read -r sym; do
    [[ -z "$sym" ]] && continue
    printf '%s\t%s\n' "$stem.o" "$sym" >> "$OUT"
    if ! grep -Fxq "$sym" "$tmp/classified"; then
      echo "FAIL: unclassified/external production dependency $stem.o -> $sym" >&2
      exit 1
    fi
  done < <(nm -u "$obj" | awk 'NF {print $NF}' | sort -u)
done

count="$(( $(wc -l < "$OUT") - 1 ))"
echo "dependency_edge_count=$count"
echo "dependency_inventory=$OUT"
echo "PASS: all production undefined symbols resolve inside the classified Assembly core"
