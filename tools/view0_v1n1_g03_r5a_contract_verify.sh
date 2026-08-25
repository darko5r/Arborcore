#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
F1_R10="${VIEW0_G03_F1_R10_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-M0B-F1-R10-g03-r5a-actual-foster-applicability-correction-freeze-candidate-4f782ef8aad6086bea3133b9301ff0a6f6282b3bc7d0cc9f401bde001797d7e9.tar.gz}"
F1_R10_SHA='4f782ef8aad6086bea3133b9301ff0a6f6282b3bc7d0cc9f401bde001797d7e9'
F1_R10_MANIFEST_SHA='c15ba3c2bdcee8ce52f8e1d0f87bcafcd5babc147ef57ae90afb44521fee1691'
F1_R10_MATRIX_SHA='ac3c5ba1964106abfeda41a9d39becd3d0ea142e80f4602f14b87c6346a2234c'
F1_R10_LEDGER_SHA='dc11120c60490ade9e3a64d1bf9f8ab75c6aa777e23b1f96a76b62df5c6ed43b'
F1_R10_CONTROL_SHA='d5286b456cd3f7d22916ff6ac13e1bbd0db78dc95f6dc41f0e8cc7d45c428ce2'
fail(){ echo "FAIL: $*" >&2; exit 1; }; need_eq(){ [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }
contract=view/arborcore-view-core-1.contract
header=tools/include/arborcore/view0_conformance/native.h
native=tools/c/view0_conformance/native.c
r5=tools/c/view0_conformance/g03_r5a.c
coverage=tests/data/view0_v1n1_g03_r5a_repair_class_support.tsv
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R5A|0.1-VIEW0-V1N1-G03-R6A|0.1-VIEW0-V1N1-G03-R7A) ;;
  *) fail "current contract is not an accepted R5A extension: $current_version" ;;
esac
for item in \
 'VIEW0_V1N1_G03_R5A_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-R5A' \
 'VIEW0_V1N1_G03_R5A_PREDECESSOR_CANDIDATE_PATH_COUNT=174' \
 'VIEW0_V1N1_G03_R5A_PREDECESSOR_CANDIDATE_MANIFEST_SHA256=97ebdbe068f058aa4ca1a3b6395135d8d33daa87b443952246a3b5c7c5b224fa' \
 'VIEW0_V1N1_G03_R5A_RULE_ID=0x0000000030030005' \
 'VIEW0_V1N1_G03_R5A_REPAIR_CLASS_COUNT=9' \
 'VIEW0_V1N1_G03_R5A_IMPLEMENT_NOW_CLASS_COUNT=1' \
 'VIEW0_V1N1_G03_R5A_IMPLEMENT_NOW_CLASS=FOSTER_PARENTED_SOURCE_STANDARD_START_TAG' \
 'VIEW0_V1N1_G03_R5A_REQUIRED_PREDICATE=STANDARD_ELEMENT_NONZERO_AND_INSERTION_SEEN_1_AND_FOSTER_PARENTING_BIT_SET_AND_INSERTION_CURRENT_STANDARD_ELEMENT_IN_TABLE_TBODY_TFOOT_THEAD_TR' \
 'VIEW0_V1N1_G03_R5A_ACTUAL_FOSTER_TARGET_SET=TABLE_TBODY_TFOOT_THEAD_TR' \
 'VIEW0_V1N1_G03_R5A_FOSTER_FLAG_ALONE_ACTUAL_FOSTER_PROOF=NO' \
 'VIEW0_V1N1_G03_R5A_FOREIGNOBJECT_P_APPLICABILITY_CONTROL=NO_R5_ERROR' \
 'VIEW0_V1N1_G03_R5A_CONTROL_CLASS_COUNT=2' \
 'VIEW0_V1N1_G03_R5A_DEFERRED_CLASS_COUNT=4' \
 'VIEW0_V1N1_G03_R5A_CUSTOM_ELEMENTS=DELEGATE_G13' \
 'VIEW0_V1N1_G03_R5A_PRIOR_ERROR_OFFSET_CAPACITY=4096' \
 'VIEW0_V1N1_G03_R5A_PRIOR_ERROR_OFFSET_SCRATCH_BYTES=32768' \
 'VIEW0_V1N1_G03_R5A_PRIOR_ERROR_OFFSET_LOOKUP=DETERMINISTIC_BOUNDED_LINEAR_SCAN' \
 'VIEW0_V1N1_G03_R5A_EVALUATE_COMPILED_STACK_BOUND_BYTES=40000' \
 'VIEW0_V1N1_G03_R5A_C0_SR1_CHANGE_REQUIRED=NO' \
 'VIEW0_V1N1_G03_R5A_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G03_R5A_PRODUCTION_VIEW_API_GROWTH=NO' \
 'VIEW0_V1N1_G03_R5A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G03_R5A_G03_RULE_IDS_ACTIVE=5' \
 'VIEW0_V1N1_G03_R5A_RETAINED_R1_R4_CONTRACT_VERIFIERS=PASS_EXTENSION_AWARE' \
 'VIEW0_V1N1_G03_R5A_SUCCESS_RESULT_ACTIVE_PARTIAL_FLAGS=R1_R2_R3_R4_R5_ON_ALL_SUCCESS_PATHS_INCLUDING_UTF8_INVALID_AND_EMPTY_INPUT' \
 'VIEW0_V1N1_G03_R5A_G03_R6_R7_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_R5A_IMPLEMENTATION_COMPLETE=NO' \
 'VIEW0_V1N1_G03_R5A_G03_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G03_R5A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fxq "$item" "$contract" || fail "R5A contract marker missing: $item"
