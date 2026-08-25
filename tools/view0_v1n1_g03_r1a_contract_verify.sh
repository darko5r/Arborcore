#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

P0_BUNDLE="${VIEW0_G03_P0_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-P0-preconstruction-review-candidate.tar.gz}"
F1_R5_BUNDLE="${VIEW0_G03_F1_R5_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-M0B-F1-R5-g03-r1-partial-support-plan-freeze-candidate.tar.gz}"
AN_P0_BUNDLE="${VIEW0_G03_AN_P0_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-R1-AN-P0-accessible-name-feasibility-review-candidate.tar.gz}"

fail() { echo "FAIL: $*" >&2; exit 1; }
need_eq() { [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }

contract='view/arborcore-view-core-1.contract'
header='tools/include/arborcore/view0_conformance/native.h'
native='tools/c/view0_conformance/native.c'
r1a='tools/c/view0_conformance/g03_r1a.c'
doc='docs/VIEW_CORE_VIEW0.md'
coverage='tests/data/view0_v1n1_g03_r1a_context_coverage.tsv'

current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R1A|0.1-VIEW0-V1N1-G03-R2A|0.1-VIEW0-V1N1-G03-R3A|0.1-VIEW0-V1N1-G03-R4A|0.1-VIEW0-V1N1-G03-R5A|0.1-VIEW0-V1N1-G03-R6A|0.1-VIEW0-V1N1-G03-R7A) ;;
  *) fail "current contract is not an accepted R1A extension: $current_version" ;;
esac

