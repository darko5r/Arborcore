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
tests/asm/view0_c4_abi_test.asm
tests/c/view0_c1_adversarial_test.c
tests/c/view0_c1_test.c
tests/c/view0_c2_adversarial_test.c
tests/c/view0_c2_test.c
tests/c/view0_c3_adversarial_test.c
tests/c/view0_c3_test.c
tests/c/view0_c4_adversarial_test.c
tests/c/view0_c4_test.c
tests/c/view0_m1_adversarial_test.c
tests/c/view0_m1_integration_test.c
tests/c/view0_m1_utf8_test.c
tests/c/view0_t1_adversarial_test.c
tests/c/view0_t1_test.c
tests/c/view0_v1_render_artifacts.c
tests/view0/v1/invalid/no-doctype.html
tests/view0/v1/invalid/ul-text.html
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
tools/view0_c4_abi_verify.sh
tools/view0_c4_baseline_verify.sh
tools/view0_c4_contract_verify.sh
tools/view0_c4_gate.sh
tools/view0_c4_scope_verify.sh
tools/view0_m1_baseline_verify.sh
tools/view0_m1_contract_verify.sh
tools/view0_m1_gate.sh
tools/view0_m1_native_verify.sh
tools/view0_m1_scope_verify.sh
tools/view0_t1_baseline_verify.sh
tools/view0_t1_contract_verify.sh
tools/view0_t1_gate.sh
tools/view0_t1_native_verify.sh
tools/view0_t1_scope_verify.sh
tools/view0_v1_baseline_verify.sh
tools/view0_v1_checker_acquire.sh
tools/view0_v1_conformance_verify.sh
tools/view0_v1_contract_verify.sh
tools/view0_v1_gate.sh
tools/view0_v1_native_verify.sh
tools/view0_v1_scope_verify.sh
view/arborcore-view-core-1.contract
PATHS
)
actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
while IFS= read -r path; do
  [[ -n "$path" ]] || continue
  grep -Fxq "$path" <<< "$actual" || { echo "FAIL: VIEW0 V1 required path missing under cumulative extension: $path" >&2; exit 1; }
done <<< "$required"
required_count=$(printf '%s\n' "$required" | sed '/^$/d' | wc -l)
cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
[[ "$required_count" -eq 59 ]]
[[ "$cumulative_count" -ge "$required_count" ]]
echo "VIEW0_V1_REQUIRED_PATH_COUNT=$required_count"
echo "VIEW0_V1_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1_EXTENSION_AWARE_SCOPE=YES'
echo 'VIEW0_V1_STAGED_CHANGES=NO'
echo 'PASS: VIEW0 V1 required construction paths remain present under cumulative native extension'
