#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'
BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
EXPECTED_COUNT='36'
EXPECTED_PATHLIST_SHA='5f38e5e4058b6d00626194ca72395fc1ea3909910b36f3f25121672d35ba855e'
tmp=$(mktemp -d /tmp/arborcore-g04-r1c-scope.XXXXXX); trap 'rm -rf "$tmp"' EXIT
[[ "$(git branch --show-current)" == 'view0-v1-completion' ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD drift'
[[ "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'base tree drift'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'
version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)"
[[ "$version" == '0.1-VIEW0-V1N1-G04-R1C' || "$version" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail "unsupported current contract for R1C verifier: $version"
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
tests/data/view0_v1n1_g04_r1a_fixture_plan.tsv
tests/data/view0_v1n1_g04_r1a_source_boundary.tsv
tests/data/view0_v1n1_g04_r1a_transparent_surface.tsv
tests/data/view0_v1n1_g04_r1b_closure_authority.tsv
tests/data/view0_v1n1_g04_r1b_fixture_plan.tsv
tests/data/view0_v1n1_g04_r1b_observer_boundary.tsv
tests/data/view0_v1n1_g04_r1c_fixture_plan.tsv
tests/data/view0_v1n1_g04_r1c_scripting_authority.tsv
tools/c/view0_conformance/g04_r1a.c
tools/c/view0_conformance/g04_r1a.h
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
view/arborcore-view-core-1.contract
LIST
if [[ "$version" == '0.1-VIEW0-V1N1-G04-R1C' ]]; then
  { git diff --name-only; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u > "$tmp/actual"
  cmp -s "$tmp/expected" "$tmp/actual" || { diff -u "$tmp/expected" "$tmp/actual" >&2 || true; fail 'G04 R1C cumulative candidate path set drift'; }
  count=$(wc -l < "$tmp/actual" | tr -d ' ')
  [[ "$count" == "$EXPECTED_COUNT" ]] || fail "G04 R1C path count drift: $count"
  path_sha=$(sha256sum "$tmp/actual" | awk '{print $1}')
  [[ "$path_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail "G04 R1C path-list SHA drift: $path_sha"
  echo "VIEW0_V1N1_G04_R1C_CUMULATIVE_CANDIDATE_PATH_COUNT=$count"
  echo "VIEW0_V1N1_G04_R1C_CUMULATIVE_CANDIDATE_PATHLIST_SHA256=$path_sha"
  echo 'VIEW0_V1N1_G04_R1C_G04_R2_PATHS=ZERO'
else
  while IFS= read -r path; do [[ -e "$path" ]] || fail "R1C retained path missing under R2: $path"; done < "$tmp/expected"
  [[ -f tools/c/view0_conformance/g04_r2a.c && -f tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 extension missing under R2 contract'
  echo 'VIEW0_V1N1_G04_R1C_SCOPE_EXTENSION_AWARE=R2_RETAINED_PATHS_PRESENT'
fi
for p in \
 tests/c/view0_v1n1_g04_r1c_noscript_transparent_test.c \
 tests/c/view0_v1n1_g04_r1c_noscript_transparent_adversarial_test.c \
 tests/data/view0_v1n1_g04_r1c_scripting_authority.tsv \
 tests/data/view0_v1n1_g04_r1c_fixture_plan.tsv \
 tools/view0_v1n1_g04_r1c_scope_verify.sh \
 tools/view0_v1n1_g04_r1c_contract_verify.sh \
 tools/view0_v1n1_g04_r1c_native_verify.sh \
 tools/view0_v1n1_g04_r1c_gate.sh; do [[ -e "$p" ]] || fail "R1C path missing: $p"; done
echo 'VIEW0_V1N1_G04_R1C_STAGED_CHANGES=NO'
echo 'PASS: G04 R1C scope is R1C scope retained exactly or by reviewed R2 extension'
