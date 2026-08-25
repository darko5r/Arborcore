#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE_HEAD='08b55954139a2e8daaf254f4935c8f7a31e3aa19'
[[ "$(git rev-parse HEAD)" == "$BASE_HEAD" ]] || fail 'HEAD drift'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'
paths=$(mktemp); trap 'rm -f "$paths"' EXIT
{ git diff --name-only "$BASE_HEAD"; git ls-files --others --exclude-standard; } | sort -u > "$paths"
[[ "$(wc -l < "$paths" | tr -d ' ')" -eq 174 ]] || fail "C0-SR1 cumulative path count !=174"
for p in \
 tests/data/view0_v1n1_g03_c0_sr1_insertion_mode_enum.tsv \
 tests/data/view0_v1n1_g03_c0_sr1_record_layout.tsv \
 tests/data/view0_v1n1_g03_c0_sr1_delivery_semantics.tsv \
 tests/data/view0_v1n1_g03_c0_sr1_qualification_controls.tsv \
 tests/c/view0_v1n1_g03_c0_sr1_source_repair_test.c \
 tests/c/view0_v1n1_g03_c0_sr1_source_repair_adversarial_test.c \
 tools/view0_v1n1_g03_c0_sr1_scope_verify.sh \
 tools/view0_v1n1_g03_c0_sr1_contract_verify.sh \
 tools/view0_v1n1_g03_c0_sr1_native_verify.sh \
 tools/view0_v1n1_g03_c0_sr1_gate.sh; do
    grep -Fxq "$p" "$paths" || fail "C0-SR1 new path missing: $p"
done
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)" == '0.1-VIEW0-V1N1-G03-R4A' ]] || fail 'active top contract changed during C0-SR1'
! grep -Rqs 'ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE\|0x0000000030030005' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R5 implementation leaked into C0-SR1'
echo 'VIEW0_V1N1_G03_C0_SR1_RETAINED_R4A_PATH_COUNT=164'
echo 'VIEW0_V1N1_G03_C0_SR1_NEW_PATH_COUNT=10'
echo 'VIEW0_V1N1_G03_C0_SR1_CUMULATIVE_PATH_COUNT=174'
echo 'VIEW0_V1N1_G03_C0_SR1_ACTIVE_TOP_CONTRACT=0.1-VIEW0-V1N1-G03-R4A'
echo 'PASS: C0-SR1 source scope established over accepted R4A without R5 rule construction'
