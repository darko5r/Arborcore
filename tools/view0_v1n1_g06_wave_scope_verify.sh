#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'
BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
EXPECTED_COUNT='137'
EXPECTED_PATHLIST_SHA='cd9e0991a4f8f20165371285902a252c9c2af9120456e5cdb952ddec9a0aa81b'
[[ "$(git branch --show-current)" == 'view0-v1-completion' ]] || fail 'branch divergence'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD divergence'
[[ "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'base tree divergence'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged changes exist'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == \
    '0.1-VIEW0-V1N1-G06-R17A-SR1' ]] || fail 'G06 R17A-SR1 contract not current'
tmp=$(mktemp -d /tmp/arborcore-g06-wave-scope.XXXXXX)
trap 'find "$tmp" -type f -delete; find "$tmp" -depth -type d -empty -delete' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | sort -u > "$tmp/actual"
actual_count=$(wc -l < "$tmp/actual" | tr -d ' ')
actual_sha=$(sha256sum "$tmp/actual" | awk '{print $1}')
[[ "$actual_count" == "$EXPECTED_COUNT" ]] || fail "candidate path count drift: $actual_count"
[[ "$actual_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail "candidate pathlist SHA drift: $actual_sha"
printf '%s\n' \
  tests/c/view0_v1n1_g06_c0_validator_test.c \
  tests/c/view0_v1n1_g06_wave_adversarial_test.c \
  tests/c/view0_v1n1_g06_wave_global_failure_atomicity_test.c \
  tests/c/view0_v1n1_g06_wave_test.c \
  tests/data/view0_v1n1_g06_c0_consumer_policy.tsv \
  tests/data/view0_v1n1_g06_c0_foundation_summary.txt \
  tests/data/view0_v1n1_g06_c0_link_as_policy.tsv \
  tests/data/view0_v1n1_g06_c0_rule_authority.tsv \
  tests/data/view0_v1n1_g06_wave_ownership.txt \
  tools/c/view0_conformance/g06.c \
  tools/c/view0_conformance/g06.h \
  tools/c/view0_conformance/g06_c0.c \
  tools/c/view0_conformance/g06_c0.h \
  tools/view0_v1n1_g06_c0_contract_verify.sh \
  tools/view0_v1n1_g06_c0_gate.sh \
  tools/view0_v1n1_g06_c0_native_verify.sh \
  tools/view0_v1n1_g06_c0_scope_verify.sh \
  tools/view0_v1n1_g06_wave_contract_verify.sh \
  tools/view0_v1n1_g06_wave_gate.sh \
  tools/view0_v1n1_g06_wave_native_verify.sh \
  tools/view0_v1n1_g06_wave_scope_verify.sh > "$tmp/g06-expected"
rg 'g06' "$tmp/actual" > "$tmp/g06-actual" || true
cmp -s "$tmp/g06-expected" "$tmp/g06-actual" || fail 'G06 cumulative path surface drift'
echo "VIEW0_V1N1_G06_WAVE_CANDIDATE_PATH_COUNT=$actual_count"
echo "VIEW0_V1N1_G06_WAVE_CANDIDATE_PATHLIST_SHA256=$actual_sha"
echo 'VIEW0_V1N1_G06_WAVE_TRANSACTION_DELTA_PATH_COUNT=15'
echo 'VIEW0_V1N1_G06_WAVE_TRANSACTION_REPLACEMENT_PATH_COUNT=5'
echo 'VIEW0_V1N1_G06_WAVE_TRANSACTION_NEW_PATH_COUNT=10'
echo 'VIEW0_V1N1_G06_WAVE_STAGED_CHANGES=NO'
echo 'PASS: exact G06 R1-R17 cumulative source scope established over G06 C0'
