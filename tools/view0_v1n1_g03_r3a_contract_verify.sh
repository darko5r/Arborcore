#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
R3_P0_BUNDLE="${VIEW0_G03_R3_P0_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-R3-P0-descendant-exclusion-preconstruction-review-candidate-2d3482eeabdbf547941ba0a9ff61d24d6af5ca96dc749653da9f8062e7b02acd.tar.gz}"
F1_R7_BUNDLE="${VIEW0_G03_F1_R7_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-M0B-F1-R7-g03-r3a-partial-support-plan-freeze-candidate-e4c9cba26bd612b19ec63aefccd80562a633310f689aaaef10ea6b72755776ec.tar.gz}"
fail() { echo "FAIL: $*" >&2; exit 1; }
need_eq() { [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }

contract='view/arborcore-view-core-1.contract'
header='tools/include/arborcore/view0_conformance/native.h'
native='tools/c/view0_conformance/native.c'
r1a='tools/c/view0_conformance/g03_r1a.c'
r3a='tools/c/view0_conformance/g03_r3a.c'
doc='docs/VIEW_CORE_VIEW0.md'
coverage='tests/data/view0_v1n1_g03_r3a_element_support.tsv'

current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R3A|0.1-VIEW0-V1N1-G03-R4A|0.1-VIEW0-V1N1-G03-R5A|0.1-VIEW0-V1N1-G03-R6A|0.1-VIEW0-V1N1-G03-R7A) ;;
  *) fail "current contract is not an accepted R3A extension: $current_version" ;;
