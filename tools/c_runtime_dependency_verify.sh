#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"

runtime="build/libarborcore_runtime.a"
[[ -f "$runtime" ]] || { echo "FAIL: missing $runtime" >&2; exit 1; }

work="$(mktemp -d /tmp/arborcore-c-runtime-deps.XXXXXX)"
trap 'rm -rf "$work"' EXIT

nm -g --defined-only "$runtime" \
  | awk 'NF >= 3 {print $3}' | sort -u > "$work/defined"
nm -u "$runtime" \
  | awk '$1 == "U" {print $2} $2 == "U" {print $3}' | sort -u > "$work/undefined"
comm -23 "$work/undefined" "$work/defined" > "$work/external"
sort -u abi/arborcore-1.symbols > "$work/abi"

if [[ -s "$work/external" ]]; then
    while IFS= read -r symbol; do
        if ! grep -Fxq "$symbol" "$work/abi"; then
            echo "FAIL: C runtime has non-ABI external dependency: $symbol" >&2
            exit 1
        fi
    done < "$work/external"
fi

count="$(wc -l < "$work/external" | tr -d ' ')"
echo "c_runtime_assembly_dependency_count=$count"
cat "$work/external" | sed 's/^/dependency=/'
echo "PASS: C runtime depends only on frozen Assembly ABI-v1 symbols"
