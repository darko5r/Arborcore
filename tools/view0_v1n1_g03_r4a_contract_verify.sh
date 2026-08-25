#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
R4_P0="${VIEW0_G03_R4_P0_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-R4-P0-nothing-model-preconstruction-review-candidate-3cd88ce7f42aaf0c622dff2a145624beccf9cc303ae5c53e7fee8d1fdc249e13.tar.gz}"
F1_R8="${VIEW0_G03_F1_R8_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-M0B-F1-R8-g03-r4a-partial-support-plan-and-fixture-freeze-candidate-44a05511236cdc592a8f367bec5fe1288a61585cfc18f3984bcbe674283dbeb9.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }; need_eq(){ [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }
contract=view/arborcore-view-core-1.contract; header=tools/include/arborcore/view0_conformance/native.h; native=tools/c/view0_conformance/native.c; r4=tools/c/view0_conformance/g03_r4a.c; coverage=tests/data/view0_v1n1_g03_r4a_subject_support.tsv
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R4A|0.1-VIEW0-V1N1-G03-R5A|0.1-VIEW0-V1N1-G03-R6A|0.1-VIEW0-V1N1-G03-R7A) ;;
  *) fail "current contract is not an accepted R4A extension: $current_version" ;;
esac
for item in \
 'VIEW0_V1N1_G03_R4A_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-R4A' \
 'VIEW0_V1N1_G03_R4A_PREDECESSOR_CANDIDATE_PATH_COUNT=155' \
 'VIEW0_V1N1_G03_R4A_PREDECESSOR_CANDIDATE_MANIFEST_SHA256=ba050efd0c961a085cf60a5a1eb589017fa325d35f2b9670c73adae8263173ea' \
 'VIEW0_V1N1_G03_R4A_RULE_ID=0x0000000030030004' \
 'VIEW0_V1N1_G03_R4A_SUBJECT_DEFINITION_COUNT=18' \
 'VIEW0_V1N1_G03_R4A_VACUOUS_SUBJECT_COUNT=14' \
 'VIEW0_V1N1_G03_R4A_IMPLEMENT_NOW_SUBJECT_COUNT=3' \
 'VIEW0_V1N1_G03_R4A_DEFERRED_SUBJECT_COUNT=1' \
 'VIEW0_V1N1_G03_R4A_DEFERRED_SUBJECT=selectedcontent' \
 'VIEW0_V1N1_G03_R4A_IMPLEMENTATION_COMPLETE=NO' \
 'VIEW0_V1N1_G03_R4A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_R4A_G03_GROUP_FREEZE=NO'; do grep -Fxq "$item" "$contract" || fail "R4A contract marker missing: $item"; done
for spec in "$R4_P0|3cd88ce7f42aaf0c622dff2a145624beccf9cc303ae5c53e7fee8d1fdc249e13|R4-P0" "$F1_R8|44a05511236cdc592a8f367bec5fe1288a61585cfc18f3984bcbe674283dbeb9|F1-R8"; do IFS='|' read -r f h l <<<"$spec"; [[ -f "$f" ]] || fail "missing $l bundle"; need_eq "$(sha256sum "$f"|awk '{print $1}')" "$h" "$l SHA"; done
need_eq "$(sha256sum "$coverage"|awk '{print $1}')" 'ab6e86b13ee808342da3e97cc78306001b6c58352bb95678a72c00a0ca94182c' 'R4A support ledger'
grep -Fq '#define ARBOR_VIEW_V1_G03_NOTHING_MODEL UINT64_C(0x0000000030030004)' "$header" || fail 'R4 rule ID missing'
grep -Fq 'ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL' "$header" || fail 'R4 partial flag missing'
grep -Fq 'ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE' "$header" || fail 'R4 selectedcontent flag missing'
grep -Fq '#include "g03_r4a.h"' "$native" || fail 'native checker missing R4A integration'
grep -Fq 'arbor_view0_native_g03_r4a_measure' "$native" || fail 'R4A measure missing'
grep -Fq 'arbor_view0_native_g03_r4a_collect' "$native" || fail 'R4A collect missing'
grep -Fq '_Static_assert(sizeof(g03_r4a_frame) == 40u' "$r4" || fail 'R4 frame assertion missing'
grep -Fq '_Static_assert(sizeof(g03_r4a_context) == 163992u' "$r4" || fail 'R4 workspace assertion missing'
grep -Fq '#define G03_R4A_NOINLINE __attribute__((noinline))' "$r4" || fail 'R4 phased noinline missing'
if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' "$r4"; then fail 'R4A direct heap allocation introduced'; fi
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R5-R7 semantics appeared during R4A'
else
    grep -Fq '#define ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE UINT64_C(0x0000000030030005)' "$header" || fail 'R5A extension rule ID missing under retained R4A'
    grep -Fq '#include "g03_r5a.h"' "$native" || fail 'R5A extension integration missing under retained R4A'
    grep -Fq 'arbor_view0_native_g03_r4a_collect_offsets' "$r4" || fail 'R4A private offset-collection compatibility mode missing under R5A'
    if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]]; then
        grep -Fq '#define ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY UINT64_C(0x0000000030030007)' "$header" || fail 'R7A extension rule ID missing'
        grep -Fq '#include "g03_r7a.h"' "$native" || fail 'R7A extension integration missing'
        ! grep -ERq 'ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT|0x0000000030030006|G03_R6' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R6 runtime semantic/flag code appeared under R7A'
    else
        ! grep -ERq 'ARBOR_VIEW_V1_G03_(SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[67]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R6-R7 runtime semantics appeared before R7A'
    fi
fi
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
echo 'VIEW0_V1N1_G03_R4A_SUBJECT_SUPPORT_LEDGER=18_OF_18'
echo 'VIEW0_V1N1_G03_R4A_VACUOUS_SUBJECT_COUNT=14'
echo 'VIEW0_V1N1_G03_R4A_IMPLEMENT_NOW_SUBJECT_COUNT=3'
echo 'VIEW0_V1N1_G03_R4A_DEFERRED_SUBJECT_COUNT=1'
echo 'VIEW0_V1N1_G03_R4A_RULE_ID=0x0000000030030004'
echo 'VIEW0_V1N1_G03_R4A_IMPLEMENTATION_COMPLETE=NO'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    echo 'VIEW0_V1N1_G03_R4A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R5A' ]]; then
    echo 'VIEW0_V1N1_G03_R4A_RETAINED_UNDER_G03_R5A=PASS'
    echo 'VIEW0_V1N1_G03_R4A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R4A_G03_R6_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]]; then
    echo 'VIEW0_V1N1_G03_R4A_RETAINED_UNDER_G03_R6A=PASS'
    echo 'VIEW0_V1N1_G03_R4A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R4A_G03_R7_RULE_IDS_IMPLEMENTED=ZERO'
else
    echo 'VIEW0_V1N1_G03_R4A_RETAINED_UNDER_G03_R7A=PASS'
    echo 'VIEW0_V1N1_G03_R4A_PRIVATE_OFFSET_COLLECTION=PASS'
fi
echo 'VIEW0_V1N1_G03_R4A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'PASS: G03 R4A accepted F1-R8 support/fixture boundary, resources and no-growth contract'
