#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz'
WA0_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1-WA0-implementation-surface-candidate-6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7.tar.gz'
R1A_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1N1-G04-R1A-construction-result-candidate-040c5031a639632cfd0e1d7b146c74e929d68549c42ec15d159b12f87383c1b9.tar.gz'
A1="${ARBOR_A1_BUNDLE:-$A1_DEFAULT}"; WA0="${ARBOR_WA0_BUNDLE:-$WA0_DEFAULT}"; R1A="${ARBOR_G04_R1A_BUNDLE:-$R1A_DEFAULT}"
A1_SHA='1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e'
WA0_SHA='6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7'
R1A_SHA='040c5031a639632cfd0e1d7b146c74e929d68549c42ec15d159b12f87383c1b9'
A1_INTERNAL='c09f1b58d193e72c718e45bcd24440db7a5193795b8f4133c50b927ebbdd43bd'
WA0_INTERNAL='986b6dbb831579c72fd9d747919a52db1b08e52da0177a727b01d7dd19f1a8bc'
fail(){ echo "FAIL: $*" >&2; exit 1; }
need_eq(){ [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }
sha(){ sha256sum -- "$1" | awk '{print $1}'; }
audit(){ python3 - "$1" <<'PY'
import pathlib,sys,tarfile
p=pathlib.Path(sys.argv[1])
with tarfile.open(p,'r:gz') as tf:
    ms=tf.getmembers()
    if not ms: raise SystemExit('FAIL: empty archive')
    for m in ms:
        q=pathlib.PurePosixPath(m.name)
        if not m.name or m.name.startswith('/') or q.is_absolute() or '..' in q.parts or not (m.isdir() or m.isreg()):
            raise SystemExit(f'FAIL: unsafe archive member {m.name!r}')
PY
}
contract=view/arborcore-view-core-1.contract
current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)"
[[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1B' || "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail "unsupported current contract: $current_version"
for item in \
 'VIEW0_V1N1_G04_R1B_CONTRACT_REVISION=0.1-VIEW0-V1N1-G04-R1B' \
 'VIEW0_V1N1_G04_R1B_SCOPE=OPTION_BRANCH_AND_SELECT_SOURCE_TEXT_CLOSURE_OVER_REVIEWED_R1A' \
 'VIEW0_V1N1_G04_R1B_PREDECESSOR_R1A_PATH_COUNT=18' \
 'VIEW0_V1N1_G04_R1B_PREDECESSOR_R1A_PATHLIST_SHA256=f5a60b1b8a12f87ee89db09eb854c7708dfd0a236caf0fb2c30b2fbc0be180d7' \
 'VIEW0_V1N1_G04_R1B_PREDECESSOR_R1A_MANIFEST_SHA256=d332ae431640903c61f1c96ed68217768bebc137736614aff0736a922631d129' \
 'VIEW0_V1N1_G04_R1B_PREDECESSOR_R1A_TREE=ed0c3c9fbdc3a3eecd8a513b2aaf6b9e5a3808fe' \
 'VIEW0_V1N1_G04_R1B_PREDECESSOR_R1A_LIVE_EVIDENCE_SHA256=040c5031a639632cfd0e1d7b146c74e929d68549c42ec15d159b12f87383c1b9' \
 'VIEW0_V1N1_G04_R1B_A1_ARCHIVE_SHA256=1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e' \
 'VIEW0_V1N1_G04_R1B_WA0_ARCHIVE_SHA256=6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7' \
 'VIEW0_V1N1_G04_R1B_RULE_ID=0x0000000030040001' \
 'VIEW0_V1N1_G04_R1B_RULE_SYMBOL=ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL' \
 'VIEW0_V1N1_G04_R1B_SOURCE_FINGERPRINT_SHA256=67008e43ca25b18af4eaede5e3eb6dda8958ad90a5ad4ebabd565f304b258092' \
 'VIEW0_V1N1_G04_R1B_CLOSURE_AUTHORITY_SHA256=e483818259bd2fabeb9782044b225d8c3baafde7af3d332fec6a540af654aea4' \
 'VIEW0_V1N1_G04_R1B_OBSERVER_BOUNDARY_SHA256=4d262844d123de37dca35bbb6e7b82f167c80ab82d3947935adaf33f2879d86e' \
 'VIEW0_V1N1_G04_R1B_FIXTURE_PLAN_SHA256=bb37cc113a5d95fff3bb2dac743f76cf1b54ade73b351292b665a7aa2071679e' \
 'VIEW0_V1N1_G04_R1B_FIXTURE_PLAN_ROWS=19' \
 'VIEW0_V1N1_G04_R1B_OPTION_SOURCE_SHA256=c8811a2931bdeb4d88303b65e83c62413534146cf85673fc4e8537958c91b0bb' \
 'VIEW0_V1N1_G04_R1B_NOSCRIPT_SOURCE_SHA256=41e5f6fb57bf5d3e77b42562a5044858dd6eced91ae7c7405862330247375cd8' \
 'VIEW0_V1N1_G04_R1B_CUSTOM_CORE_SOURCE_SHA256=ccc83b8961f62cbcbe114e3c4a92cf6c204298e2e708108f61b4b9b349f64c6c' \
 'VIEW0_V1N1_G04_R1B_OPTION_BRANCH=BOUND_FROM_AUTHORED_LABEL_VALUE_ATTRIBUTES_AND_DATALIST_ANCESTRY' \
 'VIEW0_V1N1_G04_R1B_OPTION_DEFERRAL_FLAG_PUBLICATION=RETIRED_ZERO_ON_SUCCESS_PATHS' \
 'VIEW0_V1N1_G04_R1B_SELECT_FAMILY_SOURCE_TEXT=BOUND_FROM_PINNED_LEXBOR_TOKENIZER_TEXT_AND_PRE_REPAIR_CURRENT_CONTEXT' \
 'VIEW0_V1N1_G04_R1B_CHARACTER_REFERENCE_TEXT=VALIDATED_AFTER_PINNED_TOKENIZER_DECODING' \
 'VIEW0_V1N1_G04_R1B_PRIVATE_OBSERVER_POINTER_RETENTION=ZERO' \
 'VIEW0_V1N1_G04_R1B_SOURCE_ATTRIBUTE_OBSERVER_FAILURE_ATOMICITY=REQUIRED' \
 'VIEW0_V1N1_G04_R1B_SOURCE_TEXT_OBSERVER_FAILURE_ATOMICITY=REQUIRED' \
 'VIEW0_V1N1_G04_R1B_NOSCRIPT_SCRIPTING_MODE=RESIDUAL_NO_WARNING_EXPLICIT_SCRIPTING_MODE_REQUIRED' \
 'VIEW0_V1N1_G04_R1B_AUTONOMOUS_CUSTOM_TRANSPARENCY=RESIDUAL_NO_WARNING_G13_CUSTOM_IDENTITY_OWNER' \
 'VIEW0_V1N1_G04_R1B_PARENTLESS_TRANSPARENT_FALLBACK=NOT_IN_R1B_G04_R2_OWNER' \
 'VIEW0_V1N1_G04_R1B_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G04_R1B_LEXBOR_SOURCE_MUTATION=NO' \
 'VIEW0_V1N1_G04_R1B_PRODUCTION_VIEW_API_CHANGE=NO' \
 'VIEW0_V1N1_G04_R1B_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G04_R1B_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G04_R1B_RUNTIME_GLOBAL_REGISTRY=NO' \
 'VIEW0_V1N1_G04_R1B_TRANSPARENT_FRAME_BYTES=32' \
 'VIEW0_V1N1_G04_R1B_EVALUATOR_WORKSPACE_BOUND_BYTES=1048576' \
 'VIEW0_V1N1_G04_R1B_PHASED_COMPILED_STACK_BOUND_BYTES=900000' \
 'VIEW0_V1N1_G04_R1B_STACK_THRESHOLD_WIDENING=NO' \
 'VIEW0_V1N1_G04_R1B_COMPILER_PROVENANCE=REQUIRED_IN_QUALIFICATION_EVIDENCE' \
 'VIEW0_V1N1_G04_R1B_R1_IMPLEMENTATION_COMPLETE=NO_NOSCRIPT_G13_RESIDUAL' \
 'VIEW0_V1N1_G04_R1B_G04_R2_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G04_R1B_G04_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G04_R1B_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fxq "$item" "$contract" || fail "missing G04 R1B contract marker: $item"
done
need_eq "$(sha tests/data/view0_v1n1_g04_r1b_closure_authority.tsv)" 'e483818259bd2fabeb9782044b225d8c3baafde7af3d332fec6a540af654aea4' 'R1B closure authority'
need_eq "$(sha tests/data/view0_v1n1_g04_r1b_observer_boundary.tsv)" '4d262844d123de37dca35bbb6e7b82f167c80ab82d3947935adaf33f2879d86e' 'R1B observer boundary'
need_eq "$(sha tests/data/view0_v1n1_g04_r1b_fixture_plan.tsv)" 'bb37cc113a5d95fff3bb2dac743f76cf1b54ade73b351292b665a7aa2071679e' 'R1B fixture plan'
for p in "$A1" "$WA0" "$R1A"; do [[ -f "$p" ]] || fail "missing required archive: $p"; audit "$p"; done
need_eq "$(sha "$A1")" "$A1_SHA" 'A1 archive'; need_eq "$(sha "$WA0")" "$WA0_SHA" 'WA0 archive'; need_eq "$(sha "$R1A")" "$R1A_SHA" 'R1A live evidence archive'
tmp=$(mktemp -d /tmp/arborcore-g04-r1b-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/a1" "$tmp/wa0" "$tmp/r1a"
tar -xzf "$A1" -C "$tmp/a1"; tar -xzf "$WA0" -C "$tmp/wa0"; tar -xzf "$R1A" -C "$tmp/r1a"
a1root=$(find "$tmp/a1" -mindepth 1 -maxdepth 1 -type d | head -1); wa0root=$(find "$tmp/wa0" -mindepth 1 -maxdepth 1 -type d | head -1); r1aroot=$(find "$tmp/r1a" -mindepth 1 -maxdepth 1 -type d | head -1)
need_eq "$(sha "$a1root/a1-manifest.sha256")" "$A1_INTERNAL" 'A1 internal manifest'; (cd "$a1root" && sha256sum -c a1-manifest.sha256 >/dev/null)
need_eq "$(sha "$wa0root/wa0-manifest.sha256")" "$WA0_INTERNAL" 'WA0 internal manifest'; (cd "$wa0root" && sha256sum -c wa0-manifest.sha256 >/dev/null)
(cd "$r1aroot" && sha256sum -c evidence-manifest.sha256 >/dev/null)
need_eq "$(sha "$a1root/general-sections/G04-transparent-content-line-14075.whatwg-source")" '67008e43ca25b18af4eaede5e3eb6dda8958ad90a5ad4ebabd565f304b258092' 'G04 R1 source'
need_eq "$(sha "$a1root/element-sections/092-option.whatwg-source")" 'c8811a2931bdeb4d88303b65e83c62413534146cf85673fc4e8537958c91b0bb' 'option source'
need_eq "$(sha "$a1root/element-sections/104-noscript.whatwg-source")" '41e5f6fb57bf5d3e77b42562a5044858dd6eced91ae7c7405862330247375cd8' 'noscript source'
need_eq "$(sha "$a1root/element-sections/108-custom-elements-core-concepts.whatwg-source")" 'ccc83b8961f62cbcbe114e3c4a92cf6c204298e2e708108f61b4b9b349f64c6c' 'custom core source'
need_eq "$(cat "$r1aroot/candidate-tree.txt")" 'ed0c3c9fbdc3a3eecd8a513b2aaf6b9e5a3808fe' 'R1A candidate tree evidence'
need_eq "$(sha "$r1aroot/candidate-paths.txt")" 'f5a60b1b8a12f87ee89db09eb854c7708dfd0a236caf0fb2c30b2fbc0be180d7' 'R1A pathlist evidence'
need_eq "$(sha "$r1aroot/candidate-manifest.sha256")" 'd332ae431640903c61f1c96ed68217768bebc137736614aff0736a922631d129' 'R1A manifest evidence'
python3 - "$a1root/v1n1-g04-g06-final-matrix.tsv" <<'PY'
import csv,sys
with open(sys.argv[1],newline='',encoding='utf-8') as f: rows=list(csv.DictReader(f,delimiter='\t'))
r=[x for x in rows if x['rule_id_hex']=='0x0000000030040001']
if len(r)!=1 or r[0]['deferral_boundary']!='NONE': raise SystemExit('FAIL: G04 R1 frozen authority drift')
print('VIEW0_V1N1_G04_R1B_AUTHORITY_RULE_ROWS=1_OF_1')
PY
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then
  [[ -f tools/c/view0_conformance/g04_r2a.c && -f tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 extension missing under R2 contract'
else
  [[ ! -e tools/c/view0_conformance/g04_r2a.c && ! -e tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 appeared before R2'
fi
echo 'VIEW0_V1N1_G04_R1B_CONTRACT_VERIFY=PASS'
echo 'PASS: exact A1/WA0/R1A authority and R1B residual/observer contract are bound'
