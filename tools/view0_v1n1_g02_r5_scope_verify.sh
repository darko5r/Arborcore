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

bash tools/view0_v1n1_g02_r4_scope_verify.sh >/dev/null

r5_paths=$(cat <<'PATHS'
tests/c/view0_v1n1_g02_r5_head_base_cardinality_adversarial_test.c
tests/c/view0_v1n1_g02_r5_head_base_cardinality_test.c
tools/view0_v1n1_g02_r5_contract_verify.sh
tools/view0_v1n1_g02_r5_gate.sh
tools/view0_v1n1_g02_r5_native_verify.sh
tools/view0_v1n1_g02_r5_scope_verify.sh
PATHS
)

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
while IFS= read -r path; do
    [[ -n "$path" ]] || continue
    grep -Fxq "$path" <<< "$actual" || fail "G02 R5 path missing: $path"
done <<< "$r5_paths"

cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
[[ "$cumulative_count" -ge 107 ]] || fail "G02 R5 cumulative path count regressed: $cumulative_count"

echo 'VIEW0_V1N1_G02_R5_RETAINED_R4_PATH_COUNT=101'
echo 'VIEW0_V1N1_G02_R5_NEW_PATH_COUNT=6'
echo "VIEW0_V1N1_G02_R5_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N1_G02_R5_EXTENSION_AWARE_SCOPE=YES'
echo 'VIEW0_V1N1_G02_R5_STAGED_CHANGES=NO'
echo 'PASS: G02 R5 source scope established over accepted G02 R4 verifier-R1 candidate with admitted higher-G02 extension awareness'