esac
for item in \
 'VIEW0_V1N1_G03_R3A_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-R3A' \
 'VIEW0_V1N1_G03_R3A_SCOPE=PARTIAL_DESCENDANT_EXCLUSIONS_EVALUATOR_OVER_ACCEPTED_R2A_AND_V1N0_LX1' \
 'VIEW0_V1N1_G03_R3A_PREDECESSOR_CANDIDATE_PATH_COUNT=146' \
 'VIEW0_V1N1_G03_R3A_PREDECESSOR_CANDIDATE_MANIFEST_SHA256=debcb28bcf096319a0b30191dda02e7431556c783909db431526baf910c69b42' \
 'VIEW0_V1N1_G03_R3A_R3_P0_BUNDLE_SHA256=2d3482eeabdbf547941ba0a9ff61d24d6af5ca96dc749653da9f8062e7b02acd' \
 'VIEW0_V1N1_G03_R3A_R3_P0_ELEMENT_INVENTORY_SHA256=3d5116bb527cff121d563ba2efe12f36966d64ac766373360d8dec86d1ef0188' \
 'VIEW0_V1N1_G03_R3A_R3_P0_INTERACTIVE_CATEGORY_INVENTORY_SHA256=4188ad3e86577840e8535573afbfefa6b68d3ab28b5cfd0a43aedd8c22e11553' \
 'VIEW0_V1N1_G03_R3A_F1_R7_BUNDLE_SHA256=e4c9cba26bd612b19ec63aefccd80562a633310f689aaaef10ea6b72755776ec' \
 'VIEW0_V1N1_G03_R3A_F1_R7_MATRIX_SHA256=193d27d80161f2cd8e78cedf15e05f92403cd682bc62afb7a1f754ee556c6a45' \
 'VIEW0_V1N1_G03_R3A_F1_R7_ELEMENT_SUPPORT_LEDGER_SHA256=5e218a206329dcf6dd1798545af0257f08652f1de96864a9235276e9d39cdc28' \
 'VIEW0_V1N1_G03_R3A_RULE_ID=0x0000000030030003' \
 'VIEW0_V1N1_G03_R3A_RULE_SYMBOL=ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS' \
 'VIEW0_V1N1_G03_R3A_SEVERITY=ERROR' \
 'VIEW0_V1N1_G03_R3A_ELEMENT_DEFINITION_COUNT=20' \
 'VIEW0_V1N1_G03_R3A_STATIC_PARENT_COUNT=13' \
 'VIEW0_V1N1_G03_R3A_PARTIAL_PARENT_COUNT=6' \
 'VIEW0_V1N1_G03_R3A_DEFERRED_PARENT_COUNT=1' \
 'VIEW0_V1N1_G03_R3A_EXPLICIT_DEFERRED_BRANCH_FAMILY_COUNT=5' \
 'VIEW0_V1N1_G03_R3A_IMPLEMENTATION_COMPLETE=NO' \
 'VIEW0_V1N1_G03_R3A_INPUT_TYPE_INTERACTIVE_CLASSIFICATION=DEFERRED_G06_ENUMERATED_VALUE' \
 'VIEW0_V1N1_G03_R3A_LABEL_LABELED_CONTROL_RESOLUTION=DEFERRED_EXACT_FOR_ID_OR_IMPLICIT_FIRST_LABELABLE' \
 'VIEW0_V1N1_G03_R3A_CANVAS_INPUT_STATE_EXCEPTIONS=DEFERRED_G06_ENUMERATED_VALUE' \
 'VIEW0_V1N1_G03_R3A_CANVAS_SELECT_SIZE_ATTRIBUTE=DEFERRED_G06_NON_NEGATIVE_INTEGER' \
 'VIEW0_V1N1_G03_R3A_NOSCRIPT_SCRIPTING_MODE=DEFERRED_EXPLICIT_CHECKER_MODE' \
 'VIEW0_V1N1_G03_R3A_PARSER_REPAIR_ERASED_RELATION_OWNER=G03_R5' \
 'VIEW0_V1N1_G03_R3A_R1_DUPLICATE_SUPPRESSION=REUSE_EXACT_R1A_EVALUATOR_SOURCE_OFFSETS' \
 'VIEW0_V1N1_G03_R3A_R1_PRIVATE_OFFSET_COLLECTION=NO_NEW_R1_SEMANTICS_NO_PUBLIC_API' \
 'VIEW0_V1N1_G03_R3A_R2_DESCENDANT_DUPLICATE_POLICY=R2_DELEGATES_DESCENDANT_WIDE_SEMANTICS_TO_R3' \
 'VIEW0_V1N1_G03_R3A_DIAGNOSTIC_SCOPE=AUTHORED_FORBIDDEN_DESCENDANT' \
 'VIEW0_V1N1_G03_R3A_DIAGNOSTIC_ANCHOR=AUTHORED_FORBIDDEN_DESCENDANT_START_TAG_NAME_BYTE_RANGE' \
 'VIEW0_V1N1_G03_R3A_ONE_DIAGNOSTIC_PER_FORBIDDEN_DESCENDANT_MAX=YES' \
 'VIEW0_V1N1_G03_R3A_CAPACITY_FAILURE_ATOMICITY=TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G03_R3A_MAX_OBSERVATION_DEPTH_INCLUSIVE=4097' \
 'VIEW0_V1N1_G03_R3A_FRAME_SIZE_X86_64=64' \
 'VIEW0_V1N1_G03_R3A_EVALUATOR_WORKSPACE_SIZE_X86_64=262472' \
 'VIEW0_V1N1_G03_R3A_R1_OFFSET_SCRATCH_BYTES=32768' \
 'VIEW0_V1N1_G03_R3A_PHASED_STACK_BOUND=R1A_PASS_THEN_NOINLINE_R3A_PASS' \
 'VIEW0_V1N1_G03_R3A_R1_PASS_COMPILED_STACK_BOUND_BYTES=900000' \
 'VIEW0_V1N1_G03_R3A_R3_PASS_COMPILED_STACK_BOUND_BYTES=400000' \
 'VIEW0_V1N1_G03_R3A_EVALUATOR_WORKSPACE_MAX_BYTES=1048576' \
 'VIEW0_V1N1_G03_R3A_EVALUATOR_WORKSPACE_STORAGE=CALL_STACK_FIXED_BOUNDED' \
 'VIEW0_V1N1_G03_R3A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G03_R3A_C0_C0_L1_RETROFIT_REQUIRED=NO' \
 'VIEW0_V1N1_G03_R3A_R1A_RETAINED=YES_PARTIAL' \
 'VIEW0_V1N1_G03_R3A_R2A_RETAINED=YES_PARTIAL' \
 'VIEW0_V1N1_G03_R3A_V1N0_LX1_RETAINED=YES' \
 'VIEW0_V1N1_G03_R3A_G02_GROUP_RETAINED=FROZEN_6_OF_6' \
 'VIEW0_V1N1_G03_R3A_G03_RULE_IDS_ACTIVE=3' \
 'VIEW0_V1N1_G03_R3A_G03_R4_R7_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_R3A_G03_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G03_R3A_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G03_R3A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G03_R3A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
    grep -Fxq "$item" "$contract" || fail "missing R3A contract marker: $item"
