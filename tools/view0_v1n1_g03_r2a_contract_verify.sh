#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

F1_R6_BUNDLE="${VIEW0_G03_F1_R6_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-M0B-F1-R6-g03-r2a-partial-support-plan-freeze-candidate-b01e8a2647406d632b2be5697c21718e0e813e80a92274c5d3c1d51e3b23721a.tar.gz}"
P2_BUNDLE="${VIEW0_G03_R2A_P2_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-R2A-P2-residual-predicate-ownership-freeze-candidate-c646391e83a5ba4846cf481aae23d4981d6d262a93c5ada4414e59d84968d7ae.tar.gz}"
P3_BUNDLE="${VIEW0_G03_R2A_P3_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-R2A-P3-lexbor-ruby-parser-compatibility-evidence-d03ffc474a12e404bb3f3bf22c3c3304c30b27a2ce0070051f7401b175c48a62.tar.gz}"

fail() { echo "FAIL: $*" >&2; exit 1; }
need_eq() { [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }
contract='view/arborcore-view-core-1.contract'
header='tools/include/arborcore/view0_conformance/native.h'
native='tools/c/view0_conformance/native.c'
r2a='tools/c/view0_conformance/g03_r2a.c'
coverage='tests/data/view0_v1n1_g03_r2a_predicate_coverage.tsv'
doc='docs/VIEW_CORE_VIEW0.md'

current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R2A|0.1-VIEW0-V1N1-G03-R3A|0.1-VIEW0-V1N1-G03-R4A|0.1-VIEW0-V1N1-G03-R5A|0.1-VIEW0-V1N1-G03-R6A|0.1-VIEW0-V1N1-G03-R7A) ;;
  *) fail "current contract is not an accepted R2A extension: $current_version" ;;
