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

bash tools/view0_v1n1_g02_r1_scope_verify.sh >/dev/null

r2_paths=$(cat <<'PATHS'
tests/c/view0_v1n1_g02_r2_doctype_syntax_adversarial_test.c
tests/c/view0_v1n1_g02_r2_doctype_syntax_test.c
tools/view0_v1n1_g02_r2_contract_verify.sh
tools/view0_v1n1_g02_r2_gate.sh
tools/view0_v1n1_g02_r2_native_verify.sh
tools/view0_v1n1_g02_r2_scope_verify.sh
PATHS
)

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
while IFS= read -r path; do
    [[ -n "$path" ]] || continue
    grep -Fxq "$path" <<< "$actual" || fail "G02 R2 path missing: $path"
done <<< "$r2_paths"

cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
[[ "$cumulative_count" -ge 89 ]] || fail "G02 R2 cumulative path count fell below admitted R2 floor: $cumulative_count"

echo 'VIEW0_V1N1_G02_R2_RETAINED_R1_PATH_COUNT=83'
echo 'VIEW0_V1N1_G02_R2_NEW_PATH_COUNT=6'
echo "VIEW0_V1N1_G02_R2_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N1_G02_R2_EXTENSION_AWARE_SCOPE=YES'
echo 'VIEW0_V1N1_G02_R2_STAGED_CHANGES=NO'
echo 'PASS: retained G02 R2 source scope established under admitted higher G02 extension'
