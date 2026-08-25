#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
EXPECTED_COUNT=194
EXPECTED_PATHLIST_SHA='bb0920d5c73a3639948634566cc75d85c85726c52fdb18fbbf91bda51d757a75'
new_paths=$(cat <<'EOF'
tests/c/view0_v1n1_g03_r6a_scalar_value_text_retention_adversarial_test.c
tests/c/view0_v1n1_g03_r6a_scalar_value_text_retention_test.c
tests/data/view0_v1n1_g03_r6a_diagnostic_ownership.tsv
tests/data/view0_v1n1_g03_r6a_fixture_plan.tsv
tests/data/view0_v1n1_g03_r6a_provenance.tsv
tests/data/view0_v1n1_g03_r6a_runtime_retention.tsv
tests/data/view0_v1n1_g03_r6a_support_plan.tsv
tools/view0_v1n1_g03_r6a_contract_verify.sh
tools/view0_v1n1_g03_r6a_gate.sh
tools/view0_v1n1_g03_r6a_native_verify.sh
tools/view0_v1n1_g03_r6a_scope_verify.sh
EOF
)
tmp=$(mktemp -d /tmp/arborcore-r6a-scope.XXXXXX); trap 'rm -rf "$tmp"' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | sort -u > "$tmp/paths"
count=$(wc -l < "$tmp/paths" | tr -d ' ')
[[ "$count" == "$EXPECTED_COUNT" ]] || fail "R6A cumulative path count drift: $count"
path_sha=$(sha256sum "$tmp/paths"|awk '{print $1}')
[[ "$path_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail "R6A path-list drift: $path_sha"
while IFS= read -r p; do [[ -n "$p" ]] || continue; grep -Fxq "$p" "$tmp/paths" || fail "R6A new path missing from candidate: $p"; done <<<"$new_paths"
[[ ! -e tools/c/view0_conformance/g03_r6a.c && ! -e tools/c/view0_conformance/g03_r6a.h ]] || fail 'dedicated R6 evaluator source exists'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'
echo 'VIEW0_V1N1_G03_R6A_PREDECESSOR_PATH_COUNT=183'
echo 'VIEW0_V1N1_G03_R6A_NEW_PATH_COUNT=11'
echo "VIEW0_V1N1_G03_R6A_CUMULATIVE_PATH_COUNT=$count"
echo "VIEW0_V1N1_G03_R6A_CANDIDATE_PATHLIST_SHA256=$path_sha"
echo 'VIEW0_V1N1_G03_R6A_SCOPE_IDENTITY_COLLATION=LC_ALL_C'
echo 'VIEW0_V1N1_G03_R6A_STAGED_CHANGES=NO'
echo 'PASS: G03 R6A scope is contract/verifier/test retention only over accepted R5A'
