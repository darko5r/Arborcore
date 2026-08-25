#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
BASE='08b55954139a2e8daaf254f4935c8f7a31e3aa19'
TREE='eea1bab55dd90ab7f89bce5622db800a4f60e282'

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ "$(git branch --show-current)" == 'view0-presentation-core' ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD changed from frozen HTTP1 base'
[[ "$(git rev-parse HEAD^{tree})" == "$TREE" ]] || fail 'HTTP1 base tree changed'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'

for path in \
    tools/c/view0_conformance/g03_r1a.c \
    tools/c/view0_conformance/g03_r1a.h \
    tests/c/view0_v1n1_g03_r1a_element_context_test.c \
    tests/c/view0_v1n1_g03_r1a_element_context_adversarial_test.c \
    tests/data/view0_v1n1_g03_r1a_context_coverage.tsv \
    tools/view0_v1n1_g03_r1a_scope_verify.sh \
    tools/view0_v1n1_g03_r1a_contract_verify.sh \
    tools/view0_v1n1_g03_r1a_native_verify.sh \
    tools/view0_v1n1_g03_r1a_gate.sh; do
    [[ -f "$path" ]] || fail "G03 R1A path missing: $path"
done

actual=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
cumulative_count=$(printf '%s\n' "$actual" | sed '/^$/d' | wc -l)
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R1A)
    [[ "$cumulative_count" -eq 137 ]] || fail "G03 R1A cumulative path count drift: $cumulative_count expected=137" ;;
  0.1-VIEW0-V1N1-G03-R2A)
    [[ "$cumulative_count" -ge 137 ]] || fail "G03 R1A retained path count regressed under R2A: $cumulative_count" ;;
  0.1-VIEW0-V1N1-G03-R3A)
    [[ "$cumulative_count" -ge 146 ]] || fail "G03 R1A retained path count regressed under R3A: $cumulative_count" ;;
  0.1-VIEW0-V1N1-G03-R4A)
    [[ "$cumulative_count" -ge 155 ]] || fail "G03 R1A retained path count regressed under R4A: $cumulative_count" ;;
  *) fail "unexpected contract version for retained R1A scope: $current_version" ;;
esac

for retained in \
    tools/c/view0_conformance/g03_provenance.c \
    tests/c/view0_v1n1_g03_c0_observation_test.c \
    tests/c/view0_v1n1_g03_c0_lifecycle_test.c \
    tools/view0_v1n1_g03_c0_l1_gate.sh; do
    grep -Fxq "$retained" <<< "$actual" || fail "accepted G03 C0-L1 retained path missing: $retained"
done

for new_path in \
    tools/c/view0_conformance/g03_r1a.c \
    tools/c/view0_conformance/g03_r1a.h \
    tests/c/view0_v1n1_g03_r1a_element_context_test.c \
    tests/c/view0_v1n1_g03_r1a_element_context_adversarial_test.c \
    tests/data/view0_v1n1_g03_r1a_context_coverage.tsv \
    tools/view0_v1n1_g03_r1a_scope_verify.sh \
    tools/view0_v1n1_g03_r1a_contract_verify.sh \
    tools/view0_v1n1_g03_r1a_native_verify.sh \
    tools/view0_v1n1_g03_r1a_gate.sh; do
    grep -Fxq "$new_path" <<< "$actual" || fail "R1A declared new path absent from cumulative candidate: $new_path"
done

echo 'VIEW0_V1N1_G03_R1A_RETAINED_G03_C0_L1_PATH_COUNT=128'
echo 'VIEW0_V1N1_G03_R1A_NEW_PATH_COUNT=9'
echo "VIEW0_V1N1_G03_R1A_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N1_G03_R1A_STAGED_CHANGES=NO'
echo 'PASS: G03 R1A source scope established over exact accepted G03 C0-L1 candidate'
