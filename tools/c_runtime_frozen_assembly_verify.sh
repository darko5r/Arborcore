#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"

source tools/server_benchmark_common.sh
source abi/arborcore-1.freeze

current_source="$(arborcore_production_source_sha256 "$ROOT")"
if [[ "$current_source" != "$PRODUCTION_SOURCE_SHA256" ]]; then
    echo "FAIL: frozen Assembly production source changed." >&2
    echo "expected=$PRODUCTION_SOURCE_SHA256" >&2
    echo "current=$current_source" >&2
    exit 1
fi

if ! git diff --quiet "$FREEZE_COMMIT" -- src/asm; then
    echo "FAIL: C bridge modifies frozen src/asm relative to ABI freeze commit $FREEZE_COMMIT." >&2
    git diff --stat "$FREEZE_COMMIT" -- src/asm >&2 || true
    exit 1
fi

check_hash() {
    local expected="$1"
    local path="$2"
    local actual
    actual="$(sha256sum "$path" | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
        echo "FAIL: frozen identity mismatch for $path" >&2
        echo "expected=$expected" >&2
        echo "actual=$actual" >&2
        exit 1
    fi
}

check_hash "$PUBLIC_SYMBOLS_SHA256" abi/arborcore-1.symbols
check_hash "$INTERNAL_SYMBOLS_SHA256" abi/arborcore-1.internal-symbols
check_hash "$LAYOUT_SHA256" abi/arborcore-1.layout
check_hash "$VERSION_SCRIPT_SHA256" abi/arborcore-1.map

if [[ ! -f build/libarborcore.a || ! -f build/libarborcore.so.1.0.0 ]]; then
    echo "FAIL: canonical Assembly libraries are missing; run make library-readiness first." >&2
    exit 1
fi
check_hash "$STATIC_LIBRARY_SHA256" build/libarborcore.a
check_hash "$SHARED_LIBRARY_SHA256" build/libarborcore.so.1.0.0

exports="$(nm -D --defined-only build/libarborcore.so.1.0.0 \
  | awk 'NF >= 3 {n=$3; sub(/@.*/,"",n); if (n != "ARBORCORE_1.0") print n}' \
  | sort -u | wc -l | tr -d ' ')"
if [[ "$exports" != "$PUBLIC_SYMBOL_COUNT" ]]; then
    echo "FAIL: shared Assembly export count changed: $exports != $PUBLIC_SYMBOL_COUNT" >&2
    exit 1
fi

echo "frozen_assembly_source_sha256=$current_source"
echo "frozen_static_library_sha256=$STATIC_LIBRARY_SHA256"
echo "frozen_shared_library_sha256=$SHARED_LIBRARY_SHA256"
echo "frozen_public_symbol_count=$exports"
echo "PASS: C runtime consumes the frozen Assembly ABI without mutation"
