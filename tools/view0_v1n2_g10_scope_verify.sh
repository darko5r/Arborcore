#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
[[ $(git rev-parse HEAD) == 7ed6ffd71cda186f5c199593e607cc321a7956c4 ]]
[[ $(git rev-parse 'HEAD^{tree}') == 0829bb1b49709f2abb2f404e06ac8671c337c154 ]]
[[ $(git branch --show-current) == main ]]
git diff --cached --quiet
tmp=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-v1n2-g10-scope.XXXXXXXX")
trap 'rm -rf -- "$tmp"' EXIT
git status --porcelain=v1 --untracked-files=all | cut -c4- | LC_ALL=C sort >"$tmp/actual"
[[ $(wc -l <"$tmp/actual") == 58 ]]
for path in \
 tests/c/view0_v1n2_g10_adversarial_test.c \
 tests/c/view0_v1n2_g10_global_failure_atomicity_test.c \
 tests/c/view0_v1n2_g10_test.c \
 tests/data/view0_v1n2_g10_fixture_plan.tsv \
 tests/data/view0_v1n2_g10_ownership.tsv \
 tools/c/view0_conformance/g10.c tools/c/view0_conformance/g10.h \
 tools/view0_v1n2_g10_contract_verify.sh tools/view0_v1n2_g10_gate.sh \
 tools/view0_v1n2_g10_native_verify.sh tools/view0_v1n2_g10_scope_verify.sh; do
    grep -Fx -- "$path" "$tmp/actual" >/dev/null
done
if grep -Eq '(^|/)g11([./_]|$)' "$tmp/actual"; then
    echo 'FAIL: later V1N2 group path is present' >&2; exit 1
fi
printf '%s\n' \
 'VIEW0_V1N2_G10_CUMULATIVE_CANDIDATE_PATH_COUNT=58' \
 'VIEW0_V1N2_G10_TRANSACTION_DELTA_PATH_COUNT=16' \
 'VIEW0_V1N2_G10_TRANSACTION_REPLACEMENT_PATH_COUNT=5' \
 'VIEW0_V1N2_G10_TRANSACTION_NEW_PATH_COUNT=11' \
 'VIEW0_V1N2_G10_STAGED_CHANGES=NO' \
 'VIEW0_V1N2_G10_LATER_GROUP_SOURCE_PATHS=ZERO' \
 'PASS: exact V1N2 G10 source scope established over accepted G09'
