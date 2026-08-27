#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'
BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
EXPECTED_COUNT='49'
EXPECTED_PATHLIST_SHA='b6380384a5912e2eed2474b17322393958c457ab4a6b29a8499fd13288c6ceb1'
tmp=$(mktemp -d /tmp/arborcore-g04-r2-scope.XXXXXX); trap 'rm -rf "$tmp"' EXIT
[[ "$(git branch --show-current)" == 'view0-v1-completion' ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD drift'
[[ "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'base tree drift'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail 'R2 contract not current'
cat > "$tmp/expected" <<'LIST'
Makefile
docs/VIEW_CORE_VIEW0.md
tests/c/view0_v1n1_g04_r1a_global_failure_atomicity_test.c
tests/c/view0_v1n1_g04_r1a_transparent_parent_model_adversarial_test.c
tests/c/view0_v1n1_g04_r1a_transparent_parent_model_test.c
tests/c/view0_v1n1_g04_r1b_transparent_parent_model_adversarial_test.c
tests/c/view0_v1n1_g04_r1b_transparent_parent_model_test.c
tests/c/view0_v1n1_g04_r1c_noscript_transparent_adversarial_test.c
tests/c/view0_v1n1_g04_r1c_noscript_transparent_test.c
tests/c/view0_v1n1_g04_r2_global_failure_atomicity_test.c
tests/c/view0_v1n1_g04_r2_parentless_flow_adversarial_test.c
tests/c/view0_v1n1_g04_r2_parentless_flow_test.c
tests/data/view0_v1n1_g04_r1a_fixture_plan.tsv
tests/data/view0_v1n1_g04_r1a_source_boundary.tsv
tests/data/view0_v1n1_g04_r1a_transparent_surface.tsv
tests/data/view0_v1n1_g04_r1b_closure_authority.tsv
tests/data/view0_v1n1_g04_r1b_fixture_plan.tsv
tests/data/view0_v1n1_g04_r1b_observer_boundary.tsv
tests/data/view0_v1n1_g04_r1c_fixture_plan.tsv
tests/data/view0_v1n1_g04_r1c_scripting_authority.tsv
tests/data/view0_v1n1_g04_r2_authority.tsv
tests/data/view0_v1n1_g04_r2_fixture_plan.tsv
tests/data/view0_v1n1_g04_r2_fragment_source_boundary.tsv
tests/data/view0_v1n1_g04_r2_parentless_surface.tsv
tools/c/view0_conformance/g04_r1a.c
tools/c/view0_conformance/g04_r1a.h
tools/c/view0_conformance/g04_r2a.c
tools/c/view0_conformance/g04_r2a.h
tools/c/view0_conformance/lexbor_adapter.c
tools/c/view0_conformance/main.c
tools/c/view0_conformance/native.c
tools/include/arborcore/view0_conformance/native.h
tools/view0_v1n1_g04_r1a_contract_verify.sh
tools/view0_v1n1_g04_r1a_gate.sh
tools/view0_v1n1_g04_r1a_native_verify.sh
tools/view0_v1n1_g04_r1a_scope_verify.sh
tools/view0_v1n1_g04_r1b_contract_verify.sh
tools/view0_v1n1_g04_r1b_gate.sh
tools/view0_v1n1_g04_r1b_native_verify.sh
tools/view0_v1n1_g04_r1b_scope_verify.sh
tools/view0_v1n1_g04_r1c_contract_verify.sh
tools/view0_v1n1_g04_r1c_gate.sh
tools/view0_v1n1_g04_r1c_native_verify.sh
tools/view0_v1n1_g04_r1c_scope_verify.sh
tools/view0_v1n1_g04_r2_contract_verify.sh
tools/view0_v1n1_g04_r2_gate.sh
tools/view0_v1n1_g04_r2_native_verify.sh
tools/view0_v1n1_g04_r2_scope_verify.sh
view/arborcore-view-core-1.contract
LIST
{ git diff --name-only; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u > "$tmp/actual"
cmp -s "$tmp/expected" "$tmp/actual" || { diff -u "$tmp/expected" "$tmp/actual" >&2 || true; fail 'G04 R2 cumulative candidate path set drift'; }
actual_count=$(wc -l < "$tmp/actual" | tr -d ' ')
[[ "$actual_count" == "$EXPECTED_COUNT" ]] || fail "G04 R2 path count drift: $actual_count"
actual_sha=$(sha256sum "$tmp/actual" | awk '{print $1}')
[[ "$actual_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail "G04 R2 path-list SHA drift: $actual_sha"
for p in  tests/c/view0_v1n1_g04_r2_parentless_flow_test.c  tests/c/view0_v1n1_g04_r2_parentless_flow_adversarial_test.c  tests/c/view0_v1n1_g04_r2_global_failure_atomicity_test.c  tests/data/view0_v1n1_g04_r2_authority.tsv  tests/data/view0_v1n1_g04_r2_parentless_surface.tsv  tests/data/view0_v1n1_g04_r2_fragment_source_boundary.tsv  tests/data/view0_v1n1_g04_r2_fixture_plan.tsv  tools/c/view0_conformance/g04_r2a.c  tools/c/view0_conformance/g04_r2a.h  tools/view0_v1n1_g04_r2_scope_verify.sh  tools/view0_v1n1_g04_r2_contract_verify.sh  tools/view0_v1n1_g04_r2_native_verify.sh  tools/view0_v1n1_g04_r2_gate.sh; do [[ -e "$p" ]] || fail "R2 path missing: $p"; done
# Retained R1 must remain present, but R2 is the only new G04 group rule surface.
for p in tools/c/view0_conformance/g04_r1a.c tools/c/view0_conformance/g04_r1a.h tools/view0_v1n1_g04_r1c_gate.sh; do [[ -e "$p" ]] || fail "retained R1 path missing: $p"; done
[[ ! -e tools/c/view0_conformance/g05_r1a.c ]] || fail 'G05 implementation appeared before G04 review/freeze'
echo "VIEW0_V1N1_G04_R2_CUMULATIVE_CANDIDATE_PATH_COUNT=$actual_count"
echo "VIEW0_V1N1_G04_R2_CUMULATIVE_CANDIDATE_PATHLIST_SHA256=$actual_sha"
echo 'VIEW0_V1N1_G04_R2_G05_PATHS=ZERO'
echo 'VIEW0_V1N1_G04_R2_STAGED_CHANGES=NO'
echo 'PASS: G04 R2 scope is exactly R1 retained plus the authorized R2 fragment-model construction'
