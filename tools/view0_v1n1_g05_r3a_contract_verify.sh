#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1="${ARBOR_A1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz}"
C0_SR1="${ARBOR_G05_C0_SR1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G05-C0-SR1-construction-result-candidate-a0a03c65bd7d23089df87ce45fe16445361606539fa62c07933dddb3c3d54201.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }
sha(){ sha256sum -- "$1" | awk '{print $1}'; }
[[ -f "$A1" && "$(sha "$A1")" == 1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e ]] || fail 'A1 archive'
[[ -f "$C0_SR1" && "$(sha "$C0_SR1")" == a0a03c65bd7d23089df87ce45fe16445361606539fa62c07933dddb3c3d54201 ]] || fail 'C0-SR1 archive'
tmp=$(mktemp -d /tmp/arborcore-g05-r3-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/a1" "$tmp/sr1"
tar -xzf "$A1" -C "$tmp/a1"; tar -xzf "$C0_SR1" -C "$tmp/sr1"
a1=$(find "$tmp/a1" -type f -name a1-manifest.sha256 -printf '%h\n' -quit)
sr1=$(find "$tmp/sr1" -type f -name evidence-manifest.sha256 -printf '%h\n' -quit)
[[ -n "$a1" && -n "$sr1" ]] || fail 'archive roots'
(cd "$a1" && sha256sum -c a1-manifest.sha256 >/dev/null)
(cd "$sr1" && sha256sum -c evidence-manifest.sha256 >/dev/null)
python3 - \
  "$a1/v1n1-g04-g06-final-matrix.tsv" \
  "$a1/f1-g05-conditional-applicability-clauses.tsv" \
  tests/data/view0_v1n1_g05_r3a_rule_authority.tsv \
  tests/data/view0_v1n1_g05_c0_conditional_predicate_catalog.tsv \
  tests/data/view0_v1n1_g05_r3a_predicate_implementation.tsv \
  tests/data/view0_v1n1_g05_r3a_input_forbidden_attribute_authority.tsv \
  tools/c/view0_conformance/g05_r3a.c <<'PY'
import csv,re,sys
matrix,clauses,rule_local,cat,impl,forbidden,source=sys.argv[1:]
def rd(p):
    with open(p,encoding='utf-8',newline='') as f:return list(csv.DictReader(f,delimiter='\t'))
m=rd(matrix); r=rd(rule_local); a=rd(clauses); c=rd(cat); i=rd(impl); f=rd(forbidden)
mr=[x for x in m if x['rule_id_hex']=='0x0000000030050003']
if len(mr)!=1 or len(r)!=1: raise SystemExit('FAIL R3 rule cardinality')
for k in r[0]:
    if r[0][k] != mr[0][k]: raise SystemExit(f'FAIL R3 rule field {k}')
if mr[0]['rule_symbol']!='ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY' or mr[0]['severity']!='ERROR': raise SystemExit('FAIL R3 rule identity')
if len(a)!=43 or len(c)!=43 or len(i)!=43: raise SystemExit('FAIL 43 clause cardinality')
for n,(x,y,z) in enumerate(zip(a,c,i),1):
    if int(y['clause_ordinal'])!=n or int(z['clause_ordinal'])!=n: raise SystemExit(f'FAIL ordinal {n}')
    for k in ('element','source_file','source_relative_line','clause_sha256'):
        if x[k]!=y[k] or y[k]!=z[k]: raise SystemExit(f'FAIL clause identity {n} {k}')
if len(f)!=21: raise SystemExit('FAIL forbidden-state row count')
text=open(source,encoding='utf-8').read()
parsed={}
for state_expr,mask,clause in re.findall(r'\{([^\n]+?),\s*UINT64_C\((0x[0-9a-fA-F]+)\),\s*UINT64_C\((\d+)\)\}',text):
    clause=int(clause)
    if 17<=clause<=39 and clause not in (34,37): parsed[clause]=mask.lower()
if len(parsed)!=21: raise SystemExit(f'FAIL source forbidden table parsed {len(parsed)}')
for row in f:
    clause=int(row['clause_ordinal'])
    if parsed.get(clause)!=row['forbidden_attribute_mask_hex'].lower():
        raise SystemExit(f'FAIL forbidden mask clause {clause}: {parsed.get(clause)} != {row["forbidden_attribute_mask_hex"]}')
print('VIEW0_V1N1_G05_R3A_AUTHORITY_RULE_ROWS=1_OF_1')
print('VIEW0_V1N1_G05_R3A_CONDITIONAL_CLAUSE_ROWS=43')
print('VIEW0_V1N1_G05_R3A_PREDICATE_IMPLEMENTATION_ROWS=43')
print('VIEW0_V1N1_G05_R3A_INPUT_FORBIDDEN_STATE_ROWS=21')
print('VIEW0_V1N1_G05_R3A_INPUT_FORBIDDEN_MASK_AUTHORITY=PASS')
PY
[[ "$(awk -F '\t' 'NR>1{n++}END{print n+0}' tests/data/view0_v1n1_g05_c0_sr1_input_state_authority.tsv)" == 25 ]] || fail '25 state-clause rows'
[[ "$(awk -F '\t' 'NR>1{a[$4]=1}END{print length(a)}' tests/data/view0_v1n1_g05_c0_sr1_input_state_authority.tsv)" == 22 ]] || fail '22 input states'
version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1); [[ "$version" == '0.1-VIEW0-V1N1-G05-R3A' || "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]] || fail 'contract version'
for marker in \
 'VIEW0_V1N1_G05_R3A_RULE_ID=0x0000000030050003' \
 'VIEW0_V1N1_G05_R3A_CONDITIONAL_CLAUSE_ROWS=43' \
 'VIEW0_V1N1_G05_R3A_INPUT_STATE_CLAUSE_ROWS=25' \
 'VIEW0_V1N1_G05_R3A_DISTINCT_INPUT_STATES=22' \
 'VIEW0_V1N1_G05_R3A_R1_IMPLEMENTED=YES' \
 'VIEW0_V1N1_G05_R3A_R2_IMPLEMENTED=YES' \
 'VIEW0_V1N1_G05_R3A_R3_IMPLEMENTED=YES' \
 'VIEW0_V1N1_G05_R3A_R4_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G05_R3A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G05_R3A_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G05_R3A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G05_R3A_VALUE_LANGUAGE_OWNERSHIP=DEFERRED_TO_LATER_GROUPS'; do
  grep -Fxq "$marker" view/arborcore-view-core-1.contract || fail "contract marker $marker"
done
grep -Fq '#define ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050003)' tools/include/arborcore/view0_conformance/native.h || fail 'R3 macro'
if [[ "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then grep -Fq '#define ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050004)' tools/include/arborcore/view0_conformance/native.h || fail 'R4 macro'; [[ -f tools/c/view0_conformance/g05_r4a.c ]] || fail 'R4 evaluator missing'; echo 'VIEW0_V1N1_G05_R3A_CONTRACT_EXTENSION_AWARE=R4_RETAINED'; else [[ ! -e tools/c/view0_conformance/g05_r4a.c ]] || fail 'R4 evaluator appeared'; fi
echo 'VIEW0_V1N1_G05_R3A_CONTRACT_VERIFY=PASS'
echo 'PASS: exact R3 rule/43-clause/corrected-input-state authority and later-owner boundary bound'
