#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/application-capability-kernel-repro"
mkdir -p "$OUT"

FILES=(
  Makefile
  application/arborcore-application-capability-kernel-1.contract
  docs/APPLICATION_CAPABILITY_KERNEL_AF2.md
  include/arborcore/capability.h
  src/c/capability_kernel.c
  tests/c/application_capability_kernel_test.c
  tools/application_capability_kernel_baseline_verify.sh
  tools/application_capability_kernel_contract_verify.sh
  tools/application_capability_kernel_gate.sh
  tools/application_capability_kernel_native_verify.sh
  tools/application_capability_kernel_reproducibility_verify.sh
  tools/application_capability_kernel_scope_verify.sh
)

for rel in "${FILES[@]}"; do [[ -f "$ROOT/$rel" ]] || { echo "FAIL: missing AF2 repro input $rel" >&2; exit 1; }; done
printf '%s\n' "${FILES[@]}" | LC_ALL=C sort > "$OUT/files.txt"

make_tar() {
  local destination="$1"
  (cd "$ROOT" && tar --sort=name --format=ustar --mtime='UTC 1970-01-01' --owner=0 --group=0 --numeric-owner -cf "$destination" -T "$OUT/files.txt")
}

make_tar "$OUT/a.tar"
make_tar "$OUT/b.tar"
cmp "$OUT/a.tar" "$OUT/b.tar"
sha="$(sha256sum "$OUT/a.tar" | awk '{print $1}')"
count="$(wc -l < "$OUT/files.txt")"
[[ "$count" -eq 12 ]]

cat > "$OUT/result.env" <<EVIDENCE
AF2_REPRODUCIBILITY_RESULT=PASS
AF2_ARCHIVE_SHA256=$sha
AF2_ARCHIVE_FILE_COUNT=$count
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: AF2 candidate source archive is byte-reproducible'
