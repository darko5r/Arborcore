#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
BASE='08b55954139a2e8daaf254f4935c8f7a31e3aa19'

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ "$(git branch --show-current)" == 'view0-presentation-core' ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD changed from frozen HTTP1 base'
[[ "$(git rev-parse HEAD^{tree})" == 'eea1bab55dd90ab7f89bce5622db800a4f60e282' ]] || fail 'HTTP1 base tree changed'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'

for path in \
    tests/c/view0_v1n1_g03_c0_lifecycle_test.c \
    tests/c/view0_v1n1_g03_c0_lifecycle_adversarial_test.c \
    tools/view0_v1n1_g03_c0_l1_scope_verify.sh \
    tools/view0_v1n1_g03_c0_l1_contract_verify.sh \
    tools/view0_v1n1_g03_c0_l1_native_verify.sh \
    tools/view0_v1n1_g03_c0_l1_gate.sh; do
    [[ -f "$path" ]] || fail "G03 C0-L1 path missing: $path"
done

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
[[ "$cumulative_count" -ge 128 ]] || fail "G03 C0-L1 cumulative path count regressed: $cumulative_count"

for retained in \
    tools/c/view0_conformance/g03_provenance.c \
    tests/c/view0_v1n1_g03_c0_observation_test.c \
    tests/c/view0_v1n1_g03_c0_observation_adversarial_test.c \
    tests/c/view0_v1n1_g03_c0_nowrap_test.c; do
    grep -Fxq "$retained" <<< "$actual" || fail "accepted G03 C0 retained path missing: $retained"
done

echo 'VIEW0_V1N1_G03_C0_L1_RETAINED_G03_C0_PATH_COUNT=122'
echo 'VIEW0_V1N1_G03_C0_L1_NEW_PATH_COUNT=6'
echo "VIEW0_V1N1_G03_C0_L1_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N1_G03_C0_L1_STAGED_CHANGES=NO'
echo 'PASS: G03 C0-L1 source scope established over accepted G03 C0 candidate'
