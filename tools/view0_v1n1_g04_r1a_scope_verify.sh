#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'
BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
EXPECTED_COUNT=18
EXPECTED_PATHLIST_SHA='f5a60b1b8a12f87ee89db09eb854c7708dfd0a236caf0fb2c30b2fbc0be180d7'
tmp=$(mktemp -d /tmp/arborcore-g04-r1a-scope.XXXXXX); trap 'rm -rf "$tmp"' EXIT
[[ "$(git branch --show-current)" == 'view0-v1-completion' ]] || fail 'wrong branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD drift'
[[ "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'base tree drift'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged paths exist'
cat > "$tmp/expected" <<'LIST'
Makefile
docs/VIEW_CORE_VIEW0.md
tests/c/view0_v1n1_g04_r1a_global_failure_atomicity_test.c
tests/c/view0_v1n1_g04_r1a_transparent_parent_model_adversarial_test.c
tests/c/view0_v1n1_g04_r1a_transparent_parent_model_test.c
tests/data/view0_v1n1_g04_r1a_fixture_plan.tsv
tests/data/view0_v1n1_g04_r1a_source_boundary.tsv
tests/data/view0_v1n1_g04_r1a_transparent_surface.tsv
tools/c/view0_conformance/g04_r1a.c
tools/c/view0_conformance/g04_r1a.h
tools/c/view0_conformance/main.c
tools/c/view0_conformance/native.c
tools/include/arborcore/view0_conformance/native.h
tools/view0_v1n1_g04_r1a_contract_verify.sh
tools/view0_v1n1_g04_r1a_gate.sh
tools/view0_v1n1_g04_r1a_native_verify.sh
tools/view0_v1n1_g04_r1a_scope_verify.sh
view/arborcore-view-core-1.contract
LIST
version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)
if [[ "$version" == '0.1-VIEW0-V1N1-G04-R1A' ]]; then
  { git diff --name-only; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u > "$tmp/actual"
  cmp -s "$tmp/expected" "$tmp/actual" || { diff -u "$tmp/expected" "$tmp/actual" >&2 || true; fail 'G04 R1A candidate path set drift'; }
  count=$(wc -l < "$tmp/actual" | tr -d ' ')
  [[ "$count" == "$EXPECTED_COUNT" ]] || fail "G04 R1A path count drift: $count"
  path_sha=$(sha256sum "$tmp/actual" | awk '{print $1}')
  [[ "$path_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail "G04 R1A path-list SHA drift: $path_sha"
  echo "VIEW0_V1N1_G04_R1A_CANDIDATE_PATH_COUNT=$count"
  echo "VIEW0_V1N1_G04_R1A_CANDIDATE_PATHLIST_SHA256=$path_sha"
elif [[ "$version" == '0.1-VIEW0-V1N1-G04-R1B' || "$version" == '0.1-VIEW0-V1N1-G04-R1C' || "$version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then
  # R1B/R1C legitimately replace some R1A bytes and add paths; require every original
  # R1A path to remain present in the cumulative candidate instead of pinning R1A bytes.
  while IFS= read -r path; do [[ -e "$path" ]] || fail "R1A retained path missing under extension: $path"; done < "$tmp/expected"
  echo "VIEW0_V1N1_G04_R1A_SCOPE_EXTENSION_AWARE=${version#0.1-VIEW0-V1N1-G04-}_RETAINED_PATHS_PRESENT"
else
  fail "unsupported current contract for R1A verifier: $version"
fi
[[ -f tools/c/view0_conformance/g04_r1a.c && -f tools/c/view0_conformance/g04_r1a.h ]] || fail 'G04 R1A evaluator source missing'
if [[ "$version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then
  [[ -f tools/c/view0_conformance/g04_r2a.c && -f tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 extension missing under R2 contract'
else
  [[ ! -e tools/c/view0_conformance/g04_r2a.c && ! -e tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 implementation appeared in pre-R2 R1A scope'
fi
echo 'VIEW0_V1N1_G04_R1A_STAGED_CHANGES=NO'
echo 'PASS: G04 R1A scope retained exactly or by reviewed R1B/R1C/R2 extension'