esac
for item in \
 'VIEW0_V1N1_G03_R2A_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-R2A' \
 'VIEW0_V1N1_G03_R2A_SCOPE=PARTIAL_RESIDUAL_CONTENT_MODEL_EVALUATOR_OVER_ACCEPTED_R1A_AND_V1N0_LX1' \
 'VIEW0_V1N1_G03_R2A_PREDECESSOR_CANDIDATE_PATH_COUNT=137' \
 'VIEW0_V1N1_G03_R2A_PREDECESSOR_CANDIDATE_MANIFEST_SHA256=245e39493f6ca6f7b5fdd2dc25dd8df8f3555dc0b2dc3be28a4eb4d94a693a08' \
 'VIEW0_V1N1_G03_R2A_F1_R6_BUNDLE_SHA256=b01e8a2647406d632b2be5697c21718e0e813e80a92274c5d3c1d51e3b23721a' \
 'VIEW0_V1N1_G03_R2A_F1_R6_MATRIX_SHA256=8635b2603657d69f4f14b0130c2813bb50c31104999a37fcd16f5a185ddef568' \
 'VIEW0_V1N1_G03_R2A_F1_R6_FORM_SUPPORT_LEDGER_SHA256=1ed33eeff0ee4d376a5cc744e9d08df030d73f177c352b2ffc0860c4cd1e83d1' \
 'VIEW0_V1N1_G03_R2A_P2_BUNDLE_SHA256=c646391e83a5ba4846cf481aae23d4981d6d262a93c5ada4414e59d84968d7ae' \
 'VIEW0_V1N1_G03_R2A_P2_PREDICATE_LEDGER_SHA256=90af76b4f558ed525d2df055b394c49f05c11417d22fabff2437e85a7a83fead' \
 'VIEW0_V1N1_G03_R2A_P3_BUNDLE_SHA256=d03ffc474a12e404bb3f3bf22c3c3304c30b27a2ce0070051f7401b175c48a62' \
 'VIEW0_V1N1_G03_R2A_P3_WHATWG_RP_RT_SOURCE_SHA256=47a21ff61645cdcb0146f1f474ecef3af2dabbc90343ccaeb7eaaee374ace086' \
 'VIEW0_V1N1_G03_R2A_V1N0_LX1_COMPATIBILITY_SOURCE_MANIFEST_SHA256=e5e126ad79684b69f42a81a356c268d7cce978d0a0b3948214550683007a15e5' \
 'VIEW0_V1N1_G03_R2A_RULE_ID=0x0000000030030002' \
 'VIEW0_V1N1_G03_R2A_RULE_SYMBOL=ARBOR_VIEW_V1_G03_CONTENT_MODEL' \
 'VIEW0_V1N1_G03_R2A_SEVERITY=ERROR' \
 'VIEW0_V1N1_G03_R2A_CONTENT_MODEL_FORM_COUNT=46' \
 'VIEW0_V1N1_G03_R2A_PREDICATE_COUNT=28' \
 'VIEW0_V1N1_G03_R2A_IMPLEMENT_NOW_COUNT=18' \
 'VIEW0_V1N1_G03_R2A_EXPLICIT_DEFER_COUNT=6' \
 'VIEW0_V1N1_G03_R2A_DELEGATED_OWNER_COUNT=4' \
 'VIEW0_V1N1_G03_R2A_IMPLEMENTATION_COMPLETE=NO' \
 'VIEW0_V1N1_G03_R2A_R1_DUPLICATE_SUPPRESSION=EXPLICIT' \
 'VIEW0_V1N1_G03_R2A_DIAGNOSTIC_SCOPE=PARENT_CONTENT_MODEL' \
 'VIEW0_V1N1_G03_R2A_DIAGNOSTIC_ANCHOR=AUTHORED_PARENT_ELEMENT_START_TAG_NAME_BYTE_RANGE' \
 'VIEW0_V1N1_G03_R2A_ONE_DIAGNOSTIC_PER_FAILING_PARENT_MAX=YES' \
 'VIEW0_V1N1_G03_R2A_CAPACITY_FAILURE_ATOMICITY=TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G03_R2A_DELEGATED_R3=DESCENDANT_WIDE_EXCLUSIONS' \
 'VIEW0_V1N1_G03_R2A_DELEGATED_R4=NOTHING_MODELS' \
 'VIEW0_V1N1_G03_R2A_DELEGATED_G04=TRANSPARENT_PARENT_MODEL_DERIVATION' \
 'VIEW0_V1N1_G03_R2A_DELEGATED_G06=TIME_DATE_TIME_LEXICAL_VALIDITY' \
 'VIEW0_V1N1_G03_R2A_MAX_OBSERVATION_DEPTH_INCLUSIVE=4097' \
 'VIEW0_V1N1_G03_R2A_FRAME_SIZE_X86_64=160' \
 'VIEW0_V1N1_G03_R2A_EVALUATOR_WORKSPACE_SIZE_X86_64=655736' \
 'VIEW0_V1N1_G03_R2A_EVALUATOR_WORKSPACE_MAX_BYTES=1048576' \
 'VIEW0_V1N1_G03_R2A_EVALUATOR_WORKSPACE_STORAGE=CALL_STACK_FIXED_BOUNDED' \
 'VIEW0_V1N1_G03_R2A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G03_R2A_C0_C0_L1_RETROFIT_REQUIRED=NO' \
 'VIEW0_V1N1_G03_R2A_R1A_RETAINED=YES_PARTIAL' \
 'VIEW0_V1N1_G03_R2A_G02_GROUP_RETAINED=FROZEN_6_OF_6' \
 'VIEW0_V1N1_G03_R2A_G03_RULE_IDS_ACTIVE=2' \
 'VIEW0_V1N1_G03_R2A_G03_R3_R7_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_R2A_G03_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G03_R2A_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G03_R2A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G03_R2A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
    grep -Fxq "$item" "$contract" || fail "missing R2A contract marker: $item"
done

for spec in \
 "$F1_R6_BUNDLE|b01e8a2647406d632b2be5697c21718e0e813e80a92274c5d3c1d51e3b23721a|F1-R6" \
 "$P2_BUNDLE|c646391e83a5ba4846cf481aae23d4981d6d262a93c5ada4414e59d84968d7ae|R2A-P2" \
 "$P3_BUNDLE|d03ffc474a12e404bb3f3bf22c3c3304c30b27a2ce0070051f7401b175c48a62|R2A-P3"; do
    IFS='|' read -r f h label <<< "$spec"
    [[ -f "$f" ]] || fail "missing $label bundle: $f"
    need_eq "$(sha256sum "$f" | awk '{print $1}')" "$h" "$label SHA-256"
done

