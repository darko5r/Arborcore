#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1="${ARBOR_A1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz}"
WA0="${ARBOR_WA0_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-WA0-implementation-surface-candidate-6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7.tar.gz}"
GF1="${ARBOR_G04_GF1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G04-GF1-group-freeze-candidate-889994f122452dd8b9acce0825fc3d29d2b93644916bd34dc13dc08932db039b.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }
eq(){ [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }
sha(){ sha256sum -- "$1" | awk '{print $1}'; }
for t in sha256sum tar python3 grep awk sed find sort wc mktemp; do command -v "$t" >/dev/null || fail "missing $t"; done
[[ -f "$A1" && -f "$WA0" && -f "$GF1" ]] || fail 'required authority archive missing'
eq "$(sha "$A1")" '1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e' 'A1 SHA'
eq "$(sha "$WA0")" '6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7' 'WA0 SHA'
eq "$(sha "$GF1")" '889994f122452dd8b9acce0825fc3d29d2b93644916bd34dc13dc08932db039b' 'G04 GF1 SHA'
tmp=$(mktemp -d /tmp/arborcore-g05-c0-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/a1" "$tmp/wa0" "$tmp/gf1"; tar -xzf "$A1" -C "$tmp/a1"; tar -xzf "$WA0" -C "$tmp/wa0"; tar -xzf "$GF1" -C "$tmp/gf1"
a1=$(find "$tmp/a1" -mindepth 1 -maxdepth 1 -type d | head -1); wa0=$(find "$tmp/wa0" -mindepth 1 -maxdepth 1 -type d | head -1); gf1=$(find "$tmp/gf1" -mindepth 1 -maxdepth 1 -type d | head -1)
eq "$(sha "$a1/a1-manifest.sha256")" 'c09f1b58d193e72c718e45bcd24440db7a5193795b8f4133c50b927ebbdd43bd' 'A1 manifest'
(cd "$a1" && sha256sum -c a1-manifest.sha256 >/dev/null)
eq "$(sha "$wa0/wa0-manifest.sha256")" '986b6dbb831579c72fd9d747919a52db1b08e52da0177a727b01d7dd19f1a8bc' 'WA0 manifest'
(cd "$wa0" && sha256sum -c wa0-manifest.sha256 >/dev/null)
eq "$(sha "$gf1/g04-gf1-freeze-manifest.sha256")" '32ecfa60bb13512cdde35c685edca8205b3786b67117cc93c7949ae9da0f2363' 'G04 GF1 manifest'
(cd "$gf1" && sha256sum -c g04-gf1-freeze-manifest.sha256 >/dev/null)
eq "$(sha tests/data/view0_v1n1_g05_c0_body_window_event_catalog.tsv)" '552968394e7bf104adea0dec315fbfea7a38c4289381f42d47a9941e637f3086' 'view0_v1n1_g05_c0_body_window_event_catalog.tsv SHA'
if [[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == '0.1-VIEW0-V1N1-G05-R2A-SR1' || "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == '0.1-VIEW0-V1N1-G05-R3A' || "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then
  eq "$(sha tests/data/view0_v1n1_g05_c0_conditional_predicate_catalog.tsv)" '33329f61bbf9f22dcd69365e8f399d8caaa6df393a2d8ceac0e50ee07611257c' 'corrected conditional catalog SHA'
  eq "$(sha tests/data/view0_v1n1_g05_c0_sr1_input_state_authority.tsv)" '1edfa7f1e732b1436774262905d81c780cb793c68f36cc233099ecf2f47a7f10' 'C0-SR1 input-state authority SHA'
else
  eq "$(sha tests/data/view0_v1n1_g05_c0_conditional_predicate_catalog.tsv)" 'eb681627f34c364dbca98ff511fe61301e6dbf7acb730990ec34003aafdbfec5' 'view0_v1n1_g05_c0_conditional_predicate_catalog.tsv SHA'
fi
eq "$(sha tests/data/view0_v1n1_g05_c0_element_attribute_catalog.tsv)" '1659c6bda74cb7541f4662ac8fae64950a0f7bd94cf33afb1cc85c501c821abc' 'view0_v1n1_g05_c0_element_attribute_catalog.tsv SHA'
eq "$(sha tests/data/view0_v1n1_g05_c0_foundation_summary.txt)" '09024d41ff293192f18d255c01a36f91ede5b9d85926512f2b79ddf30e85c962' 'view0_v1n1_g05_c0_foundation_summary.txt SHA'
eq "$(sha tests/data/view0_v1n1_g05_c0_global_attribute_catalog.tsv)" '42a262de37c0b6e88c9bf21d0f5c356e78f6cc75d31fe98a85847997f5989845' 'view0_v1n1_g05_c0_global_attribute_catalog.tsv SHA'
eq "$(sha tests/data/view0_v1n1_g05_c0_rule_authority.tsv)" 'c890158f8d30c528e466ce4aabc71402a339f4117b6d994e8a3f6d4fae5196fb' 'view0_v1n1_g05_c0_rule_authority.tsv SHA'
python3 - "$a1" "$wa0" <<'PY_AUTH'
import csv,re,sys,pathlib
root=pathlib.Path(sys.argv[1]); wa=pathlib.Path(sys.argv[2])
def rd(p):
    with p.open(encoding='utf-8',newline='') as f: return list(csv.DictReader(f,delimiter='\t'))
final=[r for r in rd(root/'v1n1-g04-g06-final-matrix.tsv') if r['group']=='G05']
local=rd(pathlib.Path('tests/data/view0_v1n1_g05_c0_rule_authority.tsv'))
if final!=local or len(final)!=4: raise SystemExit('FAIL: exact G05 four-rule authority divergence')
cat=rd(wa/'wa0-g05-element-attribute-catalog.tsv')
if len(cat)!=279 or sum(r['classification']=='G05_ATTRIBUTE_APPLICABILITY_REFERENCE' for r in cat)!=261 or sum(r['classification']=='BODY_WINDOW_EVENT_HANDLER_CONTENT_ATTRIBUTE' for r in cat)!=18: raise SystemExit('FAIL: WA0 G05 static catalog counts')
cond=rd(wa/'wa0-g05-conditional-predicate-map.tsv')
if len(cond)!=43 or any(not r['predicate_family'] for r in cond): raise SystemExit('FAIL: WA0 G05 conditional map')
G=(root/'general-sections/G05-global-attributes-line-14316.whatwg-source').read_text(encoding='utf-8')
rows=rd(pathlib.Path('tests/data/view0_v1n1_g05_c0_global_attribute_catalog.tsv'))
if len(rows)!=106: raise SystemExit('FAIL: global catalog row count')
patterns=[r['attribute_pattern'] for r in rows]
for n in ('accesskey','id','class','slot','role','onclick','onwheel','data-*','aria-*','xmlns'):
    if n not in patterns: raise SystemExit('FAIL: global catalog missing '+n)
if patterns.count('data-*')!=1 or patterns.count('aria-*')!=1: raise SystemExit('FAIL: global family multiplicity')
events=re.findall(r'<code data-x="handler-[^"]+">(on[^<]+)</code>',G[G.index('The following <span>event handler content attributes</span>'):])[:71]
local_events=[r['attribute_pattern'] for r in rows if r['authority_kind']=='GENERIC_EVENT_HANDLER_CONTENT_ATTRIBUTE']
if events!=local_events or len(local_events)!=71: raise SystemExit('FAIL: generic event catalog source divergence')
print('VIEW0_V1N1_G05_C0_RULE_AUTHORITY_ROWS=4_OF_4')
print('VIEW0_V1N1_G05_C0_GLOBAL_ATTRIBUTE_CATALOG_ROWS=106')
print('VIEW0_V1N1_G05_C0_ELEMENT_ATTRIBUTE_ROWS=261')
print('VIEW0_V1N1_G05_C0_BODY_WINDOW_EVENT_ROWS=18')
print('VIEW0_V1N1_G05_C0_CONDITIONAL_PREDICATE_ROWS=43')
PY_AUTH

current=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1); [[ "$current" == '0.1-VIEW0-V1N1-G05-C0' || "$current" == '0.1-VIEW0-V1N1-G05-R1A' || "$current" == '0.1-VIEW0-V1N1-G05-R2A' || "$current" == '0.1-VIEW0-V1N1-G05-R2A-SR1' || "$current" == '0.1-VIEW0-V1N1-G05-R3A' || "$current" == '0.1-VIEW0-V1N1-G05-R4A' ]] || fail 'contract version'
for line in \
 'VIEW0_V1N1_G05_C0_G05_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G05_C0_G05_R1_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G05_C0_G05_R2_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G05_C0_G05_R3_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G05_C0_G05_R4_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G05_C0_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G05_C0_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G05_C0_RETAINED_G04_STACK_REMEASURED_AFTER_OBSERVER_LAYOUT_CHANGE=YES' \
 'VIEW0_V1N1_G05_C0_RETAINED_G04_PHASED_STACK_BOUND_BYTES=900000' \
 'VIEW0_V1N1_G05_C0_STACK_THRESHOLD_WIDENING=NO'; do grep -Fxq "$line" view/arborcore-view-core-1.contract || fail "contract marker missing $line"; done
if [[ "$current" == '0.1-VIEW0-V1N1-G05-C0' ]]; then ! grep -ERq 'ARBOR_VIEW_V1_G05_|0x000000003005000[1-4]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G05 rule appeared in exact C0'; else
  grep -Fq 'ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY' tools/include/arborcore/view0_conformance/native.h || fail 'R1A rule missing under extension'
  if [[ "$current" != '0.1-VIEW0-V1N1-G05-R1A' ]]; then grep -Fq 'ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY' tools/include/arborcore/view0_conformance/native.h || fail 'R2A rule missing under extension'; fi
  if [[ "$current" == '0.1-VIEW0-V1N1-G05-R3A' || "$current" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then grep -Fq 'ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY' tools/include/arborcore/view0_conformance/native.h || fail 'R3A rule missing under extension'; fi
  if [[ "$current" == '0.1-VIEW0-V1N1-G05-R4A' ]]; then grep -Fq 'ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY' tools/include/arborcore/view0_conformance/native.h || fail 'R4A rule missing under extension'; fi
fi
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/g05_c0.c || fail 'G05 C0 direct heap allocation'
echo 'VIEW0_V1N1_G05_C0_CONTRACT_VERIFY=PASS'
echo 'PASS: exact G04-GF1/A1/WA0 G05 foundation authority and zero-rule contract bound'
