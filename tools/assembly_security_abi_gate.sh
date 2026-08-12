#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROFILE="${ARBORCORE_PERF_PROFILE:-local}"
REFERENCE="${ARBORCORE_QUALIFICATION_REFERENCE_COMMIT:-95c0229f8baed13583f0a55ddb6d097e2d38146f}"
REFERENCE_TEXT=11595
MAX_GROWTH_BYTES=256
MAX_TEXT=$((REFERENCE_TEXT + MAX_GROWTH_BYTES))

cd "$ROOT"

echo "### Assembly Security/ABI S9: warning-clean reconstruction"
log="$(mktemp)"
trap 'rm -f "$log"' EXIT
set +e
{
  make clean
  make
  make check
} >"$log" 2>&1
rc=$?
set -e
cat "$log"
if (( rc != 0 )); then
  echo "FAIL: full reconstruction/check exit=$rc" >&2
  exit "$rc"
fi
if grep -nEi 'warning:|clock skew' "$log"; then
  echo "FAIL: warning-clean reconstruction violated" >&2
  exit 1
fi
echo "PASS: warning-clean full reconstruction"

echo
echo "### S0/S5/S6: ABI surface classification"
make abi-surface-verify
make abi-dependency-verify

echo
echo "### S1/S2: security primitive shape and properties"
make core-security-property-test
make security-shape-verify

echo
echo "### S3/S4: layout/lifetime/adversarial contracts"
make abi-layout-verify
# make check above is the authoritative aggregate hostile-boundary suite.
echo "PASS: A-E adversarial/property suite retained under ABI freeze"

echo
echo "### S9: production-size evidence"
source "$ROOT/tools/server_benchmark_common.sh"
text="$(arborcore_production_text_bytes "$ROOT")"
printf 'assembly_abi_reference_text_bytes=%s\n' "$REFERENCE_TEXT"
printf 'current_production_text_bytes=%s\n' "$text"
printf 'delta_bytes=%+d\n' "$((text - REFERENCE_TEXT))"
printf 'security_abi_text_ceiling=%s\n' "$MAX_TEXT"
size build/security.o build/memory.o build/arena.o build/http_parser.o build/event.o build/connection.o build/server.o
if (( text > MAX_TEXT )); then
  echo "FAIL: security/ABI phase exceeds fixed +${MAX_GROWTH_BYTES}-byte review ceiling." >&2
  exit 1
fi

echo
echo "### S7/S8: static/shared library readiness"
make libarborcore-static libarborcore-shared
make library-readiness

echo
echo "### S9: environment-qualified performance"
ARBORCORE_QUALIFICATION_REFERENCE_COMMIT="$REFERENCE" \
ARBORCORE_PERF_PROFILE="$PROFILE" \
bash "$ROOT/tools/server_performance_qualified.sh" candidate

echo
echo "### S10: rebuild final library artifacts after performance qualification"
# The paired-performance workflow may clean/rebuild the candidate build/
# directory. Reconstruct and revalidate the deterministic library artifacts
# from the final qualified source immediately before freezing their hashes.
make library-readiness

echo
echo "### S10: Assembly ABI v1 freeze evidence"
bash "$ROOT/tools/assembly_abi_freeze.sh"

echo
echo "### ASSEMBLY SECURITY + ABI FREEZE GATE PASSED"
echo "reference_commit=$REFERENCE"
printf 'production_source_sha256='; arborcore_production_source_sha256 "$ROOT"
printf 'production_text_bytes='; arborcore_production_text_bytes "$ROOT"
printf 'memory_policy_sha256='; arborcore_memory_policy_sha256 "$ROOT"
printf 'abi_public_symbols_sha256='; sha256sum abi/arborcore-1.symbols | awk '{print $1}'
printf 'abi_layout_sha256='; sha256sum abi/arborcore-1.layout | awk '{print $1}'
echo "ABI_FREEZE_DECISION=ADMIT_CANDIDATE"
