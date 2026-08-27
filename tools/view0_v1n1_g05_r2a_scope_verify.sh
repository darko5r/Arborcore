#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'; BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'; EXPECTED_COUNT=83; EXPECTED_SHA='d917072a9aef2e9a0c9a8c70491679135ce64ddab7c8aa097cf51a8cd19fe801'
[[ "$(git branch --show-current)" == view0-v1-completion ]] || fail 'branch'; [[ "$(git rev-parse HEAD)" == "$BASE" && "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'HEAD'; [[ -z "$(git diff --cached --name-only)" ]] || fail 'staged'
version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract|head -1); [[ "$version" == '0.1-VIEW0-V1N1-G05-R2A' || "$version" == '0.1-VIEW0-V1N1-G05-R2A-SR1' || "$version" == '0.1-VIEW0-V1N1-G05-R3A' || "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]] || fail 'contract'
t=$(mktemp); trap 'rm -f "$t"' EXIT; { git diff --name-only; git ls-files --others --exclude-standard; }|sort -u >"$t"; c=$(wc -l<"$t"|tr -d ' '); h=$(sha256sum "$t"|awk '{print $1}'); if [[ "$version" == '0.1-VIEW0-V1N1-G05-R2A' ]]; then
  [[ "$c" == "$EXPECTED_COUNT" && "$h" == "$EXPECTED_SHA" ]] || fail "scope $c/$h"
else
  [[ -f tests/data/view0_v1n1_g05_c0_sr1_input_state_authority.tsv ]] || fail 'SR1 authority path missing'
fi
[[ -f tools/c/view0_conformance/g05_r2a.c && -f tests/c/view0_v1n1_g05_r2a_element_attribute_test.c ]] || fail 'R2 paths'
if [[ "$version" == '0.1-VIEW0-V1N1-G05-R3A' ]]; then
  [[ -f tools/c/view0_conformance/g05_r3a.c && ! -e tools/c/view0_conformance/g05_r4a.c ]] || fail 'R3 extension/R4 boundary'
elif [[ "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then
  [[ -f tools/c/view0_conformance/g05_r3a.c && -f tools/c/view0_conformance/g05_r4a.c ]] || fail 'R3/R4 extension paths missing'
else
  [[ ! -e tools/c/view0_conformance/g05_r3a.c && ! -e tools/c/view0_conformance/g05_r4a.c ]] || fail 'later evaluators'
fi
echo "VIEW0_V1N1_G05_R2A_CANDIDATE_PATH_COUNT=$c"; echo "VIEW0_V1N1_G05_R2A_CANDIDATE_PATHLIST_SHA256=$h"; if [[ "$version" == '0.1-VIEW0-V1N1-G05-R3A' ]]; then echo 'VIEW0_V1N1_G05_R2A_RETAINED_UNDER_R3=YES'; elif [[ "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then echo 'VIEW0_V1N1_G05_R2A_RETAINED_UNDER_R4=YES'; else echo 'VIEW0_V1N1_G05_R2A_R3_R4_PATHS=ZERO'; fi; echo 'VIEW0_V1N1_G05_R2A_STAGED_CHANGES=NO'; echo 'PASS: exact G05 R2A scope established'
