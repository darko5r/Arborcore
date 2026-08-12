#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
PROFILE="${ARBORCORE_PERF_PROFILE:-local}"
BASE_COMMIT="cad044071fbb877239ec9c1621a65071cacca9e9"
cd "$ROOT"

echo "### CR0: frozen Assembly boundary"
make library-readiness
bash tools/c_runtime_frozen_assembly_verify.sh
bash tools/c_assembly_abi_header_verify.sh

echo
echo "### CR1-CR7: warning-clean C bridge construction and qualification"
make c-runtime-check
make c-runtime-sanitize
make c-runtime-dependency-verify
make c-runtime-reproducibility-verify

echo
echo "### CR7: frozen Assembly regression suite"
log="$(mktemp /tmp/arborcore-cr-assembly-check.XXXXXX)"
trap 'rm -f "$log"' EXIT
set +e
make check >"$log" 2>&1
rc=$?
set -e
cat "$log"
if [[ $rc -ne 0 ]]; then
    echo "FAIL: frozen Assembly regression suite exit=$rc" >&2
    exit "$rc"
fi
if grep -nEi 'warning:|clock skew' "$log"; then
    echo "FAIL: warning in frozen Assembly reconstruction" >&2
    exit 1
fi

echo
echo "### CR8: C bridge overhead qualification"
ARBORCORE_PERF_PROFILE="$PROFILE" make c-runtime-benchmark-verify

echo
echo "### CR8: final lower-layer identity"
make library-readiness >/dev/null
bash tools/c_runtime_frozen_assembly_verify.sh

source abi/arborcore-1.freeze
runtime_source_sha="$({
    find include/arborcore src/c -type f -print0 \
      | sort -z \
      | xargs -0 sha256sum
} | sha256sum | awk '{print $1}')"
runtime_archive_sha="$(sha256sum build/libarborcore_runtime.a | awk '{print $1}')"
runtime_symbol_count="$(nm -g --defined-only build/libarborcore_runtime.a | awk 'NF>=3 {print $3}' | grep '^arbor_' | sort -u | wc -l | tr -d ' ')"

cat > build/c-runtime-bridge.env <<EVIDENCE
CR_PHASE=CR0-CR8
CR_STATE=ADMIT_CANDIDATE
CR_BASE_COMMIT=$BASE_COMMIT
ASSEMBLY_ABI_VERSION=$ABI_VERSION
ASSEMBLY_ABI_FREEZE_COMMIT=$FREEZE_COMMIT
ASSEMBLY_PRODUCTION_SOURCE_SHA256=$PRODUCTION_SOURCE_SHA256
ASSEMBLY_STATIC_LIBRARY_SHA256=$STATIC_LIBRARY_SHA256
ASSEMBLY_SHARED_LIBRARY_SHA256=$SHARED_LIBRARY_SHA256
ASSEMBLY_PUBLIC_SYMBOL_COUNT=$PUBLIC_SYMBOL_COUNT
C_RUNTIME_SOURCE_SHA256=$runtime_source_sha
C_RUNTIME_ARCHIVE_SHA256=$runtime_archive_sha
C_RUNTIME_PUBLIC_SYMBOL_COUNT=$runtime_symbol_count
C_RUNTIME_DEFAULT_ASSEMBLY_LINKAGE=static
C_RUNTIME_SHARED_ASSEMBLY_EQUIVALENCE=qualified
C_RUNTIME_API_STATE=UNFROZEN_CONSTRUCTION
EVIDENCE

cat build/c-runtime-bridge.env

echo
echo "### C RUNTIME BRIDGE CR0-CR8 GATE PASSED"
echo "CR_DECISION=ADMIT_CANDIDATE"
