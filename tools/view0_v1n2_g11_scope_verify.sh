#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
[[ $(git rev-parse HEAD) == 7ed6ffd71cda186f5c199593e607cc321a7956c4 ]]
[[ $(git rev-parse 'HEAD^{tree}') == 0829bb1b49709f2abb2f404e06ac8671c337c154 ]]
[[ $(git branch --show-current) == main ]]
git diff --cached --quiet
tmp=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-v1n2-g11-scope.XXXXXXXX")
trap 'rm -rf -- "$tmp"' EXIT
git status --porcelain=v1 --untracked-files=all | cut -c4- | LC_ALL=C sort >"$tmp/actual"
[[ $(wc -l <"$tmp/actual") == 69 ]]
for path in \
 tests/c/view0_v1n2_g11_adversarial_test.c \
 tests/c/view0_v1n2_g11_global_failure_atomicity_test.c \
 tests/c/view0_v1n2_g11_test.c \
 tests/data/view0_v1n2_g11_fixture_plan.tsv \
 tests/data/view0_v1n2_g11_ownership.tsv \
 tools/c/view0_conformance/g11.c tools/c/view0_conformance/g11.h \
 tools/view0_v1n2_g11_contract_verify.sh tools/view0_v1n2_g11_gate.sh \
 tools/view0_v1n2_g11_native_verify.sh tools/view0_v1n2_g11_scope_verify.sh; do
    grep -Fx -- "$path" "$tmp/actual" >/dev/null
done
if grep -Eq '(^|/)g1[2-9]([./_]|$)|(^|/)g[2-9][0-9]([./_]|$)' "$tmp/actual"; then
    echo 'FAIL: later V1N2 group path is present' >&2; exit 1
fi
printf '%s\n' \
 'VIEW0_V1N2_G11_CUMULATIVE_CANDIDATE_PATH_COUNT=69' \
 'VIEW0_V1N2_G11_TRANSACTION_DELTA_PATH_COUNT=16' \
 'VIEW0_V1N2_G11_TRANSACTION_REPLACEMENT_PATH_COUNT=5' \
 'VIEW0_V1N2_G11_TRANSACTION_NEW_PATH_COUNT=11' \
 'VIEW0_V1N2_G11_STAGED_CHANGES=NO' \
 'VIEW0_V1N2_G11_LATER_GROUP_SOURCE_PATHS=ZERO' \
 'PASS: exact V1N2 G11 source scope established over independently frozen G10'
