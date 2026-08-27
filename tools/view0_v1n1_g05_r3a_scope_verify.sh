#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'; BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
EXPECTED_PATH_COUNT='104'; EXPECTED_PATHLIST_SHA256='09a48b86da86eed414c6dfafca4f3a0748411a4ff2ee9a99c00021fa7f222ede'
[[ "$(git branch --show-current)" == view0-v1-completion ]] || fail 'branch'
[[ "$(git rev-parse HEAD)" == "$BASE" && "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'HEAD/base tree'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged changes'
version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1); [[ "$version" == '0.1-VIEW0-V1N1-G05-R3A' || "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]] || fail 'contract version'
t=$(mktemp); trap 'rm -f "$t"' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | sort -u > "$t"
c=$(wc -l < "$t" | tr -d ' '); h=$(sha256sum "$t" | awk '{print $1}')
if [[ "$version" == '0.1-VIEW0-V1N1-G05-R3A' ]]; then [[ "$c" == "$EXPECTED_PATH_COUNT" && "$h" == "$EXPECTED_PATHLIST_SHA256" ]] || fail "scope identity got=$c/$h expected=$EXPECTED_PATH_COUNT/$EXPECTED_PATHLIST_SHA256"
fi
for p in \
  tools/c/view0_conformance/g05_r3a.c tools/c/view0_conformance/g05_r3a.h \
  tests/c/view0_v1n1_g05_r3a_conditional_applicability_test.c \
  tests/c/view0_v1n1_g05_r3a_conditional_applicability_adversarial_test.c \
  tests/c/view0_v1n1_g05_r3a_conditional_matrix_test.c \
  tests/c/view0_v1n1_g05_r3a_global_failure_atomicity_test.c \
  tests/data/view0_v1n1_g05_r3a_rule_authority.tsv \
  tests/data/view0_v1n1_g05_r3a_predicate_implementation.tsv \
  tests/data/view0_v1n1_g05_r3a_input_forbidden_attribute_authority.tsv; do
  [[ -f "$p" ]] || fail "missing R3 path $p"
done
if [[ "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then [[ -f tools/c/view0_conformance/g05_r4a.c ]] || fail 'R4 extension missing'; else [[ ! -e tools/c/view0_conformance/g05_r4a.c ]] || fail 'R4 evaluator appeared'; fi
echo "VIEW0_V1N1_G05_R3A_CANDIDATE_PATH_COUNT=$c"
echo "VIEW0_V1N1_G05_R3A_CANDIDATE_PATHLIST_SHA256=$h"
if [[ "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then echo 'VIEW0_V1N1_G05_R3A_RETAINED_UNDER_R4=YES'; else echo 'VIEW0_V1N1_G05_R3A_R4_PATHS=ZERO'; fi
echo 'VIEW0_V1N1_G05_R3A_STAGED_CHANGES=NO'
echo 'PASS: exact G05 R3A scope established over corrected C0-SR1/R1/R2 predecessor'
