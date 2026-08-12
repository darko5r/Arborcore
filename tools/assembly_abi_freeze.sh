#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
OUT="${1:-$ROOT/build/assembly-abi-v1.freeze.env}"
mkdir -p "$(dirname "$OUT")"

prod_sha="$(arborcore_production_source_sha256 "$ROOT")"
prod_text="$(arborcore_production_text_bytes "$ROOT")"
policy_sha="$(arborcore_memory_policy_sha256 "$ROOT")"
public_sha="$(sha256sum "$ROOT/abi/arborcore-1.symbols" | awk '{print $1}')"
internal_sha="$(sha256sum "$ROOT/abi/arborcore-1.internal-symbols" | awk '{print $1}')"
layout_sha="$(sha256sum "$ROOT/abi/arborcore-1.layout" | awk '{print $1}')"
map_sha="$(sha256sum "$ROOT/abi/arborcore-1.map" | awk '{print $1}')"
static_sha="$(sha256sum "$ROOT/build/libarborcore.a" | awk '{print $1}')"
shared_sha="$(sha256sum "$ROOT/build/libarborcore.so.1" | awk '{print $1}')"

cat > "$OUT" <<EOT
ABI_NAME=ARBORCORE_ASSEMBLY_ABI
ABI_VERSION=1.0
TARGET_OS=Linux
TARGET_ARCH=x86_64
CALLING_CONVENTION=System_V_AMD64
REFERENCE_COMMIT=95c0229f8baed13583f0a55ddb6d097e2d38146f
PRODUCTION_SOURCE_SHA256=$prod_sha
PRODUCTION_TEXT_BYTES=$prod_text
MEMORY_POLICY_SHA256=$policy_sha
PUBLIC_SYMBOLS_SHA256=$public_sha
INTERNAL_SYMBOLS_SHA256=$internal_sha
LAYOUT_SHA256=$layout_sha
VERSION_SCRIPT_SHA256=$map_sha
STATIC_LIBRARY_SHA256=$static_sha
SHARED_LIBRARY_SHA256=$shared_sha
PUBLIC_SYMBOL_COUNT=$(wc -l < "$ROOT/abi/arborcore-1.symbols")
ABI_STATE=FREEZE_CANDIDATE
EOT

cat "$OUT"
echo "freeze_evidence=$OUT"
