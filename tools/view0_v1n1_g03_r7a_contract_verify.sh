#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
F1_R13="${VIEW0_G03_F1_R13_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-M0B-F1-R13-g03-r7-fixture-isolation-correction-freeze-candidate-c1fc115990c7aad23feecbc3590a8867130f36d6aacf67d77e441c49367e8ec7.tar.gz}"
SR1_DESIGN="${VIEW0_G03_R7A_SR1_DESIGN_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-R7A-SR1-global-failure-atomicity-correction-design-freeze-candidate-18865304b9b1ca55959eca63fc812604f1dedda372302042b6f7179a15ef3cc3.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }
need_eq(){ [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }
contract=view/arborcore-view-core-1.contract
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)
[[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]] || fail "current contract is not R7A: $current_version"
for item in \
 'VIEW0_V1N1_G03_R7A_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-R7A' \
 'VIEW0_V1N1_G03_R7A_PREDECESSOR_CANDIDATE_PATH_COUNT=194' \
 'VIEW0_V1N1_G03_R7A_PREDECESSOR_CANDIDATE_MANIFEST_SHA256=6d874531e18f1da16a58fd03ac06a0867a745d0764b681f364e9dfa91be46cf9' \
 'VIEW0_V1N1_G03_R7A_CUMULATIVE_CANDIDATE_PATH_COUNT=208' \
 'VIEW0_V1N1_G03_R7A_FINAL_DELTA_PATH_COUNT=34' \
 'VIEW0_V1N1_G03_R7A_FINAL_DELTA_NEW_PATH_COUNT=14' \
 'VIEW0_V1N1_G03_R7A_FINAL_DELTA_REPLACEMENT_PATH_COUNT=20' \
 'VIEW0_V1N1_G03_R7A_FINAL_DELTA_DELETION_PATH_COUNT=0' \
 'VIEW0_V1N1_G03_R7A_R7_P0_BUNDLE_SHA256=fb7ca5b4ae3cefabcb7a7e3f76ad56ac9e1a9e965bae16b4a3bd828e3af1a05d' \
 'VIEW0_V1N1_G03_R7A_F1_R12_BUNDLE_SHA256=f205924d6d6bdbf5d8e1b9b3c01bd472e457bf10ecd6b01f810a27f76b3f4706' \
 'VIEW0_V1N1_G03_R7A_F1_R13_BUNDLE_SHA256=c1fc115990c7aad23feecbc3590a8867130f36d6aacf67d77e441c49367e8ec7' \
 'VIEW0_V1N1_G03_R7A_F1_R12_MATRIX_SHA256=541a951ddaf24ef5a50bfbb4f19804bd3a60f20f8b6a3bed45216c61b6b125ad' \
 'VIEW0_V1N1_G03_R7A_F1_R13_FIXTURE_PLAN_SHA256=521152cca7b068770910345e06b2b27fcde855ea4f5e22bf76bafc315a726fa2' \
 'VIEW0_V1N1_G03_R7A_RULE_ID=0x0000000030030007' \
 'VIEW0_V1N1_G03_R7A_RULE_SYMBOL=ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY' \
 'VIEW0_V1N1_G03_R7A_SEVERITY=WARNING' \
 'VIEW0_V1N1_G03_R7A_DIAGNOSTIC_ANCHOR=AUTHORED_SUBJECT_SOURCE_START' \
 'VIEW0_V1N1_G03_R7A_DIRECT_DOM_CHILD_RELATION=REQUIRED' \
 'VIEW0_V1N1_G03_R7A_HIDDEN_PALPABLE_CHILD_SATISFIES=NO' \
 'VIEW0_V1N1_G03_R7A_INTER_ELEMENT_WHITESPACE_TEXT_SATISFIES=NO' \
 'VIEW0_V1N1_G03_R7A_SUBJECT_SUPPORT_PLAN_SHA256=4c87be77fbc8fc8a12c5bfb17dd413d758db3bc4d70099dc4f37fc7ac8607c2d' \
 'VIEW0_V1N1_G03_R7A_PALPABLE_CHILD_SUPPORT_PLAN_SHA256=ade32df3b7f309db13bc0c2e1673757bb7359cc195724b4ec6b6c9548b9f9233' \
 'VIEW0_V1N1_G03_R7A_DIAGNOSTIC_PLAN_SHA256=92967d4fe2788aff6b79156cc830c432f456ba17c0b744c134148934cc995f14' \
 'VIEW0_V1N1_G03_R7A_SOURCE_BOUNDARY_SHA256=4fb9cc308154c9f79753c1a75ceca727372030fd62ecb359f8b831e7d44b8819' \
 'VIEW0_V1N1_G03_R7A_STANDARD_SUBJECT_ELEMENTS=65' \
 'VIEW0_V1N1_G03_R7A_STABLE_STANDARD_SUBJECT_ELEMENTS=62' \
 'VIEW0_V1N1_G03_R7A_ACTIVE_BRANCH_SENSITIVE_SUBJECT_ELEMENTS=3' \
 'VIEW0_V1N1_G03_R7A_PALPABLE_CATEGORY=77_OF_77' \
 'VIEW0_V1N1_G03_R7A_STANDARD_UNCONDITIONAL_PALPABLE_ELEMENTS=67' \
 'VIEW0_V1N1_G03_R7A_STANDARD_CONDITIONAL_PALPABLE_ELEMENTS=6' \
 'VIEW0_V1N1_G03_R7A_FOREIGN_PALPABLE_ELEMENTS=2' \
 'VIEW0_V1N1_G03_R7A_FIXTURE_PLAN=18_OF_18' \
 'VIEW0_V1N1_G03_R7A_CONDITIONAL_PALPABILITY_CONTROLS=10_OF_10' \
 'VIEW0_V1N1_G03_R7A_G04_TRANSPARENT_DEPENDENCY=DEFER_NO_WARNING' \
 'VIEW0_V1N1_G03_R7A_G13_CUSTOM_DEPENDENCY=DEFER_NO_WARNING' \
 'VIEW0_V1N1_G03_R7A_NEW_SEMANTIC_EVALUATOR=YES' \
 'VIEW0_V1N1_G03_R7A_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G03_R7A_C0_SR1_RETROFIT_REQUIRED=NO' \
 'VIEW0_V1N1_G03_R7A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G03_R7A_RESULT_FLAG_PARTIAL=0x0000000000080000' \
 'VIEW0_V1N1_G03_R7A_RESULT_FLAG_G04_DEFERRED=0x0000000000100000' \
 'VIEW0_V1N1_G03_R7A_RESULT_FLAG_G13_DEFERRED=0x0000000000200000' \
 'VIEW0_V1N1_G03_R7A_EVALUATOR_WORKSPACE_BOUND_BYTES=1048576' \
 'VIEW0_V1N1_G03_R7A_EVALUATE_COMPILED_STACK_BOUND_BYTES=400000' \
 'VIEW0_V1N1_G03_R7A_PHASED_COMPILED_STACK_BOUND_BYTES=900000' \
 'VIEW0_V1N1_G03_R7A_FAILURE_ATOMICITY=GLOBAL_CHECKER_TWO_PASS' \
 'VIEW0_V1N1_G03_R7A_SR2_CORRECTION_DESIGN_BUNDLE_SHA256=18865304b9b1ca55959eca63fc812604f1dedda372302042b6f7179a15ef3cc3' \
 'VIEW0_V1N1_G03_R7A_SR2_PREDECESSOR_PATH_COUNT=208' \
 'VIEW0_V1N1_G03_R7A_SR2_PREDECESSOR_PATHLIST_SHA256=2d2f67c3b7c98e962624f84685fb4333bccde3fdfb2bf797b754bcacfed539f3' \
 'VIEW0_V1N1_G03_R7A_SR2_PREDECESSOR_MANIFEST_SHA256=490f033f34d011ba6cc1f97b20dbe4a5b34a1647130f65d902c2ff3438a9e3ed' \
 'VIEW0_V1N1_G03_R7A_SR2_CUMULATIVE_CANDIDATE_PATH_COUNT=209' \
 'VIEW0_V1N1_G03_R7A_SR2_CANDIDATE_PATHLIST_SHA256=72a24bcdc8848c5c87ad85036b76c50c3390614851c24583ba0f265dde4fadc7' \
 'VIEW0_V1N1_G03_R7A_SR2_ANCHOR_RECORD_BYTES=8' \
 'VIEW0_V1N1_G03_R7A_SR2_ANCHOR_WORKSPACE_BYTES=32768' \
 'VIEW0_V1N1_G03_R7A_SR2_LAST_FALLIBLE_OPERATION=EXACT_FAILURE_ATOMIC_LEXBOR_PARSE_PUBLICATION' \
 'VIEW0_V1N1_G03_R7A_SR2_GLOBAL_MECHANISM_FAILURE_ATOMICITY=REQUIRED' \
 'VIEW0_V1N1_G03_R7A_SR2_ANCHOR_EQUIVALENCE=R1_R5_R7_REQUIRED' \
 'VIEW0_V1N1_G03_R7A_SR2_EXACT_CLI_ANCHOR=OFFSET_38_LENGTH_1_LINE_1_COLUMN_39' \
 'VIEW0_V1N1_G03_R7A_SR2_R1_R7_SEMANTICS_CHANGE=NO' \
 'VIEW0_V1N1_G03_R7A_SR2_G04_G13_CHANGE=NO' \
 'VIEW0_V1N1_G03_R7A_SR2_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G03_R7A_SR2_LEXBOR_SOURCE_MUTATION=NO' \
 'VIEW0_V1N1_G03_R7A_SR2_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G03_R7A_SR2_PRODUCTION_VIEW_API_CHANGE=NO' \
 'VIEW0_V1N1_G03_R7A_SR2_STACK_THRESHOLD_WIDENING=NO' \
 'VIEW0_V1N1_G03_R7A_RETAINED_FIXTURE_ISOLATION=8_OF_8' \
 'VIEW0_V1N1_G03_R7A_RETAINED_FIXTURE_ISOLATION_SHA256=078b2743d88b57af8528d56ac85610553172528c8a3d8b7af5063abd6beb043d' \
 'VIEW0_V1N1_G03_R7A_RETAINED_NATIVE_RULE_OBJECT_CLEAN_RESET=9_OF_9' \
 'VIEW0_V1N1_G03_R7A_SCOPE_IDENTITY_COLLATION=LC_ALL_C' \
 'VIEW0_V1N1_G03_R7A_RETAINED_R1_R6_CONTRACT_VERIFIERS=PASS_EXTENSION_AWARE' \
 'VIEW0_V1N1_G03_R7A_G03_RULE_IDS_CONSTRUCTED=7_OF_7' \
 'VIEW0_V1N1_G03_R7A_SUCCESS_RESULT_ACTIVE_PARTIAL_FLAGS=R1_R2_R3_R4_R5_R7_ON_ALL_SUCCESS_PATHS_INCLUDING_UTF8_INVALID_AND_EMPTY_INPUT' \
 'VIEW0_V1N1_G03_R7A_IMPLEMENTATION_COMPLETE=NO_G04_G13_DEFERRED' \
 'VIEW0_V1N1_G03_R7A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G03_R7A_G03_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G03_R7A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fxq "$item" "$contract" || fail "missing R7A contract marker: $item"
