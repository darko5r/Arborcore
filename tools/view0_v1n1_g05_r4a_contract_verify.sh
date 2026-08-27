#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1="${ARBOR_A1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz}"
R3="${ARBOR_G05_R3A_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G05-R3A-construction-result-candidate-bcc20446e48c093bb0f1f53d886b5e5e25644a9b261823e084e7cfc308fa3e4a.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }
sha(){ sha256sum -- "$1" | awk '{print $1}'; }
[[ -f "$A1" && "$(sha "$A1")" == 1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e ]] || fail 'A1 archive'
[[ -f "$R3" && "$(sha "$R3")" == bcc20446e48c093bb0f1f53d886b5e5e25644a9b261823e084e7cfc308fa3e4a ]] || fail 'R3 archive'
tmp=$(mktemp -d /tmp/arborcore-g05-r4-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/a1" "$tmp/r3"
tar -xzf "$A1" -C "$tmp/a1"; tar -xzf "$R3" -C "$tmp/r3"
a1=$(find "$tmp/a1" -type f -name a1-manifest.sha256 -printf '%h\n' -quit)
r3=$(find "$tmp/r3" -type f -name evidence-manifest.sha256 -printf '%h\n' -quit)
[[ -n "$a1" && -n "$r3" ]] || fail 'archive roots'
(cd "$a1" && sha256sum -c a1-manifest.sha256 >/dev/null)
(cd "$r3" && sha256sum -c evidence-manifest.sha256 >/dev/null)
grep -Fxq 'VIEW0_V1N1_G05_R3A_CUMULATIVE_CANDIDATE_PATH_COUNT=104' "$r3/checkpoint.txt" || fail 'R3 predecessor count'
grep -Fxq 'VIEW0_V1N1_G05_R3A_CUMULATIVE_CANDIDATE_PATHLIST_SHA256=09a48b86da86eed414c6dfafca4f3a0748411a4ff2ee9a99c00021fa7f222ede' "$r3/checkpoint.txt" || fail 'R3 predecessor pathlist'
grep -Fxq 'VIEW0_V1N1_G05_R3A_CUMULATIVE_CANDIDATE_MANIFEST_SHA256=9ae6b08cbd1ef1a26d50457f682b253d96f892a7ea2d2b16437e1fe769ee0f65' "$r3/checkpoint.txt" || fail 'R3 predecessor manifest'
grep -Fxq 'VIEW0_V1N1_G05_R3A_CUMULATIVE_CANDIDATE_TREE=db275b3e89d6421f7ebf243bf250b83da8368695' "$r3/checkpoint.txt" || fail 'R3 predecessor tree'
python3 - \
  "$a1/v1n1-g04-g06-final-matrix.tsv" \
  "$a1/content-attribute-f1-classification.tsv" \
  tests/data/view0_v1n1_g05_r4a_rule_authority.tsv \
  tests/data/view0_v1n1_g05_c0_body_window_event_catalog.tsv <<'PY_AUTH'
import csv,sys
matrix,classify,rule_local,catalog=sys.argv[1:]
def rd(p):
    with open(p,encoding='utf-8',newline='') as f:return list(csv.DictReader(f,delimiter='\t'))
m=rd(matrix); a=rd(classify); r=rd(rule_local); c=rd(catalog)
mr=[x for x in m if x['rule_id_hex']=='0x0000000030050004']
if len(mr)!=1 or len(r)!=1: raise SystemExit('FAIL R4 rule cardinality')
for k in r[0]:
    if r[0][k]!=mr[0][k]: raise SystemExit(f'FAIL R4 rule field {k}')
if mr[0]['rule_symbol']!='ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY' or mr[0]['severity']!='ERROR': raise SystemExit('FAIL R4 identity')
if len(c)!=18 or len({x['attribute_name'] for x in c})!=18: raise SystemExit('FAIL R4 catalog cardinality')
expected={'onafterprint','onbeforeprint','onbeforeunload','onhashchange','onlanguagechange','onmessage','onmessageerror','onoffline','ononline','onpageswap','onpagehide','onpagereveal','onpageshow','onpopstate','onrejectionhandled','onstorage','onunhandledrejection','onunload'}
if {x['attribute_name'] for x in c}!=expected: raise SystemExit('FAIL R4 exact name set')
for x in c:
    if x['standard_element_id']!='8' or x['element_name']!='body' or x['classification']!='BODY_WINDOW_EVENT_HANDLER_CONTENT_ATTRIBUTE' or x['group_or_deferral']!='G05' or x['value_semantics_scope']!='EVENT_HANDLER_JAVASCRIPT_VALUE_SEMANTICS_DEFER_G16':
        raise SystemExit(f'FAIL R4 catalog row {x}')
ac=[x for x in a if x.get('heading_code_tokens')=='body' and x.get('f1_final_class')=='BODY_WINDOW_EVENT_HANDLER_CONTENT_ATTRIBUTE']
if len(ac)!=18: raise SystemExit(f'FAIL A1 body classification rows {len(ac)}')
by={(x['heading_code_tokens'],x['code_text'],x['code_data_x'],x['block_sha256'],x['f1_group_or_deferral'],x['f1_value_semantics_scope']) for x in ac}
for x in c:
    key=(x['element_name'],x['attribute_name'],x['attribute_data_x'],x['source_block_sha256'],x['group_or_deferral'],x['value_semantics_scope'])
    if key not in by: raise SystemExit(f'FAIL A1 body classification binding {x}')
print('VIEW0_V1N1_G05_R4A_AUTHORITY_RULE_ROWS=1_OF_1')
print('VIEW0_V1N1_G05_R4A_BODY_WINDOW_EVENT_CATALOG=18_OF_18')
print('VIEW0_V1N1_G05_R4A_BODY_SOURCE_HASH_AUTHORITY=PASS')
print('VIEW0_V1N1_G05_R4A_VALUE_SEMANTICS_OWNERSHIP=G16_DEFERRED')
PY_AUTH
grep -Fxq 'ARBORCORE_VIEW_CORE_VERSION=0.1-VIEW0-V1N1-G05-R4A' view/arborcore-view-core-1.contract || fail 'contract version'
for marker in \
 'VIEW0_V1N1_G05_R4A_RULE_ID=0x0000000030050004' \
 'VIEW0_V1N1_G05_R4A_BODY_WINDOW_EVENT_ROWS=18' \
 'VIEW0_V1N1_G05_R4A_APPLICABLE_OWNER=BODY_STANDARD_ELEMENT_ID_8_ONLY' \
 'VIEW0_V1N1_G05_R4A_VALUE_SEMANTICS=DEFER_G16_EVENT_HANDLER_JAVASCRIPT' \
 'VIEW0_V1N1_G05_R4A_R1_IMPLEMENTED=YES' \
 'VIEW0_V1N1_G05_R4A_R2_IMPLEMENTED=YES' \
 'VIEW0_V1N1_G05_R4A_R3_IMPLEMENTED=YES' \
 'VIEW0_V1N1_G05_R4A_R4_IMPLEMENTED=YES' \
 'VIEW0_V1N1_G05_R4A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G05_R4A_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G05_R4A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G05_R4A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fxq "$marker" view/arborcore-view-core-1.contract || fail "contract marker $marker"
done
grep -Fq '#define ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050004)' tools/include/arborcore/view0_conformance/native.h || fail 'R4 macro'
[[ -f tools/c/view0_conformance/g05_r4a.c && -f tools/c/view0_conformance/g05_r4a.h ]] || fail 'R4 evaluator paths'
echo 'VIEW0_V1N1_G05_R4A_CONTRACT_VERIFY=PASS'
echo 'PASS: exact R4 rule/body-only 18-name placement authority and G16 value boundary bound'
