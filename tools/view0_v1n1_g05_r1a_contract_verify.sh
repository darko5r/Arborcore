#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1="${ARBOR_A1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }; sha(){ sha256sum -- "$1"|awk '{print $1}'; }
[[ -f "$A1" && "$(sha "$A1")" == '1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e' ]] || fail 'A1'
tmp=$(mktemp -d /tmp/arborcore-g05-r1-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
tar -xzf "$A1" -C "$tmp"; a1=$(find "$tmp" -type f -name a1-manifest.sha256 -printf '%h\n' -quit); [[ -n "$a1" ]] || fail 'A1 root'; (cd "$a1"&&sha256sum -c a1-manifest.sha256 >/dev/null)
python3 - "$a1/v1n1-g04-g06-final-matrix.tsv" <<'PY2'
import csv,sys
with open(sys.argv[1],encoding='utf-8',newline='') as f: rows=[r for r in csv.DictReader(f,delimiter='\t') if r['rule_id_hex']=='0x0000000030050001']
if len(rows)!=1 or rows[0]['rule_symbol']!='ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY' or rows[0]['severity']!='ERROR': raise SystemExit('FAIL: R1 authority')
print('VIEW0_V1N1_G05_R1A_AUTHORITY_RULE_ROWS=1_OF_1')
PY2
[[ "$(awk -F '\t' 'NR>1{n++} END{print n+0}' tests/data/view0_v1n1_g05_c0_global_attribute_catalog.tsv)" == 106 ]] || fail 'global catalog'
version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract|head -1)
[[ "$version" == '0.1-VIEW0-V1N1-G05-R1A' || "$version" == '0.1-VIEW0-V1N1-G05-R2A' || "$version" == '0.1-VIEW0-V1N1-G05-R2A-SR1' || "$version" == '0.1-VIEW0-V1N1-G05-R3A' || "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]] || fail "contract $version"
for x in 'VIEW0_V1N1_G05_R1A_RULE_ID=0x0000000030050001' 'VIEW0_V1N1_G05_R1A_R1_IMPLEMENTED=YES' 'VIEW0_V1N1_G05_R1A_TOTAL_PUBLIC_FUNCTION_COUNT=11' 'VIEW0_V1N1_G05_R1A_SECOND_HTML_PARSER=NO' 'VIEW0_V1N1_G05_R1A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'; do grep -Fxq "$x" view/arborcore-view-core-1.contract || fail "missing $x"; done
grep -Fq '#define ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050001)' tools/include/arborcore/view0_conformance/native.h || fail 'R1 macro'
if [[ "$version" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then
  grep -Fq '#define ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050002)' tools/include/arborcore/view0_conformance/native.h || fail 'R2 extension macro'
  grep -Fq '#define ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050003)' tools/include/arborcore/view0_conformance/native.h || fail 'R3 extension macro'
  grep -Fq '#define ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050004)' tools/include/arborcore/view0_conformance/native.h || fail 'R4 extension macro'
  echo 'VIEW0_V1N1_G05_R1A_CONTRACT_EXTENSION_AWARE=R4_RETAINED'
elif [[ "$version" == '0.1-VIEW0-V1N1-G05-R3A' ]]; then
  grep -Fq '#define ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050002)' tools/include/arborcore/view0_conformance/native.h || fail 'R2 extension macro'
  grep -Fq '#define ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050003)' tools/include/arborcore/view0_conformance/native.h || fail 'R3 extension macro'
  ! grep -ERq 'ARBOR_VIEW_V1_G05_BODY|0x0000000030050004' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R4 appeared'
  echo 'VIEW0_V1N1_G05_R1A_CONTRACT_EXTENSION_AWARE=R3_RETAINED'
elif [[ "$version" == *R2A* ]]; then
  grep -Fq '#define ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050002)' tools/include/arborcore/view0_conformance/native.h || fail 'R2 extension macro'
  ! grep -ERq 'ARBOR_VIEW_V1_G05_(CONDITIONAL|BODY)|0x000000003005000[34]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R3/R4 appeared'
  echo 'VIEW0_V1N1_G05_R1A_CONTRACT_EXTENSION_AWARE=R2_RETAINED'
else
  ! grep -ERq 'ARBOR_VIEW_V1_G05_(ELEMENT|CONDITIONAL|BODY)|0x000000003005000[2-4]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'later G05 rule appeared'
fi
echo 'VIEW0_V1N1_G05_R1A_GLOBAL_ATTRIBUTE_CATALOG=106'; echo 'VIEW0_V1N1_G05_R1A_CONTRACT_VERIFY=PASS'; echo 'PASS: exact R1 authority retained under current admitted extension'
