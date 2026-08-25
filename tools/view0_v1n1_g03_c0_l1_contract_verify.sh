#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }
contract='view/arborcore-view-core-1.contract'
header='tools/include/arborcore/view0_conformance/native.h'
adapter='tools/c/view0_conformance/lexbor_adapter.c'
doc='docs/VIEW_CORE_VIEW0.md'

current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -n1)"
case "$current_version" in
  0.1-VIEW0-V1N1-G03-C0-L1|0.1-VIEW0-V1N1-G03-R1A|0.1-VIEW0-V1N1-G03-R2A|0.1-VIEW0-V1N1-G03-R3A|0.1-VIEW0-V1N1-G03-R4A) ;;
  *) fail "current contract is not an accepted G03 C0-L1 extension: $current_version" ;;
esac

for item in \
 'VIEW0_V1N1_G03_C0_L1_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-C0-L1' \
 'VIEW0_V1N1_G03_C0_L1_SCOPE=GENERIC_TRUE_DFS_TRAVERSAL_LIFECYCLE_RETROFIT_OVER_ACCEPTED_G03_C0' \
 'VIEW0_V1N1_G03_C0_L1_M1_R3_F1_R4_BUNDLE_SHA256=92bdecbe1b5ab71d693e62a8bcf7b6f587f5128ec9320f7f8974965648b9c970' \
 'VIEW0_V1N1_G03_C0_L1_F1_R4_MATRIX_SHA256=6a3a6b22878ad6dbc6e52622af8cc1a79759b4214b2c4cf82a33dd70cca447ca' \
 'VIEW0_V1N1_G03_C0_L1_G03_R1_SOURCE_SET_SHA256=2e37e1de692b4cc0ce6877f6526c3845554319f67a6ba4969de4a38797c15687' \
 'VIEW0_V1N1_G03_C0_L1_PRIVATE_CALLBACK_ADDITION=TRAVERSAL_ENTER_AND_TRAVERSAL_LEAVE' \
 'VIEW0_V1N1_G03_C0_L1_TRAVERSAL_ENTER_ORDER=BEFORE_EXISTING_ELEMENT_BEGIN' \
 'VIEW0_V1N1_G03_C0_L1_TRAVERSAL_LEAVE_ORDER=AFTER_ALL_DESCENDANT_ELEMENT_TRAVERSAL' \
 'VIEW0_V1N1_G03_C0_L1_EXISTING_ELEMENT_BEGIN_SEMANTICS_CHANGE=NO' \
 'VIEW0_V1N1_G03_C0_L1_EXISTING_ELEMENT_COMPLETE_SEMANTICS_CHANGE=NO' \
 'VIEW0_V1N1_G03_C0_L1_LEAVE_AFTER_PRIOR_CALLBACK_FAILURE=NOT_GUARANTEED' \
 'VIEW0_V1N1_G03_C0_L1_ANCESTOR_BITS_INCLUDE_CURRENT_ELEMENT=NO' \
 'VIEW0_V1N1_G03_C0_L1_TRAVERSAL_IMPLEMENTATION=ITERATIVE_PARENT_CHILD_SIBLING_NO_RECURSION' \
 'VIEW0_V1N1_G03_C0_L1_SHADOW_DOM=NO' \
 'VIEW0_V1N1_G03_C0_L1_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G03_C0_L1_LEXBOR_POINTER_ESCAPE=NO' \
 'VIEW0_V1N1_G03_C0_L1_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G03_C0_L1_OUTPUT_FAILURE_ATOMICITY=YES' \
 'VIEW0_V1N1_G03_C0_L1_QUALIFIED_NESTED_DIV_COUNT=4096' \
 'VIEW0_V1N1_G03_C0_L1_QUALIFIED_MAX_OBSERVATION_DEPTH=4097' \
 'VIEW0_V1N1_G03_C0_L1_R1_MAX_OBSERVATION_DEPTH_INCLUSIVE=4097' \
 'VIEW0_V1N1_G03_C0_L1_R1_DEPTH_POLICY_OWNER=FUTURE_G03_R1_EVALUATOR_NOT_ADAPTER' \
 'VIEW0_V1N1_G03_C0_L1_ACCESSIBLE_NAME_SUPPORT=NO' \
 'VIEW0_V1N1_G03_C0_L1_C0_DOCUMENT_FACT_LAYOUT_CHANGE=NO' \
 'VIEW0_V1N1_G03_C0_L1_C0_DOCUMENT_FACT_SIZE_X86_64=184' \
 'VIEW0_V1N1_G03_C0_L1_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G03_C0_L1_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G03_C0_L1_G03_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_C0_L1_G03_R1_ELEMENT_CONTEXT_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G03_C0_L1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
    grep -Fxq "$item" "$contract" || fail "missing G03 C0-L1 contract marker: $item"
done

grep -Fq 'arbor_view0_native_element_observer_f traversal_enter;' "$header" || fail 'traversal_enter callback missing'
grep -Fq 'arbor_view0_native_element_observer_f traversal_leave;' "$header" || fail 'traversal_leave callback missing'
grep -Fq 'build_element_observation(' "$adapter" || fail 'shared neutral observation constructor missing'
grep -Fq 'observer->traversal_enter' "$adapter" || fail 'traversal_enter emission missing'
grep -Fq 'observer->traversal_leave' "$adapter" || fail 'traversal_leave emission missing'
grep -Fq 'ancestor_counter_remove(ancestor_counters, exiting_id);' "$adapter" || fail 'ancestor removal boundary missing'

enter_line=$(grep -n 'observer->traversal_enter' "$adapter" | head -n1 | cut -d: -f1)
begin_line=$(grep -n 'observer->element_begin' "$adapter" | head -n1 | cut -d: -f1)
remove_line=$(grep -n 'ancestor_counter_remove(ancestor_counters, exiting_id);' "$adapter" | head -n1 | cut -d: -f1)
leave_call_line=$(grep -n 'status = observe_element_leave(' "$adapter" | head -n1 | cut -d: -f1)
[[ "$enter_line" -lt "$begin_line" ]] || fail 'traversal_enter is not before element_begin'
[[ "$remove_line" -lt "$leave_call_line" ]] || fail 'traversal_leave is not after current-element ancestor removal'

! grep -ERq 'ARBOR_VIEW_V1_G03_|0x000000003003' tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c || fail 'G03 C0-L1 mechanism files acquired rule semantics'
if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c; then
    fail 'G03 C0-L1 introduced direct Arborcore heap allocation'
fi
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
grep -Fq '## V1N1 G03 C0-L1: true DFS traversal lifetime retrofit' "$doc" || fail 'G03 C0-L1 documentation missing'

echo 'VIEW0_V1N1_G03_C0_L1_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G03_C0_L1_MAX_QUALIFIED_DEPTH=4097'
echo 'VIEW0_V1N1_G03_C0_L1_ACCESSIBLE_NAME_SUPPORT=NO'
echo 'VIEW0_V1N1_G03_C0_L1_G03_RULE_IDS_IMPLEMENTED=ZERO'
echo 'PASS: G03 C0-L1 true DFS lifecycle, resource, ownership and zero-rule contract'