for item in \
 'VIEW0_V1N1_G03_R1A_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-R1A' \
 'VIEW0_V1N1_G03_R1A_SCOPE=PARTIAL_STRUCTURAL_ELEMENT_CONTEXT_EVALUATOR_OVER_ACCEPTED_G03_C0_L1' \
 'VIEW0_V1N1_G03_R1A_PREDECESSOR_CANDIDATE_PATH_COUNT=128' \
 'VIEW0_V1N1_G03_R1A_PREDECESSOR_CANDIDATE_MANIFEST_SHA256=ddad30143b5144a79b9e3571f0b3d2b0922b68615f605b760c6adec46746f0ef' \
 'VIEW0_V1N1_G03_R1A_F1_R5_BUNDLE_SHA256=20495201759d5dff086393971b0886994b067a6cec848bc8ee8b466a8427ec58' \
 'VIEW0_V1N1_G03_R1A_F1_R5_MATRIX_SHA256=5a5b33c646220ba6fa64cfcbde3f86299374d1e887439df4fa46568140e4a9a2' \
 'VIEW0_V1N1_G03_R1A_SOURCE_SET_SHA256=2e37e1de692b4cc0ce6877f6526c3845554319f67a6ba4969de4a38797c15687' \
 'VIEW0_V1N1_G03_R1A_AN_P0_BUNDLE_SHA256=ee358abe9d664d7b76f0d79eea8ad49ee1f69a9eafa0878e80541cd9878c4a3c' \
 'VIEW0_V1N1_G03_R1A_RULE_ID=0x0000000030030001' \
 'VIEW0_V1N1_G03_R1A_RULE_SYMBOL=ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT' \
 'VIEW0_V1N1_G03_R1A_SEVERITY=ERROR' \
 'VIEW0_V1N1_G03_R1A_CONTEXT_ALTERNATIVE_COUNT=62' \
 'VIEW0_V1N1_G03_R1A_IMPLEMENTED_STABLE_CONTEXT_COUNT=58' \
 'VIEW0_V1N1_G03_R1A_DELEGATED_G02_CONTEXT_COUNT=2' \
 'VIEW0_V1N1_G03_R1A_PARTIAL_DEFERRED_CONTEXT_COUNT=1' \
 'VIEW0_V1N1_G03_R1A_MODE_UNREACHABLE_CONTEXT_COUNT=1' \
 'VIEW0_V1N1_G03_R1A_IMPLEMENTATION_COMPLETE=NO' \
 'VIEW0_V1N1_G03_R1A_DEFERRED_BRANCH=MAIN_FORM_ACCESSIBLE_NAME' \
 'VIEW0_V1N1_G03_R1A_MAIN_FORM_DEFINITELY_UNNAMED=NO_ARIA_LABEL_NO_ARIA_LABELLEDBY_NO_TITLE' \
 'VIEW0_V1N1_G03_R1A_MAIN_FORM_POTENTIALLY_NAMED=DEFERRED_NONREJECTING' \
 'VIEW0_V1N1_G03_R1A_G13_CUSTOM_ELEMENT_BRANCH=NONREJECTING_ASCII_HYPHEN_SUPERSET' \
 'VIEW0_V1N1_G03_R1A_BODY_OK_KEYWORDS=dns-prefetch,modulepreload,pingback,preconnect,prefetch,preload,stylesheet' \
 'VIEW0_V1N1_G03_R1A_MAX_OBSERVATION_DEPTH_INCLUSIVE=4097' \
 'VIEW0_V1N1_G03_R1A_DIAGNOSTIC_ORDER=BYTE_OFFSET_THEN_RULE_ID_THEN_SEVERITY_THEN_DISCOVERY_SEQUENCE' \
 'VIEW0_V1N1_G03_R1A_CAPACITY_FAILURE_ATOMICITY=TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G03_R1A_C0_L1_RETAINED=YES' \
 'VIEW0_V1N1_G03_R1A_G02_GROUP_RETAINED=FROZEN_6_OF_6' \
 'VIEW0_V1N1_G03_R1A_G03_RULE_IDS_ACTIVE=1' \
 'VIEW0_V1N1_G03_R1A_G03_R2_R7_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_R1A_G03_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G03_R1A_C0_DOCUMENT_FACT_SIZE_X86_64=184' \
 'VIEW0_V1N1_G03_R1A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G03_R1A_FRAME_SIZE_X86_64=192' \
 'VIEW0_V1N1_G03_R1A_EVALUATOR_WORKSPACE_SIZE_X86_64=786872' \
 'VIEW0_V1N1_G03_R1A_EVALUATOR_WORKSPACE_MAX_BYTES=1048576' \
 'VIEW0_V1N1_G03_R1A_EVALUATOR_WORKSPACE_STORAGE=CALL_STACK_FIXED_BOUNDED' \
 'VIEW0_V1N1_G03_R1A_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G03_R1A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G03_R1A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
    grep -Fxq "$item" "$contract" || fail "missing G03 R1A contract marker: $item"
done

[[ -f "$P0_BUNDLE" ]] || fail "missing exact G03 P0 bundle: $P0_BUNDLE"
[[ -f "$F1_R5_BUNDLE" ]] || fail "missing exact F1-R5 bundle: $F1_R5_BUNDLE"
[[ -f "$AN_P0_BUNDLE" ]] || fail "missing exact AN-P0 bundle: $AN_P0_BUNDLE"
need_eq "$(sha256sum "$P0_BUNDLE" | awk '{print $1}')" \
    '53d42457fcedc58b58983b04b07abb5d54a8f5613584878734f316f1b8ec7ca6' 'G03 P0 bundle SHA-256'
need_eq "$(sha256sum "$F1_R5_BUNDLE" | awk '{print $1}')" \
    '20495201759d5dff086393971b0886994b067a6cec848bc8ee8b466a8427ec58' 'F1-R5 bundle SHA-256'
need_eq "$(sha256sum "$AN_P0_BUNDLE" | awk '{print $1}')" \
    'ee358abe9d664d7b76f0d79eea8ad49ee1f69a9eafa0878e80541cd9878c4a3c' 'AN-P0 bundle SHA-256'

