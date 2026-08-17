#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"

OUT='build/ddd-support-repro'
MANIFEST="$OUT/candidate-manifest.tsv"
ARCHIVE="$OUT/candidate-source.tar"
RESULT="$OUT/result.env"
mkdir -p "$OUT"

paths=(
Makefile
application/arborcore-application-ddd-support-1.contract
docs/APPLICATION_DDD_SUPPORT_AF4.md
include/arborcore/ddd_support.h
src/c/ddd_support.c
tests/asm/ddd_support_abi_test.asm
tests/c/ddd_support_adversarial_test.c
tests/c/ddd_support_test.c
tools/ddd_support_abi_verify.sh
tools/ddd_support_baseline_verify.sh
tools/ddd_support_contract_verify.sh
tools/ddd_support_gate.sh
tools/ddd_support_native_verify.sh
tools/ddd_support_reproducibility_verify.sh
tools/ddd_support_scope_verify.sh
)

printf 'path\tmode\tbytes\tlines\tsha256\n' >"$MANIFEST"
for p in "${paths[@]}"; do
  [[ -f "$p" ]] || { echo "FAIL: missing reproducibility path $p" >&2; exit 1; }
  mode=100644
  [[ -x "$p" ]] && mode=100755
  printf '%s\t%s\t%s\t%s\t%s\n' \
    "$p" "$mode" "$(wc -c <"$p")" "$(wc -l <"$p")" \
    "$(sha256sum "$p" | awk '{print $1}')" >>"$MANIFEST"
done

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for p in "${paths[@]}"; do
  mkdir -p "$tmp/$(dirname "$p")"
  cp "$p" "$tmp/$p"
done
(
  cd "$tmp"
  LC_ALL=C tar \
    --sort=name --format=ustar \
    --owner=0 --group=0 --numeric-owner \
    --mtime='UTC 1970-01-01' \
    --mode='u=rwX,go=rX' \
    -cf "$ROOT/$ARCHIVE" "${paths[@]}"
)

MANIFEST_SHA=$(sha256sum "$MANIFEST" | awk '{print $1}')
ARCHIVE_SHA=$(sha256sum "$ARCHIVE" | awk '{print $1}')

cat >"$RESULT" <<EOF
AF4_MANIFEST_SHA256=$MANIFEST_SHA
AF4_ARCHIVE_SHA256=$ARCHIVE_SHA
AF4_ARCHIVE_FILE_COUNT=15
EOF

printf 'AF4_CANDIDATE_MANIFEST_SHA256=%s\n' "$MANIFEST_SHA"
printf 'AF4_REPRO_ARCHIVE_SHA256=%s\n' "$ARCHIVE_SHA"
printf 'AF4_REPRO_ARCHIVE_FILE_COUNT=15\n'
printf 'PASS: AF4 normalized-mode deterministic candidate archive\n'
