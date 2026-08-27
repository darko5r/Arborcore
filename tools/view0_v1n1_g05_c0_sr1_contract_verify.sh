#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1="${ARBOR_A1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }; sha(){ sha256sum -- "$1"|awk '{print $1}'; }
[[ -f "$A1" && "$(sha "$A1")" == 1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e ]] || fail A1
[[ "$(sha tests/data/view0_v1n1_g05_c0_conditional_predicate_catalog.tsv)" == 33329f61bbf9f22dcd69365e8f399d8caaa6df393a2d8ceac0e50ee07611257c ]] || fail 'corrected catalog SHA'
[[ "$(sha tests/data/view0_v1n1_g05_c0_sr1_input_state_authority.tsv)" == 1edfa7f1e732b1436774262905d81c780cb793c68f36cc233099ecf2f47a7f10 ]] || fail 'state authority SHA'
t=$(mktemp -d /tmp/arborcore-g05-c0-sr1-contract.XXXXXX); trap 'rm -rf "$t"' EXIT
tar -xzf "$A1" -C "$t"; a1=$(find "$t" -type f -name a1-manifest.sha256 -printf '%h\n' -quit); [[ -n "$a1" ]] || fail 'A1 root'; (cd "$a1" && sha256sum -c a1-manifest.sha256 >/dev/null)
python3 - "$a1/f1-g05-conditional-applicability-clauses.tsv" "$a1/element-sections/087-input.whatwg-source" tests/data/view0_v1n1_g05_c0_conditional_predicate_catalog.tsv tests/data/view0_v1n1_g05_c0_sr1_input_state_authority.tsv <<'PY'
import csv,hashlib,re,sys
src_clause,src_input,local_cat,state_map=sys.argv[1:]
def rd(p):
 with open(p,encoding='utf-8',newline='') as f:return list(csv.DictReader(f,delimiter='\t'))
a=rd(src_clause); l=rd(local_cat); m=rd(state_map)
if len(a)!=43 or len(l)!=43: raise SystemExit('FAIL 43 clauses')
for i,(x,y) in enumerate(zip(a,l),1):
 for k in ('element','source_file','source_relative_line','clause_sha256'):
  if x[k]!=y[k]: raise SystemExit(f'FAIL clause identity {i} {k}')
inputs=[r for r in l if r['element']=='input']
if len(inputs)!=24 or len(m)!=25 or len({r['state_keyword'] for r in m})!=22: raise SystemExit('FAIL state cardinality')
lines=open(src_input,encoding='utf-8').read().splitlines(); heads=[]
for i,line in enumerate(lines,1):
 if re.search(r'<h[2-6][^>]*',line):
  chunk=[]; j=i-1
  while j<len(lines) and len(chunk)<8:
   chunk.append(lines[j]); j+=1
   if '</h' in chunk[-1]:break
  raw='\n'.join(chunk)+'\n'; text=re.sub('<[^>]+>',' ',' '.join(chunk)); text=re.sub(r'\s+',' ',text).strip(); dx=re.search(r'data-x="([^"]+)"',raw)
  heads.append((i,dx.group(1) if dx else '',text,hashlib.sha256(raw.encode()).hexdigest()))
expected={16:'Hidden state',17:'Hidden state',18:'Text',19:'Telephone state',20:'URL state',21:'Email state',22:'Password state',23:'Date state',24:'Month state',25:'Week state',26:'Time state',27:'Local Date and Time state',28:'Number state',29:'Range state',30:'Color state',31:'Checkbox state',32:'Radio Button state',33:'File Upload state',34:'File Upload state',35:'Submit Button state',36:'Image Button state',37:'Image Button state',38:'Reset Button state',39:'Button state'}
for r in inputs:
 o=int(r['clause_ordinal']); n=int(r['source_relative_line']); h=max(x for x in heads if x[0]<=n)
 if expected[o] not in h[2]: raise SystemExit(f'FAIL heading {o}: {h[2]}')
 states=[x for x in r['input_state_data_x_suffix'].split('|') if x]
 mm=[x for x in m if x['clause_ordinal']==r['clause_ordinal']]
 if sorted(x['state_keyword'] for x in mm)!=sorted(states): raise SystemExit(f'FAIL states {o}')
 for x in mm:
  if int(x['heading_start_line'])!=h[0] or x['heading_data_x']!=h[1] or x['heading_text']!=h[2] or x['heading_source_sha256']!=h[3]: raise SystemExit(f'FAIL heading evidence {o}')
print('VIEW0_V1N1_G05_C0_SR1_CONDITIONAL_CLAUSE_ROWS=43')
print('VIEW0_V1N1_G05_C0_SR1_INPUT_CLAUSE_ROWS=24')
print('VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_CLAUSE_ROWS=25')
print('VIEW0_V1N1_G05_C0_SR1_DISTINCT_INPUT_STATES=22')
print('VIEW0_V1N1_G05_C0_SR1_PINNED_INPUT_HEADING_AUTHORITY=PASS')
PY
for x in \
 'VIEW0_V1N1_G05_C0_SR1_R1_SEMANTICS_CHANGE=NO' \
 'VIEW0_V1N1_G05_C0_SR1_R2_SEMANTICS_CHANGE=NO' \
 'VIEW0_V1N1_G05_C0_SR1_R3_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G05_C0_SR1_R4_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G05_C0_SR1_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G05_C0_SR1_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G05_C0_SR1_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G05_C0_SR1_PHASED_STACK_BOUND_BYTES=900000' \
 'VIEW0_V1N1_G05_C0_SR1_STACK_THRESHOLD_WIDENING=NO'; do grep -Fxq "$x" view/arborcore-view-core-1.contract || fail "contract marker $x"; done
echo 'VIEW0_V1N1_G05_C0_SR1_CONTRACT_VERIFY=PASS'
echo 'PASS: exact A1 43-clause authority and corrected input-state ownership bound'
