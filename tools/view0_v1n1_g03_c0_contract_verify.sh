#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }
contract='view/arborcore-view-core-1.contract'
header='tools/include/arborcore/view0_conformance/native.h'
adapter='tools/c/view0_conformance/lexbor_adapter.c'
provenance='tools/c/view0_conformance/g03_provenance.c'
doc='docs/VIEW_CORE_VIEW0.md'

current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -n1)"
case "$current_version" in
  0.1-VIEW0-V1N1-G03-C0) extension_aware='NO' ;;
  0.1-VIEW0-V1N1-G03-C0-L1|0.1-VIEW0-V1N1-G03-R1A|0.1-VIEW0-V1N1-G03-R2A|0.1-VIEW0-V1N1-G03-R3A|0.1-VIEW0-V1N1-G03-R4A) extension_aware='YES' ;;
  *) fail "current contract is not an accepted G03 C0 extension: $current_version" ;;
esac

for item in \
 'VIEW0_V1N1_G03_C0_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-C0' \
 'VIEW0_V1N1_G03_C0_SCOPE=PROVENANCE_AND_NEUTRAL_DOM_OBSERVATION_SUBSTRATE_OVER_ACCEPTED_G02_GF1' \
 'VIEW0_V1N1_G03_C0_P0_REVIEW_BUNDLE_SHA256=53d42457fcedc58b58983b04b07abb5d54a8f5613584878734f316f1b8ec7ca6' \
 'VIEW0_V1N1_G03_C0_G02_GF1_BUNDLE_SHA256=cfe9f86d293ef8e0723d5f92569801c5c154036b57fd94d5872b0717b5c40096' \
 'VIEW0_V1N1_G03_C0_F1_R2_MATRIX_SHA256=2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94' \
 'VIEW0_V1N1_G03_C0_STANDARD_ELEMENT_TOKEN_COUNT=113' \
 'VIEW0_V1N1_G03_C0_CONTEXT_ALTERNATIVE_COUNT=62' \
 'VIEW0_V1N1_G03_C0_CONTENT_MODEL_FORM_COUNT=46' \
 'VIEW0_V1N1_G03_C0_STANDARD_ELEMENT_ID_DEPENDS_ON_LEXBOR_TAG_ID=NO' \
 'VIEW0_V1N1_G03_C0_PROVENANCE_MECHANISM=GNU_COMPATIBLE_LD_WRAP_LXB_HTML_INTERFACE_CREATE_STATIC_LEXBOR_ONLY' \
 'VIEW0_V1N1_G03_C0_PROVENANCE_WRAPPABILITY_FAIL_CLOSED=YES' \
 'VIEW0_V1N1_G03_C0_LEXBOR_SOURCE_MODIFICATION=NO' \
 'VIEW0_V1N1_G03_C0_SOURCE_POINTER_ESCAPES_ADAPTER=NO' \
 'VIEW0_V1N1_G03_C0_DOM_COPY=NO' \
 'VIEW0_V1N1_G03_C0_TRAVERSAL=ITERATIVE_PARENT_CHILD_SIBLING' \
 'VIEW0_V1N1_G03_C0_C_RECURSIVE_DOM_WALK=NO' \
 'VIEW0_V1N1_G03_C0_ANCESTOR_COUNTER_COUNT=114' \
 'VIEW0_V1N1_G03_C0_ANCESTOR_BIT_WORD_COUNT=2' \
 'VIEW0_V1N1_G03_C0_TEMPLATE_CONTENTS_TRAVERSED=NO' \
 'VIEW0_V1N1_G03_C0_ADAPTER_OUTPUT_FAILURE_ATOMICITY=YES' \
 'VIEW0_V1N1_G03_C0_C0_DOCUMENT_FACT_LAYOUT_CHANGE=NO' \
 'VIEW0_V1N1_G03_C0_C0_DOCUMENT_FACT_SIZE_X86_64=184' \
 'VIEW0_V1N1_G03_C0_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G03_C0_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G03_C0_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G03_C0_G02_GROUP_RETAINED=FROZEN_6_OF_6' \
 'VIEW0_V1N1_G03_C0_G03_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_C0_G03_R1_ELEMENT_CONTEXT_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G03_C0_G04_G06_RULES_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_C0_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
    grep -Fxq "$item" "$contract" || fail "missing G03 C0 contract marker: $item"
done

[[ "$(grep -Ec '^    ARBOR_VIEW0_NATIVE_ELEMENT_[A-Z0-9_]+ = ([1-9][0-9]*),$' "$header")" -eq 113 ]] || fail 'stable standard-element ID enumeration does not contain exactly 113 data IDs'
grep -Fq '#define ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT UINT64_C(113)' "$header" || fail 'standard element count macro missing'
grep -Fq '#define ARBOR_VIEW0_NATIVE_ANCESTOR_WORD_COUNT UINT64_C(2)' "$header" || fail 'ancestor bit word count missing'
grep -Fq 'arbor_status arbor_view0_native_lexbor_observe(' "$header" || fail 'private G03 observation API missing'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked across private Arborcore header'
! grep -ERq 'ARBOR_VIEW_V1_G03_|0x000000003003' "$adapter" "$provenance" || fail 'G03 C0 mechanism files acquired rule semantics'

grep -Fq '__wrap_lxb_html_interface_create' "$provenance" || fail 'qualified provenance wrapper implementation missing'
grep -Fq '__real_lxb_html_interface_create' "$provenance" || fail 'provenance wrapper does not delegate to real Lexbor constructor'
grep -Fq 'document->dom_document.user' "$provenance" || fail 'stack parse context is not recovered through document user field'
grep -Fq 'node->user = (void *)context->current_begin;' "$provenance" || fail 'authored-node source provenance assignment missing'
grep -Fq 'observe_document(' "$adapter" || fail 'neutral iterative observation traversal missing'
grep -Fq 'ancestor_counters[114]' "$adapter" || fail 'fixed ancestor counters missing'
grep -Fq 'ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE' "$adapter" || fail 'direct-text whitespace classification missing'
[[ "$(grep -c 'observe_document(' "$adapter")" -eq 2 ]] || fail 'observe_document recursion/call-shape drift'

if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' "$adapter" "$provenance"; then
    fail 'G03 C0 introduced direct Arborcore heap allocation'
fi

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "G03 C0 changed production VIEW API count: $count"
! grep -Rq 'lxb_' include/arborcore/view.h || fail 'Lexbor leaked into production VIEW header'

grep -Fq '## V1N1 G03 C0: parser-repair provenance and neutral semantic observation substrate' "$doc" || fail 'G03 C0 documentation section missing'
grep -Fq '`template` contents are intentionally not traversed.' "$doc" || fail 'template-content exclusion documentation missing'

echo "VIEW0_V1N1_G03_C0_EXTENSION_AWARE_RETENTION=$extension_aware"
echo 'VIEW0_V1N1_G03_C0_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G03_C0_STANDARD_ELEMENT_ID_COUNT=113'
echo 'VIEW0_V1N1_G03_C0_G03_RULE_IDS_IMPLEMENTED=ZERO'
echo 'PASS: G03 C0 provenance, neutral observation, ownership and zero-rule contract'