done
! grep -Fq 'VIEW0_V1N1_G03_R5A_EVALUATE_COMPILED_STACK_BYTES=' "$contract" || fail 'environment-specific exact R5A stack observation must not be contractual'
[[ -f "$F1_R10" ]] || fail "missing F1-R10 bundle: $F1_R10"
need_eq "$(sha256sum "$F1_R10" | awk '{print $1}')" "$F1_R10_SHA" 'F1-R10 SHA-256'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
python3 - "$F1_R10" <<'PY'
import pathlib,sys,tarfile
p=pathlib.Path(sys.argv[1])
with tarfile.open(p,'r:gz') as tf:
    ms=tf.getmembers()
    if not ms: raise SystemExit('FAIL: F1-R10 archive empty')
    for m in ms:
        q=pathlib.PurePosixPath(m.name)
        if not m.name or m.name.startswith('/') or q.is_absolute() or '..' in q.parts:
            raise SystemExit(f'FAIL: unsafe F1-R10 path {m.name!r}')
        if not (m.isdir() or m.isreg()):
            raise SystemExit(f'FAIL: special F1-R10 member {m.name!r}')
PY
tar -xzf "$F1_R10" -C "$tmp"
f1="$tmp/Arborcore-VIEW0-V1N1-M0B-F1-R10-g03-r5a-actual-foster-applicability-correction-freeze-candidate"
[[ -d "$f1" ]] || fail 'F1-R10 root directory missing'
need_eq "$(sha256sum "$f1/f1-r10-manifest.sha256" | awk '{print $1}')" "$F1_R10_MANIFEST_SHA" 'F1-R10 internal manifest'
(cd "$f1" && sha256sum -c f1-r10-manifest.sha256 >/dev/null)
need_eq "$(sha256sum "$f1/f1-final-rule-spec-test-matrix.tsv" | awk '{print $1}')" "$F1_R10_MATRIX_SHA" 'F1-R10 matrix'
need_eq "$(sha256sum "$f1/f1-r10-r5a-repair-class-support-plan.tsv" | awk '{print $1}')" "$F1_R10_LEDGER_SHA" 'F1-R10 support ledger'
need_eq "$(sha256sum "$f1/f1-r10-applicability-control.tsv" | awk '{print $1}')" "$F1_R10_CONTROL_SHA" 'F1-R10 applicability control'
cmp -s "$coverage" "$f1/f1-r10-r5a-repair-class-support-plan.tsv" || fail 'installed R5A support ledger differs from accepted F1-R10'
python3 - "$coverage" "$f1/f1-final-rule-spec-test-matrix.tsv" "$f1/f1-r10-applicability-control.tsv" <<'PY'
import csv,sys
ledger=list(csv.DictReader(open(sys.argv[1],encoding='utf-8',newline=''),delimiter='\t'))
if len(ledger)!=9: raise SystemExit('FAIL: R5A ledger not 9 rows')
counts={}
for r in ledger: counts[r['f1_r10_disposition']]=counts.get(r['f1_r10_disposition'],0)+1
expected={'IMPLEMENT_R5A_FOSTER_PARENTED_SOURCE_STANDARD_START_TAG':1,'SUPPRESS_R5A_SAME_SOURCE_ANCHOR':1,'CONTROL_NO_R5':2,
          'DEFER_R5_NONFOSTER_REPROCESSING':1,'DEFER_R5_NO_INSERTION':1,
          'DEFER_R5_NONFOSTER_PARSER_REPAIR':1,'DEFER_R5_TABLE_SPECIAL_REPAIR':1,'DELEGATE_G13':1}
