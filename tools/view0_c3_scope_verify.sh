#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE=08b55954139a2e8daaf254f4935c8f7a31e3aa19
[[ "$(git branch --show-current)" == 'view0-presentation-core' ]]
[[ "$(git rev-parse HEAD)" == "$BASE" ]]
[[ -z "$(git diff --cached --name-only)" ]]
required=$(cat <<'PATHS'
Makefile
docs/VIEW_CORE_VIEW0.md
include/arborcore/view.h
src/c/view.c
tests/c/view0_c1_adversarial_test.c
tests/c/view0_c1_test.c
tests/c/view0_c2_adversarial_test.c
tests/c/view0_c2_test.c
tests/c/view0_c3_adversarial_test.c
tests/c/view0_c3_test.c
tools/view0_c1_baseline_verify.sh
tools/view0_c1_contract_verify.sh
tools/view0_c1_gate.sh
tools/view0_c1_native_verify.sh
tools/view0_c1_scope_verify.sh
tools/view0_c2_baseline_verify.sh
tools/view0_c2_contract_verify.sh
tools/view0_c2_gate.sh
tools/view0_c2_native_verify.sh
tools/view0_c2_scope_verify.sh
tools/view0_c3_baseline_verify.sh
tools/view0_c3_contract_verify.sh
tools/view0_c3_gate.sh
tools/view0_c3_native_verify.sh
tools/view0_c3_scope_verify.sh
view/arborcore-view-core-1.contract
PATHS
)
actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
while IFS= read -r path; do
  [[ -n "$path" ]] || continue
  printf '%s\n' "$actual" | grep -Fxq "$path" || { echo "FAIL: required VIEW0 C3 path missing from cumulative candidate: $path" >&2; exit 1; }
done <<< "$required"
echo 'VIEW0_C3_REQUIRED_PATH_COUNT=26'
echo 'VIEW0_C3_EXTENSION_AWARE_SCOPE=YES'
echo 'VIEW0_C3_STAGED_CHANGES=NO'
echo 'PASS: VIEW0 C3 required construction paths remain present in cumulative VIEW0 candidate'
