#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'
BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
EXPECTED_COUNT='127'
EXPECTED_PATHLIST_SHA='6795733c42fc7c2073aaf09efe8a94e1ff02e4ba6ef1f2a2794f8593c977e749'
[[ "$(git branch --show-current)" == 'view0-v1-completion' ]] || fail 'branch divergence'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD divergence'
[[ "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'base tree divergence'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged changes exist'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == '0.1-VIEW0-V1N1-G06-C0' ]] || fail 'G06 C0 contract not current'
tmp=$(mktemp -d /tmp/arborcore-g06-c0-scope.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | sort -u > "$tmp/actual"
actual_count=$(wc -l < "$tmp/actual" | tr -d ' ')
actual_sha=$(sha256sum "$tmp/actual" | awk '{print $1}')
[[ "$actual_count" == "$EXPECTED_COUNT" ]] || fail "candidate path count drift: $actual_count"
[[ "$actual_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail "candidate pathlist SHA drift: $actual_sha"
cat > "$tmp/g06-expected" <<'EOF'
tests/c/view0_v1n1_g06_c0_validator_test.c
tests/data/view0_v1n1_g06_c0_consumer_policy.tsv
tests/data/view0_v1n1_g06_c0_foundation_summary.txt
tests/data/view0_v1n1_g06_c0_link_as_policy.tsv
tests/data/view0_v1n1_g06_c0_rule_authority.tsv
tools/c/view0_conformance/g06_c0.c
tools/c/view0_conformance/g06_c0.h
tools/view0_v1n1_g06_c0_contract_verify.sh
tools/view0_v1n1_g06_c0_gate.sh
tools/view0_v1n1_g06_c0_native_verify.sh
tools/view0_v1n1_g06_c0_scope_verify.sh
EOF
rg 'g06' "$tmp/actual" > "$tmp/g06-actual" || true
cmp -s "$tmp/g06-expected" "$tmp/g06-actual" || fail 'G06 C0 path surface drift'
echo "VIEW0_V1N1_G06_C0_CANDIDATE_PATH_COUNT=$actual_count"
echo "VIEW0_V1N1_G06_C0_CANDIDATE_PATHLIST_SHA256=$actual_sha"
echo 'VIEW0_V1N1_G06_C0_TRANSACTION_DELTA_PATH_COUNT=14'
echo 'VIEW0_V1N1_G06_C0_TRANSACTION_REPLACEMENT_PATH_COUNT=3'
echo 'VIEW0_V1N1_G06_C0_TRANSACTION_NEW_PATH_COUNT=11'
echo 'VIEW0_V1N1_G06_C0_STAGED_CHANGES=NO'
echo 'PASS: exact G06 C0 zero-diagnostic foundation scope established'