if counts!=expected: raise SystemExit(f'FAIL: R5A ledger dispositions {counts!r}')
r5=[r for r in csv.DictReader(open(sys.argv[2],encoding='utf-8',newline=''),delimiter='\t') if r['rule_id_hex']=='0x0000000030030005']
if len(r5)!=1 or r5[0]['rule_symbol']!='ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE' or r5[0]['severity']!='ERROR':
    raise SystemExit('FAIL: frozen F1-R10 R5 identity drift')
if r5[0]['diagnostic_anchor']!='element/not-explicitly-allowed': raise SystemExit('FAIL: frozen R5 diagnostic anchor drift')
r1=next(r for r in ledger if r['class_id']=='R5C001')
if r1['f1_r10_actual_foster_target_gate']!='REQUIRE_INSERTION_CURRENT_STANDARD_ELEMENT_IN_TABLE_TBODY_TFOOT_THEAD_TR':
    raise SystemExit('FAIL: F1-R10 actual foster target gate drift')
controls=list(csv.DictReader(open(sys.argv[3],encoding='utf-8',newline=''),delimiter='\t'))
if len(controls)!=1 or controls[0]['f1_r10_decision']!='CONTROL_NO_R5_ERROR':
    raise SystemExit('FAIL: F1-R10 applicability control drift')
print('VIEW0_V1N1_G03_R5A_F1_R10_STRUCTURED_BOUNDARY=PASS')
PY
need_eq "$(sha256sum "$coverage" | awk '{print $1}')" "$F1_R10_LEDGER_SHA" 'installed R5A support ledger SHA'
grep -Fq '#define ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE UINT64_C(0x0000000030030005)' "$header" || fail 'R5 rule ID missing'
grep -Fq '#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL UINT64_C(0x40000)' "$header" || fail 'R5 partial flag missing'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]]; then
    grep -Fq '#define ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY UINT64_C(0x0000000030030007)' "$header" || fail 'R7A extension rule ID missing under retained R5A'
    ! grep -Eq 'ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT|0x0000000030030006|G03_R6' "$header" || fail 'R6 runtime semantic/flag code appeared under R7A'
else
    ! grep -Eq 'ARBOR_VIEW_V1_G03_(SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[67]' "$header" || fail 'R6/R7 runtime semantics appeared before R7A'
