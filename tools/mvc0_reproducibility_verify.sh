#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"; cd "$ROOT"
OUT='build/mvc0-repro'; MANIFEST="$OUT/candidate-manifest.tsv"; ARCHIVE="$OUT/candidate-source.tar"; RESULT="$OUT/result.env"; mkdir -p "$OUT"
paths=(
Makefile
mvc/arborcore-mvc-core-transport-1.contract
docs/MVC_CORE_TRANSPORT_MVC0.md
include/arborcore/application_transport.h
include/arborcore/mvc.h
src/asm/application_transport.asm
src/c/application_transport.c
src/c/mvc.c
tests/asm/mvc0_abi_test.asm
tests/c/mvc_core_test.c
tests/c/mvc_adversarial_test.c
tests/c/mvc_integration_test.c
tests/c/mvc_end_to_end_test.c
tools/mvc0_baseline_verify.sh
tools/mvc0_contract_verify.sh
tools/mvc0_native_verify.sh
tools/mvc0_abi_verify.sh
tools/mvc0_scope_verify.sh
tools/mvc0_reproducibility_verify.sh
tools/mvc0_gate.sh
)
printf 'path\tmode\tbytes\tlines\tsha256\n' >"$MANIFEST"
for p in "${paths[@]}"; do [[ -f "$p" ]] || { echo "FAIL: missing reproducibility path $p" >&2; exit 1; }; mode=100644; [[ -x "$p" ]] && mode=100755; printf '%s\t%s\t%s\t%s\t%s\n' "$p" "$mode" "$(wc -c <"$p")" "$(wc -l <"$p")" "$(sha256sum "$p"|awk '{print $1}')" >>"$MANIFEST"; done
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
for p in "${paths[@]}"; do mkdir -p "$tmp/$(dirname "$p")"; cp "$p" "$tmp/$p"; done
(cd "$tmp" && LC_ALL=C tar --sort=name --format=ustar --owner=0 --group=0 --numeric-owner --mtime='UTC 1970-01-01' --mode='u=rwX,go=rX' -cf "$ROOT/$ARCHIVE" "${paths[@]}")
MSHA=$(sha256sum "$MANIFEST"|awk '{print $1}'); ASHA=$(sha256sum "$ARCHIVE"|awk '{print $1}')
cat >"$RESULT" <<EOF
MVC0_MANIFEST_SHA256=$MSHA
MVC0_ARCHIVE_SHA256=$ASHA
MVC0_ARCHIVE_FILE_COUNT=20
EOF
printf 'MVC0_CANDIDATE_MANIFEST_SHA256=%s\nMVC0_REPRO_ARCHIVE_SHA256=%s\nMVC0_REPRO_ARCHIVE_FILE_COUNT=20\n' "$MSHA" "$ASHA"
printf 'PASS: MVC0 normalized-mode deterministic candidate archive\n'
