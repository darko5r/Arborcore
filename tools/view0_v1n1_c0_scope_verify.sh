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

bash tools/view0_v1_scope_verify.sh >/dev/null

retained_paths=$(awk '
    /^required_paths=\$\(cat <</ { capture=1; next }
    /^__VIEW0_V1N0_PATHS__$/ { capture=0; next }
    capture { print }
' tools/view0_v1_native_foundation_verify.sh)
retained_count=$(printf '%s\n' "$retained_paths" | sed '/^$/d' | wc -l)
[[ "$retained_count" -eq 71 ]] || fail "retained V1N0 path inventory drift: $retained_count"

c0_paths=$(cat <<'PATHS'
tests/c/view0_v1n1_c0_facts_adversarial_test.c
tests/c/view0_v1n1_c0_facts_test.c
tools/view0_v1n1_c0_contract_verify.sh
tools/view0_v1n1_c0_gate.sh
tools/view0_v1n1_c0_native_verify.sh
tools/view0_v1n1_c0_scope_verify.sh
PATHS
)

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
while IFS= read -r path; do
    [[ -n "$path" ]] || continue
    grep -Fxq "$path" <<< "$actual" || fail "retained V1N0 path missing: $path"
done <<< "$retained_paths"
while IFS= read -r path; do
    [[ -n "$path" ]] || continue
    grep -Fxq "$path" <<< "$actual" || fail "C0 path missing: $path"
done <<< "$c0_paths"

cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
[[ "$cumulative_count" -ge 77 ]] || fail "C0 cumulative path count fell below retained floor: $cumulative_count"

echo 'VIEW0_V1N1_C0_RETAINED_V1N0_PATH_COUNT=71'
echo 'VIEW0_V1N1_C0_NEW_PATH_COUNT=6'
echo 'VIEW0_V1N1_C0_CUMULATIVE_PATH_COUNT=77'
echo 'VIEW0_V1N1_C0_EXTENSION_AWARE_SCOPE=YES'
echo 'VIEW0_V1N1_C0_STAGED_CHANGES=NO'
echo 'PASS: retained C0 source scope established under admitted higher V1N1 extension'
