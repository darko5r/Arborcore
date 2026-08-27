#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'; BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
[[ "$(git branch --show-current)" == 'view0-v1-completion' ]] || fail 'branch'
[[ "$(git rev-parse HEAD)" == "$BASE" && "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'HEAD/base tree'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged changes'
version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract|head -1)
case "$version" in
  0.1-VIEW0-V1N1-G05-R1A) expected_count=73; expected_sha='d0b3497aa299928b5d3d75f28b049e6ec9dc14f39fc8a2b6c0437243d91bca07'; extension=NO ;;
  0.1-VIEW0-V1N1-G05-R2A) expected_count=83; expected_sha='d917072a9aef2e9a0c9a8c70491679135ce64ddab7c8aa097cf51a8cd19fe801'; extension=R2 ;;
  0.1-VIEW0-V1N1-G05-R2A-SR1) expected_count=0; expected_sha=''; extension=R2A_SR1 ;;
  0.1-VIEW0-V1N1-G05-R3A) expected_count=0; expected_sha=''; extension=R3 ;;
  0.1-VIEW0-V1N1-G05-R4A) expected_count=0; expected_sha=''; extension=R4 ;;
  *) fail "unsupported retained R1 contract $version" ;;
esac
t=$(mktemp); trap 'rm -f "$t"' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | sort -u > "$t"
c=$(wc -l < "$t"|tr -d ' '); h=$(sha256sum "$t"|awk '{print $1}')
if [[ "$expected_count" != 0 ]]; then [[ "$c" == "$expected_count" && "$h" == "$expected_sha" ]] || fail "scope identity got=$c/$h expected=$expected_count/$expected_sha"; fi
[[ -f tools/c/view0_conformance/g05_r1a.c && -f tests/c/view0_v1n1_g05_r1a_global_attribute_test.c ]] || fail 'R1 paths missing'
if [[ "$extension" == R2 || "$extension" == R2A_SR1 || "$extension" == R3 || "$extension" == R4 ]]; then
  [[ -f tools/c/view0_conformance/g05_r2a.c && -f tests/c/view0_v1n1_g05_r2a_element_attribute_test.c ]] || fail 'R2 extension paths missing'
  if [[ "$extension" == R3 || "$extension" == R4 ]]; then
    [[ -f tools/c/view0_conformance/g05_r3a.c && -f tests/c/view0_v1n1_g05_r3a_conditional_applicability_test.c ]] || fail 'R3 extension paths missing'
    if [[ "$extension" == R4 ]]; then [[ -f tools/c/view0_conformance/g05_r4a.c ]] || fail 'R4 extension path missing'; else [[ ! -e tools/c/view0_conformance/g05_r4a.c ]] || fail 'R4 evaluator appeared'; fi
  else
    [[ ! -e tools/c/view0_conformance/g05_r3a.c && ! -e tools/c/view0_conformance/g05_r4a.c ]] || fail 'R3/R4 evaluator appeared'
  fi
  [[ "$extension" != R2A_SR1 && "$extension" != R3 && "$extension" != R4 || -f tests/data/view0_v1n1_g05_c0_sr1_input_state_authority.tsv ]] || fail 'C0-SR1 authority missing' 
  echo "VIEW0_V1N1_G05_R1A_SCOPE_EXTENSION_AWARE=${extension}_RETAINED_PATHS_PRESENT"
else
  [[ ! -e tools/c/view0_conformance/g05_r2a.c && ! -e tools/c/view0_conformance/g05_r3a.c && ! -e tools/c/view0_conformance/g05_r4a.c ]] || fail 'later G05 evaluator appeared'
fi
echo "VIEW0_V1N1_G05_R1A_CANDIDATE_PATH_COUNT=$c"
echo "VIEW0_V1N1_G05_R1A_CANDIDATE_PATHLIST_SHA256=$h"
echo 'VIEW0_V1N1_G05_R1A_STAGED_CHANGES=NO'
echo 'PASS: G05 R1A scope retained exactly or by reviewed R2/R3/R4 extension'
