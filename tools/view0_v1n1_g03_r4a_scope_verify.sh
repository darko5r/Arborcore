#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE='08b55954139a2e8daaf254f4935c8f7a31e3aa19'; TREE='eea1bab55dd90ab7f89bce5622db800a4f60e282'
PREDECESSOR_COUNT=155; PREDECESSOR_MANIFEST='ba050efd0c961a085cf60a5a1eb589017fa325d35f2b9670c73adae8263173ea'; EXPECTED_COUNT=164
fail(){ echo "FAIL: $*" >&2; exit 1; }
[[ "$(git branch --show-current)" == view0-presentation-core ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" && "$(git rev-parse HEAD^{tree})" == "$TREE" ]] || fail 'frozen HTTP1 base changed'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'
new_paths=$(cat <<'PATHS'
tests/c/view0_v1n1_g03_r4a_nothing_model_adversarial_test.c
tests/c/view0_v1n1_g03_r4a_nothing_model_test.c
tests/data/view0_v1n1_g03_r4a_subject_support.tsv
tools/c/view0_conformance/g03_r4a.c
tools/c/view0_conformance/g03_r4a.h
tools/view0_v1n1_g03_r4a_contract_verify.sh
tools/view0_v1n1_g03_r4a_gate.sh
tools/view0_v1n1_g03_r4a_native_verify.sh
tools/view0_v1n1_g03_r4a_scope_verify.sh
PATHS
)
while IFS= read -r p; do [[ -f "$p" ]] || fail "R4A path missing: $p"; done <<< "$new_paths"
actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
if grep -Fxq 'VIEW0_V1N1_G03_C0_SR1_SOURCE_REPAIR_CONTEXT=QUALIFIED' view/arborcore-view-core-1.contract; then
  [[ "$count" -eq 174 ]] || fail "R4A retained-under-C0-SR1 cumulative path count drift: $count expected=174"
  echo 'VIEW0_V1N1_G03_R4A_RETAINED_UNDER_C0_SR1=PASS'
else
  [[ "$count" -eq "$EXPECTED_COUNT" ]] || fail "R4A cumulative path count drift: $count expected=$EXPECTED_COUNT"
fi
while IFS= read -r p; do grep -Fxq "$p" <<< "$actual" || fail "R4A declared new path absent: $p"; done <<< "$new_paths"
for retained in tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c tools/c/view0_conformance/g03_r3a.c tests/data/view0_v1n1_g03_r3a_element_support.tsv tools/view0_v1n1_g03_r3a_gate.sh tools/view0_v1_lexbor_build.sh; do grep -Fxq "$retained" <<< "$actual" || fail "retained R3A path missing: $retained"; done
echo "VIEW0_V1N1_G03_R4A_RETAINED_R3A_PATH_COUNT=$PREDECESSOR_COUNT"
echo "VIEW0_V1N1_G03_R4A_RETAINED_R3A_MANIFEST_SHA256=$PREDECESSOR_MANIFEST"
echo 'VIEW0_V1N1_G03_R4A_NEW_PATH_COUNT=9'
echo "VIEW0_V1N1_G03_R4A_CUMULATIVE_PATH_COUNT=$count"
echo 'VIEW0_V1N1_G03_R4A_STAGED_CHANGES=NO'
echo 'PASS: G03 R4A source scope established over exact accepted R3A predecessor identity'