done

[[ -f "$F1_R13" ]] || fail "missing F1-R13: $F1_R13"
need_eq "$(sha256sum "$F1_R13"|awk '{print $1}')" 'c1fc115990c7aad23feecbc3590a8867130f36d6aacf67d77e441c49367e8ec7' 'F1-R13 archive'
tmp=$(mktemp -d /tmp/arborcore-r7a-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
[[ -f "$SR1_DESIGN" ]] || fail "missing accepted SR1 design archive: $SR1_DESIGN"
need_eq "$(sha256sum "$SR1_DESIGN"|awk '{print $1}')" '18865304b9b1ca55959eca63fc812604f1dedda372302042b6f7179a15ef3cc3' 'SR1 design archive'
python3 - "$SR1_DESIGN" <<'PY'
import pathlib,sys,tarfile
p=pathlib.Path(sys.argv[1])
with tarfile.open(p,'r:gz') as tf:
    ms=tf.getmembers()
    if not ms: raise SystemExit('FAIL: SR1 design archive empty')
    for m in ms:
        q=pathlib.PurePosixPath(m.name)
        if not m.name or m.name.startswith('/') or q.is_absolute() or '..' in q.parts:
            raise SystemExit(f'FAIL: unsafe SR1 design path: {m.name!r}')
        if not (m.isdir() or m.isreg()):
            raise SystemExit(f'FAIL: unsafe SR1 design member type: {m.name!r}')
PY
mkdir "$tmp/sr1"
tar -xzf "$SR1_DESIGN" -C "$tmp/sr1"
sr1="$tmp/sr1/Arborcore-VIEW0-V1N1-G03-R7A-SR1-global-failure-atomicity-correction-design-freeze-candidate"
(cd "$sr1" && sha256sum -c sr1-manifest.sha256 >/dev/null)
for marker in \
 'VIEW0_V1N1_G03_R7A_SR1_PREDECESSOR_MANIFEST_SHA256=490f033f34d011ba6cc1f97b20dbe4a5b34a1647130f65d902c2ff3438a9e3ed' \
 'VIEW0_V1N1_G03_R7A_SR1_ANCHOR_BYTES=8' \
 'VIEW0_V1N1_G03_R7A_SR1_ANCHOR_WORKSPACE_BYTES=32768' \
 'VIEW0_V1N1_G03_R7A_SR1_LAST_FALLIBLE_OPERATION=EXACT_FAILURE_ATOMIC_LEXBOR_PARSE_PUBLICATION' \
 'VIEW0_V1N1_G03_R7A_SR1_R1_R7_SEMANTICS_CHANGE=NO' \
 'VIEW0_V1N1_G03_R7A_SR1_STACK_THRESHOLD_WIDENING=NO'; do
  grep -Fxq "$marker" "$sr1/sr1-summary.txt" || fail "SR1 design marker missing: $marker"
done
python3 - "$F1_R13" <<'PY'
import pathlib,sys,tarfile
p=pathlib.Path(sys.argv[1])
with tarfile.open(p,'r:gz') as tf:
    ms=tf.getmembers()
    if not ms: raise SystemExit('FAIL: F1-R13 archive empty')
    for m in ms:
        q=pathlib.PurePosixPath(m.name)
        if not m.name or m.name.startswith('/') or q.is_absolute() or '..' in q.parts:
            raise SystemExit(f'FAIL: unsafe F1-R13 path: {m.name!r}')
        if not (m.isdir() or m.isreg()):
            raise SystemExit(f'FAIL: unsafe F1-R13 member type: {m.name!r}')
PY
tar -xzf "$F1_R13" -C "$tmp"
f1="$tmp/Arborcore-VIEW0-V1N1-M0B-F1-R13-g03-r7-fixture-isolation-correction-freeze-candidate"
(cd "$f1" && sha256sum -c f1-r13-manifest.sha256 >/dev/null)
for spec in \
 'f1-final-rule-spec-test-matrix.tsv|541a951ddaf24ef5a50bfbb4f19804bd3a60f20f8b6a3bed45216c61b6b125ad' \
 'f1-r12-r7-subject-support-plan.tsv|4c87be77fbc8fc8a12c5bfb17dd413d758db3bc4d70099dc4f37fc7ac8607c2d' \
 'f1-r12-r7-palpable-child-support-plan.tsv|ade32df3b7f309db13bc0c2e1673757bb7359cc195724b4ec6b6c9548b9f9233' \
 'f1-r13-r7-fixture-plan.tsv|521152cca7b068770910345e06b2b27fcde855ea4f5e22bf76bafc315a726fa2' \
 'f1-r12-r7-diagnostic-plan.tsv|92967d4fe2788aff6b79156cc830c432f456ba17c0b744c134148934cc995f14' \
 'f1-r12-r7-source-boundary.tsv|4fb9cc308154c9f79753c1a75ceca727372030fd62ecb359f8b831e7d44b8819'; do
  IFS='|' read -r n h <<<"$spec"; need_eq "$(sha256sum "$f1/$n"|awk '{print $1}')" "$h" "$n"
done
cmp -s tests/data/view0_v1n1_g03_r7a_subject_support_plan.tsv "$f1/f1-r12-r7-subject-support-plan.tsv" || fail 'installed R7 subject plan drift'
cmp -s tests/data/view0_v1n1_g03_r7a_palpable_child_support_plan.tsv "$f1/f1-r12-r7-palpable-child-support-plan.tsv" || fail 'installed R7 palpable-child plan drift'
cmp -s tests/data/view0_v1n1_g03_r7a_fixture_plan.tsv "$f1/f1-r13-r7-fixture-plan.tsv" || fail 'installed corrected R7 fixture plan drift'
cmp -s tests/data/view0_v1n1_g03_r7a_diagnostic_plan.tsv "$f1/f1-r12-r7-diagnostic-plan.tsv" || fail 'installed R7 diagnostic plan drift'
cmp -s tests/data/view0_v1n1_g03_r7a_source_boundary.tsv "$f1/f1-r12-r7-source-boundary.tsv" || fail 'installed R7 source boundary drift'
need_eq "$(sha256sum tests/data/view0_v1n1_g03_r7a_retained_fixture_isolation.tsv|awk '{print $1}')" '078b2743d88b57af8528d56ac85610553172528c8a3d8b7af5063abd6beb043d' 'R7A retained fixture isolation ledger'
need_eq "$(($(wc -l < tests/data/view0_v1n1_g03_r7a_retained_fixture_isolation.tsv)-1))" '8' 'R7A retained fixture isolation count'

python3 - "$f1/r7-p0-standard-subject-inventory.tsv" "$f1/r7-p0-palpable-content-inventory.tsv" tools/c/view0_conformance/g03_r7a.c <<'PY'
from pathlib import Path
import csv,re,sys
subjects, palpable, srcp = map(Path, sys.argv[1:])
src=srcp.read_text()
with subjects.open(newline='') as f:
    rows=list(csv.DictReader(f, delimiter='\t'))
expected_stable={r['element'].upper() for r in rows if r['subject_class']=='STABLE_ALLOWS_FLOW_OR_PHRASING'}
active={r['element'] for r in rows if r['subject_class']=='ACTIVE_BRANCH_SENSITIVE'}
if len(rows)!=65 or len(expected_stable)!=62 or active!={'div','time','option'}:
    raise SystemExit(f'FAIL: subject authority drift rows={len(rows)} stable={len(expected_stable)} active={sorted(active)}')
def segment(a,b):
    i=src.find(a); j=src.find(b,i+1)
    if i<0 or j<0: raise SystemExit(f'FAIL: source segment missing {a!r}..{b!r}')
    return src[i:j]
stable=segment('static bool stable_subject', 'static bool unconditional_standard_palpable')
actual_stable=set(re.findall(r'case ARBOR_VIEW0_NATIVE_ELEMENT_([A-Z0-9_]+):', stable))
if actual_stable != expected_stable:
    raise SystemExit(f'FAIL: stable subject set drift missing={sorted(expected_stable-actual_stable)} extra={sorted(actual_stable-expected_stable)}')
with palpable.open(newline='') as f:
    prows=list(csv.DictReader(f, delimiter='\t'))
expected_uncond={r['pinned_item'].upper() for r in prows if r['class']=='STANDARD_HTML_ELEMENT' and r['conditional']=='NO'}
if len(prows)!=77 or len(expected_uncond)!=67:
    raise SystemExit(f'FAIL: palpable authority drift rows={len(prows)} unconditional={len(expected_uncond)}')
uncond=segment('static bool unconditional_standard_palpable', 'static void set_deferred')
actual_uncond=set(re.findall(r'case ARBOR_VIEW0_NATIVE_ELEMENT_([A-Z0-9_]+):', uncond))
if actual_uncond != expected_uncond:
    raise SystemExit(f'FAIL: unconditional palpable set drift missing={sorted(expected_uncond-actual_uncond)} extra={sorted(actual_uncond-expected_uncond)}')
classify=segment('static r7_child_class classify_element_palpability', 'static r7_subject_class classify_subject')
actual_cond=set(re.findall(r'case ARBOR_VIEW0_NATIVE_ELEMENT_([A-Z0-9_]+):', classify))
expected_cond={'AUDIO','INPUT','DL','MENU','OL','UL'}
if actual_cond != expected_cond:
    raise SystemExit(f'FAIL: conditional palpable set drift got={sorted(actual_cond)}')
for needle in ['frame->foreign_math || frame->foreign_svg','frame->autonomous_custom','R7_ATTR_HIDDEN']:
    if needle not in classify: raise SystemExit(f'FAIL: R7 child classifier boundary missing {needle}')
print('VIEW0_V1N1_G03_R7A_STABLE_SUBJECT_SOURCE_SET=62_OF_62')
print('VIEW0_V1N1_G03_R7A_UNCONDITIONAL_PALPABLE_SOURCE_SET=67_OF_67')
print('VIEW0_V1N1_G03_R7A_CONDITIONAL_PALPABLE_SOURCE_SET=6_OF_6')
PY

r7=tools/c/view0_conformance/g03_r7a.c
[[ -f "$r7" && -f tools/c/view0_conformance/g03_r7a.h ]] || fail 'R7A source/header missing'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' "$r7" || fail 'direct heap allocation appeared in R7A evaluator'
grep -Fq '_Static_assert(sizeof(g03_r7a_context) <= 1048576u' "$r7" || fail 'R7A 1MiB workspace admission missing'
grep -Fq '#define ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY UINT64_C(0x0000000030030007)' tools/include/arborcore/view0_conformance/native.h || fail 'R7 rule ID missing'
for spec in \
 'ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_PARTIAL UINT64_C(0x80000)' \
 'ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G04_TRANSPARENT UINT64_C(0x100000)' \
 'ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM UINT64_C(0x200000)'; do grep -Fq "$spec" tools/include/arborcore/view0_conformance/native.h || fail "R7 result flag drift: $spec"; done
! grep -ERq 'ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT|0x0000000030030006|G03_R6' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R6 runtime semantic/flag code appeared under R7A'
grep -Fq 'typedef struct arbor_view0_native_source_anchor' tools/include/arborcore/view0_conformance/native.h || fail 'SR2 compact anchor type missing'
grep -Fq '_Static_assert(sizeof(arbor_view0_native_source_anchor) == 8u' tools/include/arborcore/view0_conformance/native.h || fail 'SR2 8-byte anchor layout assertion missing'
grep -Fq 'arbor_view0_native_lexbor_collect_exact(' tools/c/view0_conformance/native.c || fail 'SR2 native coordinator exact Lexbor publication missing'
for rule in r1a r2a r3a r4a r5a r7a; do
  grep -Fq "arbor_view0_native_g03_${rule}_collect_anchors(" "tools/c/view0_conformance/g03_${rule}.c" || fail "SR2 $rule anchor collection missing"
  grep -Fq "arbor_view0_native_g03_${rule}_materialize_anchor(" "tools/c/view0_conformance/g03_${rule}.c" || fail "SR2 $rule no-fail materializer missing"
done
[[ -f tests/c/view0_v1n1_g03_r7a_global_failure_atomicity_test.c ]] || fail 'SR2 failure-atomicity test source missing'
for src in tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c tools/c/view0_conformance/g03_r3a.c tools/c/view0_conformance/g03_r4a.c tools/c/view0_conformance/g03_r5a.c tools/c/view0_conformance/g03_r7a.c; do
  ! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' "$src" || fail "direct Arborcore heap allocation appeared in SR2 source: $src"
done
grep -F 'VIEW0_V1N0_NATIVE_RULE_SOURCES :=' Makefile | grep -Fq '$(VIEW0_V1N1_G03_R7A_SOURCE)' || fail 'R7A source missing from Makefile sanitizer source closure'
grep -Fq '$(VIEW0_V1N1_G03_R7A_OBJ)' Makefile || fail 'R7A object missing from native rule object closure'
grep -Fxq 'export LC_ALL=C' tools/view0_v1n1_g03_r7a_scope_verify.sh || fail 'R7A scope verifier does not self-pin LC_ALL=C'
gate=tools/view0_v1n1_g03_r7a_gate.sh
for obj in native.o lexbor_adapter.o g03_c0_provenance.o g03_r1a.o g03_r2a.o g03_r3a.o g03_r4a.o g03_r5a.o g03_r7a.o; do grep -Fq "build/view0-v1/native/$obj" "$gate" || fail "R7A clean reset missing native rule object: $obj"; done
grep -Fxq "echo 'VIEW0_V1N1_G03_R7A_RETAINED_NATIVE_RULE_OBJECT_CLEAN_RESET=9_OF_9'" "$gate" || fail 'R7A 9/9 clean-reset evidence marker missing'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'

for rule in r1a r2a r3a r4a r5a r6a; do
    out=$(bash "tools/view0_v1n1_g03_${rule}_contract_verify.sh")
    marker="VIEW0_V1N1_G03_${rule^^}_RETAINED_UNDER_G03_R7A=PASS"
    grep -Fxq "$marker" <<<"$out" || fail "$rule did not publish R7A retention"
    ! grep -Fq 'G03_R7_RULE_IDS_IMPLEMENTED=ZERO' <<<"$out" || fail "$rule republished stale R7-zero current-state evidence under R7A"
done
echo 'VIEW0_V1N1_G03_R7A_RETAINED_R1_R6_CONTRACT_VERIFIERS=PASS'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G03_R7A_SR2_ACCEPTED_SR1_DESIGN=PASS'
echo 'PASS: G03 R7A SR2 contract binds accepted SR1 atomicity design while retaining F1-R13 and lower-stage semantics'
