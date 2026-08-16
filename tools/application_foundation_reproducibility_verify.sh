#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/application-foundation-repro"
mkdir -p "$OUT"
cd "$ROOT"

FILES=(
  Makefile
  application/arborcore-application-ddd-mvc-foundation-1.contract
  docs/APPLICATION_DDD_MVC_FOUNDATION.md
  include/arborcore/application.h
  src/c/application_foundation.c
  tests/c/application_foundation_test.c
  tools/application_foundation_contract_verify.sh
  tools/application_foundation_frozen_layers_verify.sh
  tools/application_foundation_gate.sh
  tools/application_foundation_native_verify.sh
  tools/application_foundation_reproducibility_verify.sh
  tools/application_foundation_scope_verify.sh
)
for rel in "${FILES[@]}"; do [[ -f "$rel" ]] || { echo "FAIL: missing AF reproducibility path $rel" >&2; exit 1; }; done
printf '%s\n' "${FILES[@]}" | LC_ALL=C sort > "$OUT/files.txt"
make_tar() {
  local dest="$1"
  tar --sort=name --format=ustar --mtime='UTC 1970-01-01' --owner=0 --group=0 --numeric-owner \
    -cf "$dest" -T "$OUT/files.txt"
}
make_tar "$OUT/a.tar"
make_tar "$OUT/b.tar"
cmp "$OUT/a.tar" "$OUT/b.tar"
sha="$(sha256sum "$OUT/a.tar" | awk '{print $1}')"
count="$(wc -l < "$OUT/files.txt")"
[[ "$count" -eq 12 ]]
printf 'AF0_AF1_REPRODUCIBILITY_RESULT=PASS\nAF0_AF1_ARCHIVE_SHA256=%s\nAF0_AF1_ARCHIVE_FILE_COUNT=%s\n' \
  "$sha" "$count" | tee "$OUT/result.env"
echo 'PASS: AF0-AF1 candidate source archive is byte-reproducible'