tmp=$(mktemp -d /tmp/arborcore-g03-r2a-contract.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/r6" "$tmp/p2" "$tmp/p3"
tar -xzf "$F1_R6_BUNDLE" -C "$tmp/r6"
tar -xzf "$P2_BUNDLE" -C "$tmp/p2"
tar -xzf "$P3_BUNDLE" -C "$tmp/p3"
r6="$tmp/r6/Arborcore-VIEW0-V1N1-M0B-F1-R6-g03-r2a-partial-support-plan-freeze-candidate"
p2="$tmp/p2/Arborcore-VIEW0-V1N1-G03-R2A-P2-residual-predicate-ownership-freeze-candidate"
p3="$tmp/p3/Arborcore-VIEW0-V1N1-G03-R2A-P3-lexbor-ruby-parser-compatibility-evidence"
(cd "$r6" && sha256sum -c f1-r6-manifest.sha256 >/dev/null)
(cd "$p2" && sha256sum -c r2a-p2-manifest.sha256 >/dev/null)
(cd "$p3" && sha256sum -c r2a-p3-manifest.sha256 >/dev/null)
need_eq "$(sha256sum "$r6/f1-final-rule-spec-test-matrix.tsv" | awk '{print $1}')" '8635b2603657d69f4f14b0130c2813bb50c31104999a37fcd16f5a185ddef568' 'F1-R6 matrix'
need_eq "$(sha256sum "$r6/f1-r6-r2a-form-support-plan.tsv" | awk '{print $1}')" '1ed33eeff0ee4d376a5cc744e9d08df030d73f177c352b2ffc0860c4cd1e83d1' 'F1-R6 support ledger'
need_eq "$(sha256sum "$p2/r2a-residual-predicate-ledger.tsv" | awk '{print $1}')" '90af76b4f558ed525d2df055b394c49f05c11417d22fabff2437e85a7a83fead' 'P2 predicate ledger'
need_eq "$(sha256sum "$coverage" | awk '{print $1}')" '90af76b4f558ed525d2df055b394c49f05c11417d22fabff2437e85a7a83fead' 'repository predicate coverage ledger'
cmp -s "$p2/r2a-residual-predicate-ledger.tsv" "$coverage" || fail 'repository predicate coverage differs from accepted P2'
need_eq "$(sha256sum "$p3/whatwg-rp-rt-parser.whatwg-source" | awk '{print $1}')" '47a21ff61645cdcb0146f1f474ecef3af2dabbc90343ccaeb7eaaee374ace086' 'P3 WHATWG ruby parser slice'

python3 - "$coverage" "$r6/f1-r6-r2a-form-support-plan.tsv" <<'PY'
import csv,sys
from collections import Counter
with open(sys.argv[1],newline='',encoding='utf-8') as f: p=list(csv.DictReader(f,delimiter='\t'))
with open(sys.argv[2],newline='',encoding='utf-8') as f: forms=list(csv.DictReader(f,delimiter='\t'))
if len(p)!=28: raise SystemExit(f'FAIL: R2A predicate count={len(p)} expected=28')
if [r['predicate_id'] for r in p] != [f'R2A-P{i:03d}' for i in range(1,29)]: raise SystemExit('FAIL: R2A predicate IDs/order drift')
c=Counter(r['status'] for r in p)
if sum(v for k,v in c.items() if k.startswith('IMPLEMENT_R2A'))!=18: raise SystemExit(f'FAIL: implement count={c}')
if sum(v for k,v in c.items() if k.startswith('DEFER_R2A'))!=6: raise SystemExit(f'FAIL: defer count={c}')
if sum(v for k,v in c.items() if k.startswith('DELEGATE_'))!=4: raise SystemExit(f'FAIL: delegate count={c}')
if len(forms)!=46: raise SystemExit(f'FAIL: F1-R6 form count={len(forms)} expected=46')
print('VIEW0_V1N1_G03_R2A_PREDICATE_LEDGER=28_OF_28')
print('VIEW0_V1N1_G03_R2A_IMPLEMENT_NOW_COUNT=18')
print('VIEW0_V1N1_G03_R2A_EXPLICIT_DEFER_COUNT=6')
print('VIEW0_V1N1_G03_R2A_DELEGATED_OWNER_COUNT=4')
PY

grep -Fq '#define ARBOR_VIEW_V1_G03_CONTENT_MODEL UINT64_C(0x0000000030030002)' "$header" || fail 'stable R2 rule ID missing'
for flag in G03_R2_PARTIAL G03_R2_DEFERRED_STYLE G03_R2_DEFERRED_SCRIPT G03_R2_DEFERRED_NOSCRIPT G03_R2_DEFERRED_SELECT_SIZE G03_R2_DEFERRED_SELECT_PLATFORM G03_R2_DEFERRED_UNCLASSIFIED; do
    grep -Fq "ARBOR_VIEW0_NATIVE_RESULT_FLAG_$flag" "$header" || fail "R2 result flag missing: $flag"
done
grep -Fq '#include "g03_r2a.h"' "$native" || fail 'native checker does not integrate R2A'
grep -Fq 'arbor_view0_native_g03_r2a_measure' "$native" || fail 'R2A measurement pass missing'
grep -Fq 'arbor_view0_native_g03_r2a_collect' "$native" || fail 'R2A publication pass missing'
grep -Fq 'G03_R2A_MAX_DEPTH UINT64_C(4097)' "$r2a" || fail 'R2A max-depth policy missing'
grep -Fq '_Static_assert(sizeof(g03_r2a_frame) == 160u' "$r2a" || fail 'R2A frame-size assertion missing'
grep -Fq '_Static_assert(sizeof(g03_r2a_context) == 655736u' "$r2a" || fail 'R2A workspace-size assertion missing'
if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' "$r2a"; then fail 'R2A introduced direct Arborcore heap allocation'; fi
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R2A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[3-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R3-R7 semantics appeared during R2A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    grep -Fq '#define ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS UINT64_C(0x0000000030030003)' "$header" || fail 'R3A extension rule ID missing under retained R2A'
    grep -Fq '#include "g03_r3a.h"' "$native" || fail 'R3A extension integration missing under retained R2A'
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 semantics appeared under R3A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    grep -Fq '#define ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS UINT64_C(0x0000000030030003)' "$header" || fail 'R3A retained rule ID missing under R4A'
    grep -Fq '#define ARBOR_VIEW_V1_G03_NOTHING_MODEL UINT64_C(0x0000000030030004)' "$header" || fail 'R4A extension rule ID missing under retained R2A'
    grep -Fq '#include "g03_r3a.h"' "$native" || fail 'R3A retained integration missing under R4A'
    grep -Fq '#include "g03_r4a.h"' "$native" || fail 'R4A extension integration missing under retained R2A'
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 semantics appeared under R4A'
else
    for spec in \
      'ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS|0x0000000030030003|R3A' \
      'ARBOR_VIEW_V1_G03_NOTHING_MODEL|0x0000000030030004|R4A' \
      'ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE|0x0000000030030005|R5A'; do
        IFS='|' read -r symbol id label <<<"$spec"
        grep -Fq "#define $symbol UINT64_C($id)" "$header" || fail "$label retained/extension rule ID missing under R5A"
    done
    for r in r3a r4a r5a; do grep -Fq "#include \"g03_${r}.h\"" "$native" || fail "$r integration missing under R5A"; done
    grep -Fq 'arbor_view0_native_g03_r2a_collect_offsets' tools/c/view0_conformance/g03_r2a.c || fail 'R2A private offset-collection compatibility mode missing under R5A'
    if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]]; then
        grep -Fq '#define ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY UINT64_C(0x0000000030030007)' "$header" || fail 'R7A extension rule ID missing'
        grep -Fq '#include "g03_r7a.h"' "$native" || fail 'R7A extension integration missing'
        ! grep -ERq 'ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT|0x0000000030030006|G03_R6' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R6 runtime semantic/flag code appeared under R7A'
    else
        ! grep -ERq 'ARBOR_VIEW_V1_G03_(SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[67]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R6-R7 runtime semantics appeared before R7A'
    fi
