#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
BASE='08b55954139a2e8daaf254f4935c8f7a31e3aa19'
TREE='eea1bab55dd90ab7f89bce5622db800a4f60e282'
PREDECESSOR_COUNT=137
PREDECESSOR_MANIFEST='245e39493f6ca6f7b5fdd2dc25dd8df8f3555dc0b2dc3be28a4eb4d94a693a08'
R2A_COUNT=146

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ "$(git branch --show-current)" == 'view0-presentation-core' ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD changed from frozen HTTP1 base'
[[ "$(git rev-parse HEAD^{tree})" == "$TREE" ]] || fail 'HTTP1 base tree changed'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'

new_paths=$(cat <<'PATHS'
tests/c/view0_v1n1_g03_r2a_content_model_adversarial_test.c
tests/c/view0_v1n1_g03_r2a_content_model_test.c
tests/data/view0_v1n1_g03_r2a_predicate_coverage.tsv
tools/c/view0_conformance/g03_r2a.c
tools/c/view0_conformance/g03_r2a.h
tools/view0_v1n1_g03_r2a_contract_verify.sh
tools/view0_v1n1_g03_r2a_gate.sh
tools/view0_v1n1_g03_r2a_native_verify.sh
tools/view0_v1n1_g03_r2a_scope_verify.sh
PATHS
)
while IFS= read -r path; do
    [[ -n "$path" && -f "$path" ]] || fail "R2A path missing: $path"
done <<< "$new_paths"

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R2A)
    [[ "$cumulative_count" -eq "$R2A_COUNT" ]] || fail "R2A cumulative path count drift: $cumulative_count expected=$R2A_COUNT" ;;
  0.1-VIEW0-V1N1-G03-R3A)
    [[ "$cumulative_count" -ge "$R2A_COUNT" ]] || fail "R2A retained path count regressed under R3A: $cumulative_count" ;;
  0.1-VIEW0-V1N1-G03-R4A)
    [[ "$cumulative_count" -ge 155 ]] || fail "R2A retained path count regressed under R4A: $cumulative_count" ;;
  *) fail "unexpected contract version for retained R2A scope: $current_version" ;;
esac
while IFS= read -r path; do
    grep -Fxq "$path" <<< "$actual" || fail "R2A declared new path absent from candidate: $path"
done <<< "$new_paths"

for retained in \
    tools/c/view0_conformance/g03_r1a.c \
    tests/data/view0_v1n1_g03_r1a_context_coverage.tsv \
    tools/view0_v1n1_g03_r1a_gate.sh \
    tools/view0_v1_lexbor_build.sh; do
    grep -Fxq "$retained" <<< "$actual" || fail "accepted R1A/LX1 retained path missing: $retained"
done

echo "VIEW0_V1N1_G03_R2A_RETAINED_R1A_LX1_PATH_COUNT=$PREDECESSOR_COUNT"
echo "VIEW0_V1N1_G03_R2A_RETAINED_R1A_LX1_MANIFEST_SHA256=$PREDECESSOR_MANIFEST"
echo 'VIEW0_V1N1_G03_R2A_NEW_PATH_COUNT=9'
echo "VIEW0_V1N1_G03_R2A_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N1_G03_R2A_STAGED_CHANGES=NO'
echo 'PASS: G03 R2A source scope established over exact accepted R1A/LX1 predecessor identity'
