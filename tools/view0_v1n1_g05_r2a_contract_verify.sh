#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1="${ARBOR_A1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }; sha(){ sha256sum -- "$1"|awk '{print $1}'; }
[[ -f "$A1" && "$(sha "$A1")" == 1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e ]] || fail 'A1'
tmp=$(mktemp -d /tmp/arborcore-g05-r2-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT; tar -xzf "$A1" -C "$tmp"; a1=$(find "$tmp" -type f -name a1-manifest.sha256 -printf '%h\n' -quit); [[ -n "$a1" ]] || fail 'A1 root'; (cd "$a1"&&sha256sum -c a1-manifest.sha256 >/dev/null)
python3 - "$a1/v1n1-g04-g06-final-matrix.tsv" <<'PY2'
import csv,sys
with open(sys.argv[1],encoding='utf-8',newline='') as f: rows=[r for r in csv.DictReader(f,delimiter='\t') if r['rule_id_hex']=='0x0000000030050002']
if len(rows)!=1 or rows[0]['rule_symbol']!='ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY' or rows[0]['severity']!='ERROR': raise SystemExit('FAIL R2 authority')
if rows[0]['deferral_boundary']!='5 custom-element references are G13, not this rule': raise SystemExit('FAIL R2 deferral')
print('VIEW0_V1N1_G05_R2A_AUTHORITY_RULE_ROWS=1_OF_1')
PY2
[[ "$(awk -F '\t' 'NR>1{n++}END{print n+0}' tests/data/view0_v1n1_g05_c0_element_attribute_catalog.tsv)" == 261 ]] || fail '261 catalog'
[[ "$(awk -F '\t' 'NR>1{n++}END{print n+0}' tests/data/view0_v1n1_g05_c0_conditional_predicate_catalog.tsv)" == 43 ]] || fail '43 conditional'
version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract|head -1); [[ "$version" == '0.1-VIEW0-V1N1-G05-R2A' || "$version" == '0.1-VIEW0-V1N1-G05-R2A-SR1' || "$version" == '0.1-VIEW0-V1N1-G05-R3A' || "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]] || fail 'version'
for x in 'VIEW0_V1N1_G05_R2A_RULE_ID=0x0000000030050002' 'VIEW0_V1N1_G05_R2A_R1_IMPLEMENTED=YES' 'VIEW0_V1N1_G05_R2A_R2_IMPLEMENTED=YES' 'VIEW0_V1N1_G05_R2A_R3_IMPLEMENTED=NO' 'VIEW0_V1N1_G05_R2A_R4_IMPLEMENTED=NO' 'VIEW0_V1N1_G05_R2A_TOTAL_PUBLIC_FUNCTION_COUNT=11' 'VIEW0_V1N1_G05_R2A_SECOND_HTML_PARSER=NO' 'VIEW0_V1N1_G05_R2A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'; do grep -Fxq "$x" view/arborcore-view-core-1.contract || fail "missing $x"; done
grep -Fq '#define ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050002)' tools/include/arborcore/view0_conformance/native.h || fail 'R2 macro'
if [[ "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then
  grep -Fq '#define ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050003)' tools/include/arborcore/view0_conformance/native.h || fail 'R3 extension macro'
  grep -Fq '#define ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050004)' tools/include/arborcore/view0_conformance/native.h || fail 'R4 extension macro'
  echo 'VIEW0_V1N1_G05_R2A_CONTRACT_EXTENSION_AWARE=R4_RETAINED'
elif [[ "$version" == '0.1-VIEW0-V1N1-G05-R3A' ]]; then
  grep -Fq '#define ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050003)' tools/include/arborcore/view0_conformance/native.h || fail 'R3 macro'
  ! grep -ERq 'ARBOR_VIEW_V1_G05_BODY|0x0000000030050004' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R4 appeared'
  echo 'VIEW0_V1N1_G05_R2A_CONTRACT_EXTENSION_AWARE=R3_RETAINED'
else
  ! grep -ERq 'ARBOR_VIEW_V1_G05_(CONDITIONAL|BODY)|0x000000003005000[34]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R3/R4 appeared'
fi
echo 'VIEW0_V1N1_G05_R2A_ELEMENT_ATTRIBUTE_CATALOG=261'; echo 'VIEW0_V1N1_G05_R2A_CONDITIONAL_PREDICATES_DEFERRED_TO_R3=43'; echo 'VIEW0_V1N1_G05_R2A_CONTRACT_VERIFY=PASS'; echo 'PASS: exact R2 authority/static-pair and later-owner boundary bound'