done

[[ -f "$R3_P0_BUNDLE" ]] || fail "missing exact R3-P0 bundle: $R3_P0_BUNDLE"
[[ -f "$F1_R7_BUNDLE" ]] || fail "missing exact F1-R7 bundle: $F1_R7_BUNDLE"
need_eq "$(sha256sum "$R3_P0_BUNDLE" | awk '{print $1}')" '2d3482eeabdbf547941ba0a9ff61d24d6af5ca96dc749653da9f8062e7b02acd' 'R3-P0 archive SHA-256'
need_eq "$(sha256sum "$F1_R7_BUNDLE" | awk '{print $1}')" 'e4c9cba26bd612b19ec63aefccd80562a633310f689aaaef10ea6b72755776ec' 'F1-R7 archive SHA-256'

tmp=$(mktemp -d /tmp/arborcore-g03-r3a-contract.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/p0" "$tmp/r7"
tar -xzf "$R3_P0_BUNDLE" -C "$tmp/p0"
tar -xzf "$F1_R7_BUNDLE" -C "$tmp/r7"
p0="$tmp/p0/Arborcore-VIEW0-V1N1-G03-R3-P0-descendant-exclusion-preconstruction-review-candidate"
r7="$tmp/r7/Arborcore-VIEW0-V1N1-M0B-F1-R7-g03-r3a-partial-support-plan-freeze-candidate"
need_eq "$(sha256sum "$p0/r3-p0-manifest.sha256" | awk '{print $1}')" 'd0c181ec211039f492c2b070522d05317ac08d394f419ba80e1ef00012211378' 'R3-P0 internal manifest identity'
(cd "$p0" && sha256sum -c r3-p0-manifest.sha256 >/dev/null)
need_eq "$(sha256sum "$r7/f1-r7-manifest.sha256" | awk '{print $1}')" 'b3b521d281e817a7a2d10b939a5a36b933a977d042b19be07ef416aac6fe0dfa' 'F1-R7 internal manifest identity'
(cd "$r7" && sha256sum -c f1-r7-manifest.sha256 >/dev/null)
need_eq "$(sha256sum "$p0/g03-r3-element-predicate-inventory.tsv" | awk '{print $1}')" '3d5116bb527cff121d563ba2efe12f36966d64ac766373360d8dec86d1ef0188' 'R3-P0 element inventory'
need_eq "$(sha256sum "$p0/g03-r3-interactive-category-inventory.tsv" | awk '{print $1}')" '4188ad3e86577840e8535573afbfefa6b68d3ab28b5cfd0a43aedd8c22e11553' 'R3-P0 interactive inventory'
need_eq "$(sha256sum "$r7/f1-final-rule-spec-test-matrix.tsv" | awk '{print $1}')" '193d27d80161f2cd8e78cedf15e05f92403cd682bc62afb7a1f754ee556c6a45' 'F1-R7 matrix'
need_eq "$(sha256sum "$r7/f1-r7-r3a-element-support-plan.tsv" | awk '{print $1}')" '5e218a206329dcf6dd1798545af0257f08652f1de96864a9235276e9d39cdc28' 'F1-R7 support ledger'
need_eq "$(sha256sum "$coverage" | awk '{print $1}')" '5e218a206329dcf6dd1798545af0257f08652f1de96864a9235276e9d39cdc28' 'repository R3A support ledger'
cmp -s "$r7/f1-r7-r3a-element-support-plan.tsv" "$coverage" || fail 'repository R3A support ledger differs from accepted F1-R7'

python3 - "$p0/g03-r3-element-predicate-inventory.tsv" "$coverage" "$r7/f1-final-rule-spec-test-matrix.tsv" <<'PY'
import csv,sys
from collections import Counter
with open(sys.argv[1],newline='',encoding='utf-8') as f: p0=list(csv.DictReader(f,delimiter='\t'))
with open(sys.argv[2],newline='',encoding='utf-8') as f: r7=list(csv.DictReader(f,delimiter='\t'))
with open(sys.argv[3],newline='',encoding='utf-8') as f: matrix=list(csv.DictReader(f,delimiter='\t'))
if len(p0)!=20 or len(r7)!=20: raise SystemExit(f'FAIL: R3 support row count P0={len(p0)} R7={len(r7)} expected=20')
if [r['element_definition'] for r in p0] != [r['element_definition'] for r in r7]: raise SystemExit('FAIL: R3 support element order differs from P0')
c=Counter(r['r3a_disposition'] for r in r7)
expected=Counter({'IMPLEMENT_R3A_STATIC':12,'IMPLEMENT_R3A_STATIC_PHASE_AWARE':1,'IMPLEMENT_R3A_PARTIAL_INTERACTIVE_INPUT_TYPE':4,'IMPLEMENT_R3A_PARTIAL_LABELED_CONTROL':1,'IMPLEMENT_R3A_PARTIAL_CANVAS_EXCEPTIONS':1,'DEFER_R3A_NOSCRIPT_SCRIPTING_MODE':1})
if c!=expected: raise SystemExit(f'FAIL: R3A support distribution={c} expected={expected}')
r=next(x for x in matrix if x['rule_id_hex']=='0x0000000030030003')
if r['rule_symbol']!='ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS' or r['severity']!='ERROR': raise SystemExit('FAIL: F1-R7 R3 identity/severity drift')
if 'R3_IMPLEMENTATION_COMPLETE=NO' not in r['deferral_boundary'] or 'G03_GROUP_FREEZE=NO' not in r['deferral_boundary']: raise SystemExit('FAIL: F1-R7 R3 partial boundary missing')
print('VIEW0_V1N1_G03_R3A_ELEMENT_SUPPORT_LEDGER=20_OF_20')
print('VIEW0_V1N1_G03_R3A_STATIC_PARENT_COUNT=13')
print('VIEW0_V1N1_G03_R3A_PARTIAL_PARENT_COUNT=6')
print('VIEW0_V1N1_G03_R3A_DEFERRED_PARENT_COUNT=1')
PY

grep -Fq '#define ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS UINT64_C(0x0000000030030003)' "$header" || fail 'stable R3 rule ID missing'
for flag in G03_R3_PARTIAL G03_R3_DEFERRED_INPUT_TYPE G03_R3_DEFERRED_LABELED_CONTROL G03_R3_DEFERRED_CANVAS_INPUT_STATE G03_R3_DEFERRED_CANVAS_SELECT_SIZE G03_R3_DEFERRED_NOSCRIPT; do
    grep -Fq "ARBOR_VIEW0_NATIVE_RESULT_FLAG_$flag" "$header" || fail "R3 result flag missing: $flag"
done
grep -Fq '#include "g03_r3a.h"' "$native" || fail 'native checker does not integrate R3A'
grep -Fq 'arbor_view0_native_g03_r3a_measure' "$native" || fail 'R3A measurement pass missing'
grep -Fq 'arbor_view0_native_g03_r3a_collect' "$native" || fail 'R3A publication pass missing'
grep -Fq 'arbor_view0_native_g03_r1a_collect_offsets' "$r1a" || fail 'R1A private offset collection missing'
grep -Fq 'G03_R3A_MAX_DEPTH UINT64_C(4097)' "$r3a" || fail 'R3A max-depth policy missing'
grep -Fq '_Static_assert(sizeof(g03_r3a_frame) == 64u' "$r3a" || fail 'R3A frame-size assertion missing'
grep -Fq '_Static_assert(sizeof(g03_r3a_context) == 262472u' "$r3a" || fail 'R3A workspace-size assertion missing'
grep -Fq '#define G03_R3A_NOINLINE __attribute__((noinline))' "$r3a" || fail 'R3A phased-stack noinline boundary missing'
if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' "$r3a"; then fail 'R3A introduced direct Arborcore heap allocation'; fi
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 semantics appeared during R3A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    grep -Fq '#define ARBOR_VIEW_V1_G03_NOTHING_MODEL UINT64_C(0x0000000030030004)' "$header" || fail 'R4A extension rule ID missing under retained R3A'
    grep -Fq '#include "g03_r4a.h"' "$native" || fail 'R4A extension integration missing under retained R3A'
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 semantics appeared under R4A'
else
    grep -Fq '#define ARBOR_VIEW_V1_G03_NOTHING_MODEL UINT64_C(0x0000000030030004)' "$header" || fail 'R4A retained rule ID missing under R5A'
    grep -Fq '#define ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE UINT64_C(0x0000000030030005)' "$header" || fail 'R5A extension rule ID missing under retained R3A'
    grep -Fq '#include "g03_r4a.h"' "$native" || fail 'R4A retained integration missing under R5A'
    grep -Fq '#include "g03_r5a.h"' "$native" || fail 'R5A extension integration missing under retained R3A'
    grep -Fq 'arbor_view0_native_g03_r3a_collect_offsets' "$r3a" || fail 'R3A private offset-collection compatibility mode missing under R5A'
    if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]]; then
        grep -Fq '#define ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY UINT64_C(0x0000000030030007)' "$header" || fail 'R7A extension rule ID missing'
        grep -Fq '#include "g03_r7a.h"' "$native" || fail 'R7A extension integration missing'
        ! grep -ERq 'ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT|0x0000000030030006|G03_R6' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R6 runtime semantic/flag code appeared under R7A'
    else
        ! grep -ERq 'ARBOR_VIEW_V1_G03_(SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[67]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R6-R7 runtime semantics appeared before R7A'
    fi
