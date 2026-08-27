#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'
BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
EXPECTED_PATH_COUNT='116'
EXPECTED_PATHLIST_SHA256='f2155d6b700d84f314ddee64b94d0bd7cc84c1d2b4f904dfcb56e3c360f1e053'
[[ "$(git branch --show-current)" == view0-v1-completion ]] || fail 'branch'
[[ "$(git rev-parse HEAD)" == "$BASE" && "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'HEAD/base tree'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged changes'
grep -Fxq 'ARBORCORE_VIEW_CORE_VERSION=0.1-VIEW0-V1N1-G05-R4A' view/arborcore-view-core-1.contract || fail 'contract version'
t=$(mktemp); trap 'rm -f "$t"' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u > "$t"
c=$(wc -l < "$t" | tr -d ' '); h=$(sha256sum "$t" | awk '{print $1}')
[[ "$c" == "$EXPECTED_PATH_COUNT" && "$h" == "$EXPECTED_PATHLIST_SHA256" ]] || fail "scope identity got=$c/$h expected=$EXPECTED_PATH_COUNT/$EXPECTED_PATHLIST_SHA256"
for p in \
  tools/c/view0_conformance/g05_r4a.c tools/c/view0_conformance/g05_r4a.h \
  tests/c/view0_v1n1_g05_r4a_body_window_event_test.c \
  tests/c/view0_v1n1_g05_r4a_body_window_event_adversarial_test.c \
  tests/c/view0_v1n1_g05_r4a_global_failure_atomicity_test.c \
  tests/data/view0_v1n1_g05_r4a_rule_authority.tsv \
  tests/data/view0_v1n1_g05_r4a_ownership.tsv \
  tests/data/view0_v1n1_g05_r4a_fixture_plan.tsv; do
  [[ -f "$p" ]] || fail "missing R4 path $p"
done
[[ ! -e tools/c/view0_conformance/g06_r1a.c ]] || fail 'G06 evaluator appeared'
echo "VIEW0_V1N1_G05_R4A_CANDIDATE_PATH_COUNT=$c"
echo "VIEW0_V1N1_G05_R4A_CANDIDATE_PATHLIST_SHA256=$h"
echo 'VIEW0_V1N1_G05_R4A_G06_PATHS=ZERO'
echo 'VIEW0_V1N1_G05_R4A_STAGED_CHANGES=NO'
echo 'PASS: exact G05 R4A scope established over accepted R3A predecessor'
