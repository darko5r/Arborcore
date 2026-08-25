#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
EXPECTED_COUNT=209
EXPECTED_PATHLIST_SHA='72a24bcdc8848c5c87ad85036b76c50c3390614851c24583ba0f265dde4fadc7'
new_paths=$(cat <<'LIST'
tests/c/view0_v1n1_g03_r7a_palpable_content_adversarial_test.c
tests/c/view0_v1n1_g03_r7a_global_failure_atomicity_test.c
tests/c/view0_v1n1_g03_r7a_palpable_content_test.c
tests/data/view0_v1n1_g03_r7a_diagnostic_plan.tsv
tests/data/view0_v1n1_g03_r7a_fixture_plan.tsv
tests/data/view0_v1n1_g03_r7a_palpable_child_support_plan.tsv
tests/data/view0_v1n1_g03_r7a_retained_fixture_isolation.tsv
tests/data/view0_v1n1_g03_r7a_source_boundary.tsv
tests/data/view0_v1n1_g03_r7a_subject_support_plan.tsv
tools/c/view0_conformance/g03_r7a.c
tools/c/view0_conformance/g03_r7a.h
tools/view0_v1n1_g03_r7a_contract_verify.sh
tools/view0_v1n1_g03_r7a_gate.sh
tools/view0_v1n1_g03_r7a_native_verify.sh
tools/view0_v1n1_g03_r7a_scope_verify.sh
LIST
)
tmp=$(mktemp -d /tmp/arborcore-r7a-scope.XXXXXX); trap 'rm -rf "$tmp"' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | sort -u > "$tmp/paths"
count=$(wc -l < "$tmp/paths" | tr -d ' ')
[[ "$count" == "$EXPECTED_COUNT" ]] || fail "R7A cumulative path count drift: $count"
path_sha=$(sha256sum "$tmp/paths"|awk '{print $1}')
[[ "$path_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail "R7A path-list drift: $path_sha"
while IFS= read -r p; do [[ -n "$p" ]] || continue; grep -Fxq "$p" "$tmp/paths" || fail "R7A new path missing from candidate: $p"; done <<<"$new_paths"
[[ -f tools/c/view0_conformance/g03_r7a.c && -f tools/c/view0_conformance/g03_r7a.h ]] || fail 'R7A evaluator source missing'
[[ ! -e tools/c/view0_conformance/g03_r6a.c && ! -e tools/c/view0_conformance/g03_r6a.h ]] || fail 'dedicated R6 evaluator source appeared'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'
echo 'VIEW0_V1N1_G03_R7A_PREDECESSOR_PATH_COUNT=194'
echo 'VIEW0_V1N1_G03_R7A_NEW_PATH_COUNT=14'
echo 'VIEW0_V1N1_G03_R7A_SR2_PREDECESSOR_PATH_COUNT=208'
echo 'VIEW0_V1N1_G03_R7A_SR2_NEW_PATH_COUNT=1'
echo "VIEW0_V1N1_G03_R7A_SR2_CUMULATIVE_PATH_COUNT=$count"
echo "VIEW0_V1N1_G03_R7A_SR2_CANDIDATE_PATHLIST_SHA256=$path_sha"
echo 'VIEW0_V1N1_G03_R7A_SCOPE_IDENTITY_COLLATION=LC_ALL_C'
echo 'VIEW0_V1N1_G03_R7A_STAGED_CHANGES=NO'
echo 'PASS: G03 R7A SR2 scope is the failure-atomicity correction over the exact 208-path R7A candidate'
