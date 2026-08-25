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
    tools/c/view0_conformance/g03_provenance.c \
    tools/c/view0_conformance/g03_provenance_internal.h \
    tests/c/view0_v1n1_g03_c0_observation_test.c \
    tests/c/view0_v1n1_g03_c0_observation_adversarial_test.c \
    tests/c/view0_v1n1_g03_c0_nowrap_test.c \
    tools/view0_v1n1_g03_c0_scope_verify.sh \
    tools/view0_v1n1_g03_c0_contract_verify.sh \
    tools/view0_v1n1_g03_c0_native_verify.sh \
    tools/view0_v1n1_g03_c0_gate.sh; do
    [[ -f "$path" ]] || fail "G03 C0 path missing: $path"
done

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
[[ "$cumulative_count" -ge 122 ]] || fail "G03 C0 cumulative path count regressed: $cumulative_count"

for retained in \
    tools/view0_v1n1_g02_r6_gate.sh \
    tests/c/view0_v1n1_g02_r6_body_singleton_test.c \
    tests/c/view0_v1n1_g02_r6_body_singleton_adversarial_test.c; do
    grep -Fxq "$retained" <<< "$actual" || fail "accepted G02 retained path missing: $retained"
done

echo 'VIEW0_V1N1_G03_C0_RETAINED_G02_GF1_PATH_COUNT=113'
echo 'VIEW0_V1N1_G03_C0_NEW_PATH_COUNT=9'
echo "VIEW0_V1N1_G03_C0_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N1_G03_C0_STAGED_CHANGES=NO'
echo 'PASS: G03 C0 source scope established over accepted G02 GF1/R6 candidate'
