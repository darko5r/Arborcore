#!/usr/bin/env bash
set -euo pipefail

ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
[[ $(git rev-parse HEAD) == 7ed6ffd71cda186f5c199593e607cc321a7956c4 ]]
[[ $(git rev-parse 'HEAD^{tree}') == 0829bb1b49709f2abb2f404e06ac8671c337c154 ]]
[[ $(git branch --show-current) == main ]]
git diff --cached --quiet

tmp=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-v1n2-g08-scope.XXXXXXXX")
trap 'rm -rf -- "$tmp"' EXIT
git status --porcelain=v1 --untracked-files=all | cut -c4- | LC_ALL=C sort >"$tmp/actual.txt"

cat >"$tmp/g08-new.txt" <<'PATHS'
tests/c/view0_v1n2_g08_adversarial_test.c
tests/c/view0_v1n2_g08_global_failure_atomicity_test.c
tests/c/view0_v1n2_g08_test.c
tests/data/view0_v1n2_g08_fixture_plan.tsv
tests/data/view0_v1n2_g08_ownership.tsv
tools/c/view0_conformance/g08.c
tools/c/view0_conformance/g08.h
tools/view0_v1n2_g08_contract_verify.sh
tools/view0_v1n2_g08_gate.sh
tools/view0_v1n2_g08_native_verify.sh
tools/view0_v1n2_g08_scope_verify.sh
PATHS
LC_ALL=C sort -o "$tmp/g08-new.txt" "$tmp/g08-new.txt"

cat >"$tmp/g07-predecessor.txt" <<'PATHS'
Makefile
docs/VIEW_CORE_VIEW0.md
tests/c/view0_v1n2_c0_foundation_test.c
tests/c/view0_v1n2_g07_adversarial_test.c
tests/c/view0_v1n2_g07_global_failure_atomicity_test.c
tests/c/view0_v1n2_g07_test.c
tests/data/view0_v1n2_c0_external_authority.tsv
tests/data/view0_v1n2_c0_rule_authority.tsv
tests/data/view0_v1n2_g07_fixture_plan.tsv
tests/data/view0_v1n2_g07_ownership.tsv
tools/c/view0_conformance/g07.c
tools/c/view0_conformance/g07.h
tools/c/view0_conformance/native.c
tools/c/view0_conformance/v1n2_c0.c
tools/c/view0_conformance/v1n2_c0.h
tools/include/arborcore/view0_conformance/native.h
tools/view0_v1n2_c0_contract_verify.sh
tools/view0_v1n2_c0_gate.sh
tools/view0_v1n2_c0_native_verify.sh
tools/view0_v1n2_c0_scope_verify.sh
tools/view0_v1n2_g07_contract_verify.sh
tools/view0_v1n2_g07_gate.sh
tools/view0_v1n2_g07_native_verify.sh
tools/view0_v1n2_g07_scope_verify.sh
view/arborcore-view-core-1.contract
PATHS
LC_ALL=C sort -o "$tmp/g07-predecessor.txt" "$tmp/g07-predecessor.txt"
LC_ALL=C sort -u "$tmp/g07-predecessor.txt" "$tmp/g08-new.txt" >"$tmp/expected.txt"
diff -u "$tmp/expected.txt" "$tmp/actual.txt"
[[ $(wc -l <"$tmp/actual.txt") == 36 ]]
[[ $(wc -l <"$tmp/g08-new.txt") == 11 ]]
if grep -Eq '(^|/)g0(9)([./_]|$)|(^|/)g1(0|1)([./_]|$)' "$tmp/actual.txt"; then
    echo 'FAIL: later V1N2 group path is present' >&2
    exit 1
fi
printf '%s\n' \
  'VIEW0_V1N2_G08_CUMULATIVE_CANDIDATE_PATH_COUNT=36' \
  'VIEW0_V1N2_G08_TRANSACTION_DELTA_PATH_COUNT=16' \
  'VIEW0_V1N2_G08_TRANSACTION_REPLACEMENT_PATH_COUNT=5' \
  'VIEW0_V1N2_G08_TRANSACTION_NEW_PATH_COUNT=11' \
  'VIEW0_V1N2_G08_STAGED_CHANGES=NO' \
  'VIEW0_V1N2_G08_LATER_GROUP_SOURCE_PATHS=ZERO' \
  'PASS: exact V1N2 G08 source scope established over accepted G07'
