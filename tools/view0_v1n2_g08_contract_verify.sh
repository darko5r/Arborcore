#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
need_line() { grep -Fqx -- "$2" "$1" || { printf 'FAIL: missing contract line: %s\n' "$2" >&2; exit 1; }; }
contract=view/arborcore-view-core-1.contract
need_line "$contract" 'VIEW0_V1N2_G08_C1_CONTRACT_REVISION=0.1-VIEW0-V1N2-G08-C1'
need_line "$contract" 'VIEW0_V1N2_G08_C1_RULES_IMPLEMENTED=12_OF_12'
need_line "$contract" 'VIEW0_V1N2_G08_C1_STATIC_DIAGNOSTIC_RULES=11'
need_line "$contract" 'VIEW0_V1N2_G08_C1_R11=HTML_FOREIGN_CONTENT_INTEGRATION_ONLY'
need_line "$contract" 'VIEW0_V1N2_G08_C1_WEBVTT_RESOURCE_BODY=DEFERRED'
need_line "$contract" 'VIEW0_V1N2_G08_C1_SVG_MATHML_LANGUAGE_CONFORMANCE=DEFERRED_HTML_INTEGRATION_ONLY'
need_line "$contract" 'VIEW0_V1N2_G08_C1_RUNTIME_MEDIA_FETCHING=EXCLUDED'
need_line "$contract" 'VIEW0_V1N2_G08_C1_TOTAL_PUBLIC_FUNCTION_COUNT=11'
need_line "$contract" 'VIEW0_V1N2_G08_C1_PHASED_STACK_BOUND_BYTES=900000'
need_line "$contract" 'VIEW0_V1N2_G08_C1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
[[ $(awk -F '\t' 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n2_g08_ownership.tsv) == 12 ]]
[[ $(awk -F '\t' 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n2_g08_fixture_plan.tsv) == 12 ]]
awk -F '\t' 'NR>1 { expected=sprintf("0x000000003008%04X", NR-1); if ($1 != expected || $2 != "G08") exit 1 } END {if (NR != 13) exit 1}' tests/data/view0_v1n2_g08_ownership.tsv
for ordinal in $(seq 1 12); do printf -v id '0x000000003008%04x' "$ordinal"; grep -Fiq "$id" tools/c/view0_conformance/v1n2_c0.c; done
grep -Fq 'arbor_view0_native_v1n2_g08_measure' tools/c/view0_conformance/g08.c
grep -Fq 'arbor_view0_native_v1n2_g08_collect_anchors' tools/c/view0_conformance/g08.c
grep -Fq 'arbor_view0_native_v1n2_g08_materialize_anchor' tools/c/view0_conformance/g08.c
grep -Fq 'foreign_integration_count' tools/c/view0_conformance/g08.c
if grep -En '(^|[^[:alnum:]_])(malloc|calloc|realloc|free|setlocale|localeconv)[[:space:]]*\(' tools/c/view0_conformance/g08.c tools/c/view0_conformance/g08.h >/dev/null; then
  echo 'FAIL: G08 introduces forbidden heap or locale operation' >&2; exit 1
fi
public_count=$(grep -Ec '^arbor_status arbor_view_|^void arbor_view_|^uint64_t arbor_view_|^const .*arbor_view_' include/arborcore/view.h)
[[ $public_count == 11 ]]
printf '%s\n' \
  'VIEW0_V1N2_G08_RULE_IDENTITIES=12_OF_12' \
  'VIEW0_V1N2_G08_DIAGNOSTIC_BOUNDARY=11_STATIC_PLUS_R11_HTML_INTEGRATION_ONLY' \
  'VIEW0_V1N2_G08_PRIOR_OWNER_BOUNDARY=G03_G05_G06_G07_RETAINED' \
  'VIEW0_V1N2_G08_WEBVTT_SVG_MATHML_EXTERNAL_BOUNDARY=EXPLICIT' \
  'VIEW0_V1N2_G08_RUNTIME_MEDIA_FETCHING=EXCLUDED' \
  'VIEW0_V1N2_G08_CONTRACT_VERIFY=PASS' \
  'PASS: V1N2 G08 exact authority, ownership and no-overclaim contract'
