#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/application-service-runtime-repro"
mkdir -p "$OUT"

FILES=(
  Makefile
  application/arborcore-application-service-runtime-1.contract
  docs/APPLICATION_SERVICE_RUNTIME_AF3.md
  include/arborcore/application_service.h
  src/c/application_service.c
  tests/c/application_service_runtime_test.c
  tests/c/application_service_runtime_adversarial_test.c
  tests/asm/application_service_runtime_abi_test.asm
  tools/application_service_runtime_baseline_verify.sh
  tools/application_service_runtime_contract_verify.sh
  tools/application_service_runtime_native_verify.sh
  tools/application_service_runtime_abi_verify.sh
  tools/application_service_runtime_scope_verify.sh
  tools/application_service_runtime_reproducibility_verify.sh
  tools/application_service_runtime_gate.sh
)

for rel in "${FILES[@]}"; do
  [[ -f "$ROOT/$rel" ]] || { echo "FAIL: missing AF3 reproducibility input $rel" >&2; exit 1; }
done
printf '%s\n' "${FILES[@]}" | LC_ALL=C sort > "$OUT/files.txt"

make_tar() {
  local destination="$1"
  (cd "$ROOT" && tar --sort=name --format=ustar --mtime='UTC 1970-01-01' \
    --owner=0 --group=0 --numeric-owner --mode='u=rwX,go=rX' \
    -cf "$destination" -T "$OUT/files.txt")
}

make_tar "$OUT/a.tar"
make_tar "$OUT/b.tar"
cmp "$OUT/a.tar" "$OUT/b.tar"

extract_dir="$OUT/extracted"
rm -rf "$extract_dir"
mkdir -p "$extract_dir"
tar -xf "$OUT/a.tar" -C "$extract_dir"
for rel in "${FILES[@]}"; do
  expected_mode=644
  if [[ "$rel" == tools/*.sh ]]; then
    expected_mode=755
  fi
  actual_mode="$(stat -c '%a' "$extract_dir/$rel")"
  [[ "$actual_mode" == "$expected_mode" ]] || {
    echo "FAIL: normalized archive mode mismatch for $rel: expected $expected_mode got $actual_mode" >&2
    exit 1
  }
done
rm -rf "$extract_dir"
sha="$(sha256sum "$OUT/a.tar" | awk '{print $1}')"
count="$(wc -l < "$OUT/files.txt")"
[[ "$count" -eq 15 ]]

cat > "$OUT/result.env" <<EVIDENCE
AF3_REPRODUCIBILITY_RESULT=PASS
AF3_ARCHIVE_SHA256=$sha
AF3_ARCHIVE_FILE_COUNT=$count
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: AF3 candidate source archive is byte-reproducible'