fi
grep -Fq '#include "g03_r5a.h"' "$native" || fail 'native checker missing R5A integration'
grep -Fq 'arbor_view0_native_g03_r5a_measure' "$native" || fail 'R5A measure missing'
grep -Fq 'arbor_view0_native_g03_r5a_collect' "$native" || fail 'R5A collect missing'
grep -Fq 'ARBOR_VIEW0_NATIVE_SOURCE_REPAIR_FLAG_FOSTER_PARENTING' "$r5" || fail 'R5A foster state predicate missing'
grep -Fq 'insertion_target_is_actual_foster_family' "$r5" || fail 'R5A actual-foster target gate missing'
for e in TABLE TBODY TFOOT THEAD TR; do grep -Fq "ARBOR_VIEW0_NATIVE_ELEMENT_$e" "$r5" || fail "R5A target family missing $e"; done
grep -Fq 'prior_owner_at_offset' "$r5" || fail 'R5A prior-owner suppression missing'
grep -Fq 'G03_R5A_PRIOR_OFFSET_CAPACITY UINT64_C(4096)' "$r5" || fail 'R5A prior-offset bound missing'
grep -Fq '_Static_assert(sizeof(g03_r5a_context) == 64u' "$r5" || fail 'R5A context layout assertion missing'
for r in r2a r3a r4a; do grep -Fq "arbor_view0_native_g03_${r}_collect_offsets" "tools/c/view0_conformance/g03_${r}.c" || fail "$r private offset collection missing"; done
if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' "$r5"; then fail 'R5A direct heap allocation introduced'; fi
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
grep -Fq 'g03_r5=partial' tools/c/view0_conformance/main.c || fail 'CLI R5 partial marker missing'
echo 'VIEW0_V1N1_G03_R5A_SUPPORT_LEDGER=9_OF_9'
echo 'VIEW0_V1N1_G03_R5A_IMPLEMENTABLE_CLASS_COUNT=1'
echo 'VIEW0_V1N1_G03_R5A_DEFERRED_CLASS_COUNT=4'
echo 'VIEW0_V1N1_G03_R5A_CONTROL_CLASS_COUNT=2'
echo 'VIEW0_V1N1_G03_R5A_SAME_ANCHOR_SUPPRESSION_CLASS_COUNT=1'
echo 'VIEW0_V1N1_G03_R5A_G13_DELEGATED_CLASS_COUNT=1'
echo 'VIEW0_V1N1_G03_R5A_ACTUAL_FOSTER_TARGET_SET=TABLE_TBODY_TFOOT_THEAD_TR'
echo 'VIEW0_V1N1_G03_R5A_FOSTER_FLAG_ALONE_ACTUAL_FOSTER_PROOF=NO'
printf '%s\n' '### RETAINED R1A-R4A CONTRACT EXTENSION-AWARENESS'
retained_stage=R5A
[[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]] && retained_stage=R6A
[[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]] && retained_stage=R7A
for rule in r1a r2a r3a r4a; do
    marker="VIEW0_V1N1_G03_${rule^^}_RETAINED_UNDER_G03_${retained_stage}=PASS"
    retained_output=$(bash "tools/view0_v1n1_g03_${rule}_contract_verify.sh")
    grep -Fxq "$marker" <<<"$retained_output" || fail "retained ${rule^^} contract verifier did not publish ${retained_stage} extension marker"
done
echo 'VIEW0_V1N1_G03_R5A_RETAINED_R1_R4_CONTRACT_VERIFIERS=PASS'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]]; then
    echo 'VIEW0_V1N1_G03_R5A_RETAINED_UNDER_G03_R6A=PASS'
    echo 'VIEW0_V1N1_G03_R5A_G03_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]]; then
    echo 'VIEW0_V1N1_G03_R5A_RETAINED_UNDER_G03_R7A=PASS'
fi
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'PASS: G03 R5A exact F1-R10 actual-foster support boundary, ownership, resources and no-growth contract'
