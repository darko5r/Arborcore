#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE='08b55954139a2e8daaf254f4935c8f7a31e3aa19'
TREE='eea1bab55dd90ab7f89bce5622db800a4f60e282'
PREDECESSOR_COUNT=174
PREDECESSOR_MANIFEST='97ebdbe068f058aa4ca1a3b6395135d8d33daa87b443952246a3b5c7c5b224fa'
EXPECTED_COUNT=183
EXPECTED_PATHLIST_SHA='fe8f0672b9ceaec58d8516bfe0d9ef8dcd8b86dce55d0224f0ad82d6b4305942'
fail(){ echo "FAIL: $*" >&2; exit 1; }
[[ "$(git branch --show-current)" == view0-presentation-core ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" && "$(git rev-parse HEAD^{tree})" == "$TREE" ]] || fail 'frozen HTTP1 base changed'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'
new_paths=$(cat <<'PATHS'
tests/c/view0_v1n1_g03_r5a_explicit_html_element_allowance_adversarial_test.c
tests/c/view0_v1n1_g03_r5a_explicit_html_element_allowance_test.c
tests/data/view0_v1n1_g03_r5a_repair_class_support.tsv
tools/c/view0_conformance/g03_r5a.c
tools/c/view0_conformance/g03_r5a.h
tools/view0_v1n1_g03_r5a_contract_verify.sh
tools/view0_v1n1_g03_r5a_gate.sh
tools/view0_v1n1_g03_r5a_native_verify.sh
tools/view0_v1n1_g03_r5a_scope_verify.sh
PATHS
)
while IFS= read -r p; do [[ -f "$p" && ! -L "$p" ]] || fail "R5A new path missing/non-regular: $p"; done <<< "$new_paths"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
{ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u > "$tmp/paths"
count=$(wc -l < "$tmp/paths" | tr -d ' ')
[[ "$count" -eq "$EXPECTED_COUNT" ]] || fail "R5A cumulative path count drift: $count expected=$EXPECTED_COUNT"
need_path_sha=$(sha256sum "$tmp/paths" | awk '{print $1}')
[[ "$need_path_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail "R5A candidate path-list drift: got=$need_path_sha expected=$EXPECTED_PATHLIST_SHA"
while IFS= read -r p; do grep -Fxq "$p" "$tmp/paths" || fail "declared R5A new path absent: $p"; done <<< "$new_paths"
for retained in \
  tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c tools/c/view0_conformance/g03_r4a.c \
  tests/data/view0_v1n1_g03_c0_sr1_record_layout.tsv \
  tests/data/view0_v1n1_g03_r4a_subject_support.tsv; do
  grep -Fxq "$retained" "$tmp/paths" || fail "retained predecessor path missing: $retained"
done
echo "VIEW0_V1N1_G03_R5A_PREDECESSOR_PATH_COUNT=$PREDECESSOR_COUNT"
echo "VIEW0_V1N1_G03_R5A_PREDECESSOR_MANIFEST_SHA256=$PREDECESSOR_MANIFEST"
echo 'VIEW0_V1N1_G03_R5A_NEW_PATH_COUNT=9'
echo "VIEW0_V1N1_G03_R5A_CUMULATIVE_PATH_COUNT=$count"
echo "VIEW0_V1N1_G03_R5A_CANDIDATE_PATHLIST_SHA256=$need_path_sha"
echo 'VIEW0_V1N1_G03_R5A_STAGED_CHANGES=NO'
echo 'PASS: G03 R5A source scope established over exact accepted 174-path R4A+C0-SR1 predecessor'