tmp=$(mktemp -d /tmp/arborcore-g03-r1a-contract.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/p0" "$tmp/f15" "$tmp/anp0"
tar -xzf "$P0_BUNDLE" -C "$tmp/p0"
tar -xzf "$F1_R5_BUNDLE" -C "$tmp/f15"
tar -xzf "$AN_P0_BUNDLE" -C "$tmp/anp0"
p0="$tmp/p0/Arborcore-VIEW0-V1N1-G03-P0-preconstruction-review-candidate"
f15="$tmp/f15/Arborcore-VIEW0-V1N1-M0B-F1-R5-g03-r1-partial-support-plan-freeze-candidate"
anp0="$tmp/anp0/Arborcore-VIEW0-V1N1-G03-R1-AN-P0-accessible-name-feasibility-review-candidate"
need_eq "$(sha256sum "$f15/f1-final-rule-spec-test-matrix.tsv" | awk '{print $1}')" \
    '5a5b33c646220ba6fa64cfcbde3f86299374d1e887439df4fa46568140e4a9a2' 'F1-R5 matrix SHA-256'
need_eq "$(sha256sum "$f15/f1-r3-g03-r1-source-set.tsv" | awk '{print $1}')" \
    '2e37e1de692b4cc0ce6877f6526c3845554319f67a6ba4969de4a38797c15687' 'R1 source-set SHA-256'

python3 - "$p0/g03-context-alternative-inventory.tsv" "$coverage" "$f15/f1-final-rule-spec-test-matrix.tsv" "$anp0/main-form-accessible-name-tristate-policy-candidate.txt" <<'PY'
import csv, pathlib, sys
src, cov, matrix, policy = map(pathlib.Path, sys.argv[1:])
with src.open(newline='',encoding='utf-8') as f:
    a=list(csv.DictReader(f,delimiter='\t'))
with cov.open(newline='',encoding='utf-8') as f:
    b=list(csv.DictReader(f,delimiter='\t'))
if len(a)!=62 or len(b)!=62:
    raise SystemExit(f'FAIL: context coverage count P0={len(a)} R1A={len(b)} expected=62')
for x,y in zip(a,b):
    for key in ('context_ordinal','context_alternative','element_definition_names'):
        if x[key] != y[key]:
            raise SystemExit(f'FAIL: coverage diverges from exact P0 at ordinal {x["context_ordinal"]} field={key}')
from collections import Counter
counts=Counter(x['r1a_disposition'] for x in b)
expected=Counter({
    'IMPLEMENTED_R1A_STABLE_RELATION':58,
    'DELEGATED_G02_SPECIFIC':2,
    'PARTIAL_AN_P0_DEFERRED':1,
    'MODE_UNREACHABLE_COMPOUND_SUBDOCUMENT':1,
})
if counts != expected:
    raise SystemExit(f'FAIL: R1A disposition counts={counts} expected={expected}')
if [int(x['context_ordinal']) for x in b if x['r1a_disposition']=='DELEGATED_G02_SPECIFIC'] != [45,46]:
    raise SystemExit('FAIL: exact G02 delegated context ordinals changed')
if [int(x['context_ordinal']) for x in b if x['r1a_disposition']=='PARTIAL_AN_P0_DEFERRED'] != [53]:
    raise SystemExit('FAIL: exact AN-P0 partial context ordinal changed')
if [int(x['context_ordinal']) for x in b if x['r1a_disposition']=='MODE_UNREACHABLE_COMPOUND_SUBDOCUMENT'] != [62]:
    raise SystemExit('FAIL: exact mode-unreachable context ordinal changed')
with matrix.open(newline='',encoding='utf-8') as f:
    rows=list(csv.DictReader(f,delimiter='\t'))
r=next(x for x in rows if x['rule_id_hex']=='0x0000000030030001')
if r['rule_symbol']!='ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT' or r['severity']!='ERROR':
    raise SystemExit('FAIL: F1-R5 R1 identity/severity drift')
if 'R1A may emit ELEMENT_CONTEXT only for definite supported violations' not in r['deferral_boundary']:
    raise SystemExit('FAIL: F1-R5 partial emission boundary missing')
p=policy.read_text(encoding='utf-8')
for marker in (
 'FORM_ACCESSIBLE_NAME_CLASSIFICATION=TRISTATE',
 'FORM_NAME_DEFINITELY_UNNAMED=No_aria-label_attribute_AND_no_aria-labelledby_attribute_AND_no_title_attribute',
 'FORM_NAME_DEFERRED=Any_aria-label_OR_aria-labelledby_OR_title_attribute_is_present',
 'FULL_ACCNAME_ENGINE_IN_G03_NOW=NO'):
    if marker not in p:
        raise SystemExit(f'FAIL: accepted AN-P0 marker missing: {marker}')
print('VIEW0_V1N1_G03_R1A_CONTEXT_ALTERNATIVE_COUNT=62')
print('VIEW0_V1N1_G03_R1A_IMPLEMENTED_STABLE_CONTEXT_COUNT=58')
print('VIEW0_V1N1_G03_R1A_DELEGATED_G02_CONTEXT_COUNT=2')
print('VIEW0_V1N1_G03_R1A_PARTIAL_DEFERRED_CONTEXT_COUNT=1')
print('VIEW0_V1N1_G03_R1A_MODE_UNREACHABLE_CONTEXT_COUNT=1')
PY

need_eq "$(sha256sum "$coverage" | awk '{print $1}')" \
    '5d7750ded45fa9c236b5c7fdb2d6a6a2be1c96635a74429c2a8db4211b25ab50' 'R1A context coverage SHA-256'
grep -Fq '#define ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT UINT64_C(0x0000000030030001)' "$header" || fail 'stable G03 R1 rule ID missing'
grep -Fq '#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL UINT64_C(0x2)' "$header" || fail 'R1 partial flag missing'
grep -Fq '#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_DEFERRED_MAIN_FORM UINT64_C(0x4)' "$header" || fail 'R1 deferred-main-form flag missing'
grep -Fq '#include "g03_r1a.h"' "$native" || fail 'native checker does not integrate R1A'
grep -Fq 'arbor_view0_native_g03_r1a_measure' "$native" || fail 'R1A measurement pass missing'
grep -Fq 'arbor_view0_native_g03_r1a_collect' "$native" || fail 'R1A publication pass missing'
grep -Fq 'G03_R1A_MAX_DEPTH UINT64_C(4097)' "$r1a" || fail 'R1A 4097 depth policy missing'
for keyword in dns-prefetch modulepreload pingback preconnect prefetch preload stylesheet; do
    grep -Fq "\"$keyword\"" "$r1a" || fail "pinned body-ok keyword absent from R1A: $keyword"
done
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R1A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(CONTENT_MODEL|DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[2-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R2-R7 semantics appeared during R1A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R2A' ]]; then
    grep -Fq '#define ARBOR_VIEW_V1_G03_CONTENT_MODEL UINT64_C(0x0000000030030002)' "$header" || fail 'R2A extension rule ID missing'
    grep -Fq '#include "g03_r2a.h"' "$native" || fail 'R2A extension is not integrated after retained R1A'
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[3-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R3-R7 semantics appeared under R2A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    grep -Fq '#define ARBOR_VIEW_V1_G03_CONTENT_MODEL UINT64_C(0x0000000030030002)' "$header" || fail 'R2A retained rule ID missing under R3A'
    grep -Fq '#define ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS UINT64_C(0x0000000030030003)' "$header" || fail 'R3A extension rule ID missing'
    grep -Fq '#include "g03_r2a.h"' "$native" || fail 'R2A retained integration missing under R3A'
    grep -Fq '#include "g03_r3a.h"' "$native" || fail 'R3A extension is not integrated after retained R1A'
    grep -Fq 'arbor_view0_native_g03_r1a_collect_offsets' tools/c/view0_conformance/g03_r1a.c || fail 'R1A private offset-collection compatibility mode missing under R3A'
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 semantics appeared under R3A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    grep -Fq '#define ARBOR_VIEW_V1_G03_CONTENT_MODEL UINT64_C(0x0000000030030002)' "$header" || fail 'R2A retained rule ID missing under R4A'
    grep -Fq '#define ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS UINT64_C(0x0000000030030003)' "$header" || fail 'R3A retained rule ID missing under R4A'
    grep -Fq '#define ARBOR_VIEW_V1_G03_NOTHING_MODEL UINT64_C(0x0000000030030004)' "$header" || fail 'R4A extension rule ID missing'
    grep -Fq '#include "g03_r2a.h"' "$native" || fail 'R2A retained integration missing under R4A'
    grep -Fq '#include "g03_r3a.h"' "$native" || fail 'R3A retained integration missing under R4A'
    grep -Fq '#include "g03_r4a.h"' "$native" || fail 'R4A extension is not integrated after retained R1A'
    grep -Fq 'arbor_view0_native_g03_r1a_collect_offsets' tools/c/view0_conformance/g03_r1a.c || fail 'R1A private offset-collection compatibility mode missing under R4A'
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 semantics appeared under R4A'
else
    for spec in \
      'ARBOR_VIEW_V1_G03_CONTENT_MODEL|0x0000000030030002|R2A' \
      'ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS|0x0000000030030003|R3A' \
      'ARBOR_VIEW_V1_G03_NOTHING_MODEL|0x0000000030030004|R4A' \
      'ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE|0x0000000030030005|R5A'; do
        IFS='|' read -r symbol id label <<<"$spec"
        grep -Fq "#define $symbol UINT64_C($id)" "$header" || fail "$label retained/extension rule ID missing under R5A"
    done
    for r in r2a r3a r4a r5a; do grep -Fq "#include \"g03_${r}.h\"" "$native" || fail "$r integration missing under R5A"; done
    grep -Fq 'arbor_view0_native_g03_r1a_collect_offsets' tools/c/view0_conformance/g03_r1a.c || fail 'R1A private offset-collection compatibility mode missing under R5A'
    if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]]; then
        grep -Fq '#define ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY UINT64_C(0x0000000030030007)' "$header" || fail 'R7A extension rule ID missing'
        grep -Fq '#include "g03_r7a.h"' "$native" || fail 'R7A extension integration missing'
        ! grep -ERq 'ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT|0x0000000030030006|G03_R6' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R6 runtime semantic/flag code appeared under R7A'
    else
        ! grep -ERq 'ARBOR_VIEW_V1_G03_(SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[67]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R6-R7 runtime semantics appeared before R7A'
    fi
fi
if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' "$r1a"; then
    fail 'R1A introduced direct Arborcore heap allocation'
fi
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
grep -Fq '## V1N1 G03 R1A: partial structural ELEMENT_CONTEXT evaluator' "$doc" || fail 'R1A documentation missing'

echo 'VIEW0_V1N1_G03_R1A_RULE_ID=0x0000000030030001'
echo 'VIEW0_V1N1_G03_R1A_IMPLEMENTATION_COMPLETE=NO'
echo 'VIEW0_V1N1_G03_R1A_DEFERRED_BRANCH=MAIN_FORM_ACCESSIBLE_NAME'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R1A' ]]; then
    echo 'VIEW0_V1N1_G03_R1A_G03_R2_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R2A' ]]; then
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R2A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_G03_R3_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R3A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R1A_G03_R4_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R4A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R1A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R5A' ]]; then
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R5A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R1A_G03_R6_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]]; then
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R6A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R1A_G03_R7_RULE_IDS_IMPLEMENTED=ZERO'
else
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R7A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_PRIVATE_OFFSET_COLLECTION=PASS'
fi
echo 'VIEW0_V1N1_G03_R1A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'PASS: G03 R1A exact source/catalog, partial-boundary, ownership and no-growth contract'