fi
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
grep -Fq '## V1N1 G03 R3A: partial `DESCENDANT_EXCLUSIONS` evaluator' "$doc" || fail 'R3A documentation missing'

echo 'VIEW0_V1N1_G03_R3A_RULE_ID=0x0000000030030003'
echo 'VIEW0_V1N1_G03_R3A_IMPLEMENTATION_COMPLETE=NO'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    echo 'VIEW0_V1N1_G03_R3A_G03_R4_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    echo 'VIEW0_V1N1_G03_R3A_RETAINED_UNDER_G03_R4A=PASS'
    echo 'VIEW0_V1N1_G03_R3A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R5A' ]]; then
    echo 'VIEW0_V1N1_G03_R3A_RETAINED_UNDER_G03_R5A=PASS'
    echo 'VIEW0_V1N1_G03_R3A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R3A_G03_R6_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]]; then
    echo 'VIEW0_V1N1_G03_R3A_RETAINED_UNDER_G03_R6A=PASS'
    echo 'VIEW0_V1N1_G03_R3A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R3A_G03_R7_RULE_IDS_IMPLEMENTED=ZERO'
else
    echo 'VIEW0_V1N1_G03_R3A_RETAINED_UNDER_G03_R7A=PASS'
    echo 'VIEW0_V1N1_G03_R3A_PRIVATE_OFFSET_COLLECTION=PASS'
fi
echo 'VIEW0_V1N1_G03_R3A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'PASS: G03 R3A accepted source/support authorities, partial boundary, ownership, resources and no-growth contract'
