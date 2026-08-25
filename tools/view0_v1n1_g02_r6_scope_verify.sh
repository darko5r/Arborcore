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

bash tools/view0_v1n1_g02_r5_scope_verify.sh >/dev/null

r6_paths=$(cat <<'PATHS'
tests/c/view0_v1n1_g02_r6_body_singleton_adversarial_test.c
tests/c/view0_v1n1_g02_r6_body_singleton_test.c
tools/view0_v1n1_g02_r6_contract_verify.sh
tools/view0_v1n1_g02_r6_gate.sh
tools/view0_v1n1_g02_r6_native_verify.sh
tools/view0_v1n1_g02_r6_scope_verify.sh
PATHS
)

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
while IFS= read -r path; do
    [[ -n "$path" ]] || continue
    grep -Fxq "$path" <<< "$actual" || fail "G02 R6 path missing: $path"
done <<< "$r6_paths"

cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
[[ "$cumulative_count" -ge 113 ]] || fail "G02 R6 cumulative path count regressed: $cumulative_count"

echo 'VIEW0_V1N1_G02_R6_RETAINED_R5_PATH_COUNT=107'
echo 'VIEW0_V1N1_G02_R6_NEW_PATH_COUNT=6'
echo "VIEW0_V1N1_G02_R6_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N1_G02_R6_EXTENSION_AWARE_SCOPE=YES'
echo 'VIEW0_V1N1_G02_R6_STAGED_CHANGES=NO'
echo 'PASS: G02 R6 source scope established over accepted G02 R5 candidate'