fi
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
grep -Fq '## V1N1 G03 R2A: partial residual CONTENT_MODEL evaluator' "$doc" || fail 'R2A documentation missing'

echo 'VIEW0_V1N1_G03_R2A_RULE_ID=0x0000000030030002'
echo 'VIEW0_V1N1_G03_R2A_IMPLEMENTATION_COMPLETE=NO'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R2A' ]]; then
    echo 'VIEW0_V1N1_G03_R2A_G03_R3_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    echo 'VIEW0_V1N1_G03_R2A_RETAINED_UNDER_G03_R3A=PASS'
    echo 'VIEW0_V1N1_G03_R2A_G03_R4_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    echo 'VIEW0_V1N1_G03_R2A_RETAINED_UNDER_G03_R4A=PASS'
    echo 'VIEW0_V1N1_G03_R2A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R5A' ]]; then
    echo 'VIEW0_V1N1_G03_R2A_RETAINED_UNDER_G03_R5A=PASS'
    echo 'VIEW0_V1N1_G03_R2A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R2A_G03_R6_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]]; then
    echo 'VIEW0_V1N1_G03_R2A_RETAINED_UNDER_G03_R6A=PASS'
    echo 'VIEW0_V1N1_G03_R2A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R2A_G03_R7_RULE_IDS_IMPLEMENTED=ZERO'
else
    echo 'VIEW0_V1N1_G03_R2A_RETAINED_UNDER_G03_R7A=PASS'
    echo 'VIEW0_V1N1_G03_R2A_PRIVATE_OFFSET_COLLECTION=PASS'
fi
echo 'VIEW0_V1N1_G03_R2A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'PASS: G03 R2A accepted support/predicate authorities, partial boundary, resources and no-growth contract'
