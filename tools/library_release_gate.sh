#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
FREEZE="$ROOT/abi/arborcore-1.freeze"
[[ -r "$FREEZE" ]] || { echo "FAIL: missing frozen ABI manifest" >&2; exit 2; }
# shellcheck disable=SC1090
source "$FREEZE"
source "$ROOT/tools/server_benchmark_common.sh"

actual_prod_sha="$(arborcore_production_source_sha256 "$ROOT")"
actual_policy_sha="$(arborcore_memory_policy_sha256 "$ROOT")"
actual_public_sha="$(sha256sum abi/arborcore-1.symbols | awk '{print $1}')"
actual_internal_sha="$(sha256sum abi/arborcore-1.internal-symbols | awk '{print $1}')"
actual_layout_sha="$(sha256sum abi/arborcore-1.layout | awk '{print $1}')"
actual_map_sha="$(sha256sum abi/arborcore-1.map | awk '{print $1}')"
freeze_manifest_sha="$(sha256sum abi/arborcore-1.freeze | awk '{print $1}')"
install_manifest_sha="$(sha256sum packaging/arborcore-library-files.list | awk '{print $1}')"

[[ "$actual_prod_sha" == "$PRODUCTION_SOURCE_SHA256" ]] || { echo "FAIL: frozen production source changed" >&2; exit 1; }
[[ "$actual_policy_sha" == "$MEMORY_POLICY_SHA256" ]] || { echo "FAIL: frozen memory policy changed" >&2; exit 1; }
[[ "$actual_public_sha" == "$PUBLIC_SYMBOLS_SHA256" ]] || { echo "FAIL: public ABI manifest changed" >&2; exit 1; }
[[ "$actual_internal_sha" == "$INTERNAL_SYMBOLS_SHA256" ]] || { echo "FAIL: internal symbol classification changed" >&2; exit 1; }
[[ "$actual_layout_sha" == "$LAYOUT_SHA256" ]] || { echo "FAIL: frozen ABI layout changed" >&2; exit 1; }
[[ "$actual_map_sha" == "$VERSION_SCRIPT_SHA256" ]] || { echo "FAIL: ABI version script changed" >&2; exit 1; }

echo "### L4: warning-clean frozen-core compatibility"
make clean >/dev/null
log="$(mktemp)"
trap 'rm -f "$log"' EXIT
set +e
{ make; make check; } >"$log" 2>&1
rc=$?
set -e
cat "$log"
[[ $rc -eq 0 ]] || { echo "FAIL: frozen-core compatibility check exit=$rc" >&2; exit $rc; }
if grep -nEi 'warning:|clock skew' "$log"; then
  echo "FAIL: warning-clean library release reconstruction" >&2
  exit 1
fi
echo "PASS: warning-clean frozen-core compatibility"
actual_prod_text="$(arborcore_production_text_bytes "$ROOT")"
[[ "$actual_prod_text" == "$PRODUCTION_TEXT_BYTES" ]] || { echo "FAIL: frozen production text size changed" >&2; exit 1; }

echo
echo "### L0/L2: canonical library products"
make library-readiness

echo
echo "### L1: independent reproducibility"
bash tools/library_reproducibility_verify.sh

echo
echo "### L1/L3: staged installation qualification"
bash tools/library_install_verify.sh

echo
echo "### L4: frozen ABI identity recheck"
make abi-surface-verify abi-dependency-verify abi-layout-verify security-shape-verify

static_sha="$(sha256sum build/libarborcore.a | awk '{print $1}')"
shared_sha="$(sha256sum build/libarborcore.so.1.0.0 | awk '{print $1}')"
[[ "$static_sha" == "$STATIC_LIBRARY_SHA256" ]] || { echo "FAIL: final static library hash" >&2; exit 1; }
[[ "$shared_sha" == "$SHARED_LIBRARY_SHA256" ]] || { echo "FAIL: final shared library hash" >&2; exit 1; }

out="$ROOT/build/library-release-v1.0.0.env"
cat > "$out" <<EOT
LIBRARY_NAME=Arborcore
LIBRARY_RELEASE_VERSION=$LIBRARY_RELEASE_VERSION
ASSEMBLY_ABI_VERSION=$ABI_VERSION
ASSEMBLY_ABI_STATE=$ABI_STATE
ASSEMBLY_ABI_FREEZE_COMMIT=$FREEZE_COMMIT
PRODUCTION_SOURCE_SHA256=$actual_prod_sha
PRODUCTION_TEXT_BYTES=$actual_prod_text
PUBLIC_SYMBOLS_SHA256=$actual_public_sha
LAYOUT_SHA256=$actual_layout_sha
STATIC_LIBRARY_FILE=libarborcore.a
STATIC_LIBRARY_SHA256=$static_sha
SHARED_LIBRARY_FILE=libarborcore.so.1.0.0
SHARED_LIBRARY_SONAME=libarborcore.so.1
SHARED_LIBRARY_SHA256=$shared_sha
PUBLIC_SYMBOL_COUNT=$PUBLIC_SYMBOL_COUNT
ABI_FREEZE_MANIFEST_SHA256=$freeze_manifest_sha
INSTALL_MANIFEST_SHA256=$install_manifest_sha
PACKAGING_STATE=RELEASE_CANDIDATE
EOT

cat "$out"
echo "release_evidence=$out"
echo
echo "### ARBORCORE LIBRARY PACKAGING L0-L4 GATE PASSED"
echo "LIBRARY_RELEASE_DECISION=ADMIT_CANDIDATE"
