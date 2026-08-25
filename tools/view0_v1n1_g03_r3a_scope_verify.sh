#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
BASE='08b55954139a2e8daaf254f4935c8f7a31e3aa19'
TREE='eea1bab55dd90ab7f89bce5622db800a4f60e282'
PREDECESSOR_COUNT=146
PREDECESSOR_MANIFEST='debcb28bcf096319a0b30191dda02e7431556c783909db431526baf910c69b42'
EXPECTED_COUNT=155

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ "$(git branch --show-current)" == 'view0-presentation-core' ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD changed from frozen HTTP1 base'
[[ "$(git rev-parse HEAD^{tree})" == "$TREE" ]] || fail 'HTTP1 base tree changed'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'

new_paths=$(cat <<'PATHS'
tests/c/view0_v1n1_g03_r3a_descendant_exclusions_adversarial_test.c
tests/c/view0_v1n1_g03_r3a_descendant_exclusions_test.c
tests/data/view0_v1n1_g03_r3a_element_support.tsv
tools/c/view0_conformance/g03_r3a.c
tools/c/view0_conformance/g03_r3a.h
tools/view0_v1n1_g03_r3a_contract_verify.sh
tools/view0_v1n1_g03_r3a_gate.sh
tools/view0_v1n1_g03_r3a_native_verify.sh
tools/view0_v1n1_g03_r3a_scope_verify.sh
PATHS
)
while IFS= read -r path; do
    [[ -n "$path" && -f "$path" ]] || fail "R3A path missing: $path"
done <<< "$new_paths"

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R3A) [[ "$cumulative_count" -eq "$EXPECTED_COUNT" ]] || fail "R3A cumulative path count drift: $cumulative_count expected=$EXPECTED_COUNT" ;;
  0.1-VIEW0-V1N1-G03-R4A) [[ "$cumulative_count" -ge "$EXPECTED_COUNT" ]] || fail "R3A retained path count regressed under R4A: $cumulative_count" ;;
  *) fail "unexpected contract version for retained R3A scope: $current_version" ;;
esac
while IFS= read -r path; do
    grep -Fxq "$path" <<< "$actual" || fail "R3A declared new path absent from candidate: $path"
done <<< "$new_paths"

for retained in \
    tools/c/view0_conformance/g03_r1a.c \
    tools/c/view0_conformance/g03_r2a.c \
    tests/data/view0_v1n1_g03_r1a_context_coverage.tsv \
    tests/data/view0_v1n1_g03_r2a_predicate_coverage.tsv \
    tools/view0_v1n1_g03_r2a_gate.sh \
    tools/view0_v1_lexbor_build.sh; do
    grep -Fxq "$retained" <<< "$actual" || fail "accepted R1A/R2A/LX1 retained path missing: $retained"
done

echo "VIEW0_V1N1_G03_R3A_RETAINED_R2A_PATH_COUNT=$PREDECESSOR_COUNT"
echo "VIEW0_V1N1_G03_R3A_RETAINED_R2A_MANIFEST_SHA256=$PREDECESSOR_MANIFEST"
echo 'VIEW0_V1N1_G03_R3A_NEW_PATH_COUNT=9'
echo "VIEW0_V1N1_G03_R3A_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N1_G03_R3A_STAGED_CHANGES=NO'
echo 'PASS: G03 R3A source scope established over exact accepted R2A predecessor identity'
