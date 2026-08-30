#!/usr/bin/env bash
set -euo pipefail

ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"

need_line() {
    local file=$1 line=$2
    grep -Fqx -- "$line" "$file" || { printf 'FAIL: missing contract line: %s\n' "$line" >&2; exit 1; }
}

contract=view/arborcore-view-core-1.contract
need_line "$contract" 'VIEW0_V1N2_G07_C1_CONTRACT_REVISION=0.1-VIEW0-V1N2-G07-C1'
need_line "$contract" 'VIEW0_V1N2_G07_C1_RULES_IMPLEMENTED=5_OF_5'
need_line "$contract" 'VIEW0_V1N2_G07_C1_G03_CONTENT_MODEL_OWNER=RETAINED_NO_DUPLICATE'
need_line "$contract" 'VIEW0_V1N2_G07_C1_G05_ATTRIBUTE_APPLICABILITY_OWNER=RETAINED_NO_DUPLICATE'
need_line "$contract" 'VIEW0_V1N2_G07_C1_G06_TOKEN_VALUE_OWNER=RETAINED_NO_DUPLICATE'
need_line "$contract" 'VIEW0_V1N2_G07_C1_EXTENSION_RELATION_REGISTRY=UNFROZEN_NON_REJECTING'
need_line "$contract" 'VIEW0_V1N2_G07_C1_NAVIGATION_FETCH_PING_RUNTIME_ALGORITHMS=EXCLUDED'
need_line "$contract" 'VIEW0_V1N2_G07_C1_TOTAL_PUBLIC_FUNCTION_COUNT=11'
need_line "$contract" 'VIEW0_V1N2_G07_C1_PHASED_STACK_BOUND_BYTES=900000'
need_line "$contract" 'VIEW0_V1N2_G07_C1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'

[[ $(awk -F '\t' 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n2_g07_ownership.tsv) == 5 ]]
[[ $(awk -F '\t' 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n2_g07_fixture_plan.tsv) == 6 ]]
awk -F '\t' 'NR>1 {
    expected=sprintf("0x000000003007%04x", $1)
    if ($2 != expected) exit 1
    if ($3 !~ /^ARBOR_VIEW_V1_G07_/) exit 1
} END {if (NR != 6) exit 1}' tests/data/view0_v1n2_g07_ownership.tsv

for ordinal in 1 2 3 4 5; do
    printf -v id '0x000000003007%04x' "$ordinal"
    grep -Fq "$id" tools/c/view0_conformance/v1n2_c0.c
done

grep -Fq 'arbor_view0_native_v1n2_g07_measure' tools/c/view0_conformance/g07.c
grep -Fq 'arbor_view0_native_v1n2_g07_collect_anchors' tools/c/view0_conformance/g07.c
grep -Fq 'arbor_view0_native_v1n2_g07_materialize_anchor' tools/c/view0_conformance/g07.c
grep -Fq 'extension_relation_deferred_count' tools/c/view0_conformance/g07.c

if grep -En '(^|[^[:alnum:]_])(malloc|calloc|realloc|free|setlocale|localeconv)[[:space:]]*\(' \
    tools/c/view0_conformance/g07.c tools/c/view0_conformance/g07.h >/dev/null; then
    echo 'FAIL: G07 introduces forbidden heap or locale operation' >&2
    exit 1
fi

public_count=$(grep -Ec '^arbor_status arbor_view_|^void arbor_view_|^uint64_t arbor_view_|^const .*arbor_view_' include/arborcore/view.h)
[[ $public_count == 11 ]] || { echo "FAIL: production VIEW public function count=$public_count" >&2; exit 1; }

printf '%s\n' \
    'VIEW0_V1N2_G07_RULE_IDENTITIES=5_OF_5' \
    'VIEW0_V1N2_G07_PRIOR_OWNER_BOUNDARY=G03_G05_G06_RETAINED' \
    'VIEW0_V1N2_G07_EXTENSION_REGISTRY=UNFROZEN_NON_REJECTING' \
    'VIEW0_V1N2_G07_RUNTIME_NAVIGATION_FETCH_PING=EXCLUDED' \
    'VIEW0_V1N2_G07_CONTRACT_VERIFY=PASS' \
    'PASS: V1N2 G07 exact authority, ownership and no-overclaim contract'
