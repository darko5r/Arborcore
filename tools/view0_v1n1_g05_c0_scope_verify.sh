#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
BASE='115d5dcee8e755edcfd0f5c447f9cfc9a0e38893'
BASE_TREE='91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f'
EXPECTED_COUNT='63'
EXPECTED_PATHLIST_SHA='a3f6b9ebd2beda46719e58c5abdda99146d1d8ae2c68e3f6ee2bab50ec64db1b'
[[ "$(git branch --show-current)" == 'view0-v1-completion' ]] || fail 'branch divergence'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'HEAD divergence'
[[ "$(git rev-parse HEAD^{tree})" == "$BASE_TREE" ]] || fail 'base tree divergence'
[[ -z "$(git diff --cached --name-only)" ]] || fail 'staged changes exist'
current=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1); [[ "$current" == '0.1-VIEW0-V1N1-G05-C0' || "$current" == '0.1-VIEW0-V1N1-G05-R1A' || "$current" == '0.1-VIEW0-V1N1-G05-R2A' || "$current" == '0.1-VIEW0-V1N1-G05-R2A-SR1' || "$current" == '0.1-VIEW0-V1N1-G05-R3A' || "$current" == '0.1-VIEW0-V1N1-G05-R4A' ]] || fail 'G05 C0 retained contract not current'
tmp=$(mktemp -d /tmp/arborcore-g05-c0-scope.XXXXXX); trap 'rm -rf "$tmp"' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u > "$tmp/actual"
actual_count=$(wc -l < "$tmp/actual" | tr -d ' ')
actual_sha=$(sha256sum "$tmp/actual" | awk '{print $1}')
if [[ "$current" == '0.1-VIEW0-V1N1-G05-C0' ]]; then
  [[ "$actual_count" == "$EXPECTED_COUNT" ]] || fail 'C0 path count drift'
  [[ "$actual_sha" == "$EXPECTED_PATHLIST_SHA" ]] || fail 'C0 pathlist SHA drift'
  [[ ! -e tools/c/view0_conformance/g05_r1a.c ]] || fail 'R1 evaluator appeared in exact C0'
  ! grep -ERq 'ARBOR_VIEW_V1_G05_|0x000000003005000[1-4]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G05 rule ID/symbol appeared in exact C0'
else
  for f in tests/data/view0_v1n1_g05_c0_global_attribute_catalog.tsv tests/data/view0_v1n1_g05_c0_element_attribute_catalog.tsv tests/data/view0_v1n1_g05_c0_conditional_predicate_catalog.tsv tests/data/view0_v1n1_g05_c0_body_window_event_catalog.tsv tools/c/view0_conformance/g05_c0.c tools/c/view0_conformance/g05_c0.h; do [[ -f "$f" ]] || fail "retained C0 path missing $f"; done
  [[ -f tools/c/view0_conformance/g05_r1a.c && -f tools/c/view0_conformance/g05_r1a.h ]] || fail 'R1A extension missing'
  if [[ "$current" != '0.1-VIEW0-V1N1-G05-R1A' ]]; then [[ -f tools/c/view0_conformance/g05_r2a.c ]] || fail 'R2A extension missing'; fi
  if [[ "$current" == '0.1-VIEW0-V1N1-G05-R3A' || "$current" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then [[ -f tools/c/view0_conformance/g05_r3a.c ]] || fail 'R3A extension missing'; fi
  if [[ "$current" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then [[ -f tools/c/view0_conformance/g05_r4a.c ]] || fail 'R4A extension missing'; fi
  [[ "$current" != '0.1-VIEW0-V1N1-G05-R2A-SR1' && "$current" != '0.1-VIEW0-V1N1-G05-R3A' && "$current" != '0.1-VIEW0-V1N1-G05-R4A' || -f tests/data/view0_v1n1_g05_c0_sr1_input_state_authority.tsv ]] || fail 'C0-SR1 authority map missing'
  echo "VIEW0_V1N1_G05_C0_SCOPE_EXTENSION_AWARE=$current"
fi
echo "VIEW0_V1N1_G05_C0_CANDIDATE_PATH_COUNT=$actual_count"
echo "VIEW0_V1N1_G05_C0_CANDIDATE_PATHLIST_SHA256=$actual_sha"
echo 'VIEW0_V1N1_G05_C0_G05_RULE_IDS_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G05_C0_STAGED_CHANGES=NO'
echo 'PASS: exact G05 C0 zero-rule foundation scope established'
