#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz'
WA0_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1-WA0-implementation-surface-candidate-6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7.tar.gz'
R1B_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1N1-G04-R1B-construction-result-candidate-d8c5f1c4727400be32ec7b03cfb0b3f0394cb901e66de5a97e848aa364b74eee.tar.gz'
A1="${ARBOR_A1_BUNDLE:-$A1_DEFAULT}"; WA0="${ARBOR_WA0_BUNDLE:-$WA0_DEFAULT}"; R1B="${ARBOR_G04_R1B_BUNDLE:-$R1B_DEFAULT}"
A1_SHA='1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e'
WA0_SHA='6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7'
R1B_SHA='d8c5f1c4727400be32ec7b03cfb0b3f0394cb901e66de5a97e848aa364b74eee'
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
[[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail "unsupported current contract: $current_version"
for item in \
 'VIEW0_V1N1_G04_R1C_CONTRACT_REVISION=0.1-VIEW0-V1N1-G04-R1C' \
 'VIEW0_V1N1_G04_R1C_SCOPE=EXPLICIT_SCRIPTING_DISABLED_NOSCRIPT_TRANSPARENT_CLOSURE_OVER_REVIEWED_R1B' \
 'VIEW0_V1N1_G04_R1C_BASE_COMMIT=115d5dcee8e755edcfd0f5c447f9cfc9a0e38893' \
 'VIEW0_V1N1_G04_R1C_BASE_TREE=91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f' \
 'VIEW0_V1N1_G04_R1C_PREDECESSOR_R1B_PATH_COUNT=28' \
 'VIEW0_V1N1_G04_R1C_PREDECESSOR_R1B_PATHLIST_SHA256=62a539a7f5fa5f64334c5d06d8cc262a19149dd9b7ec153cb85b949d473d69a4' \
 'VIEW0_V1N1_G04_R1C_PREDECESSOR_R1B_MANIFEST_SHA256=bc36b5484442b9c29ef7040139f698a399199adfe3caf3654f307ebaa08af485' \
 'VIEW0_V1N1_G04_R1C_PREDECESSOR_R1B_TREE=b1b6f2dd9dabfd7b6fb33c9a910cdb92a382f8bc' \
 'VIEW0_V1N1_G04_R1C_PREDECESSOR_R1B_LIVE_EVIDENCE_SHA256=d8c5f1c4727400be32ec7b03cfb0b3f0394cb901e66de5a97e848aa364b74eee' \
 'VIEW0_V1N1_G04_R1C_A1_ARCHIVE_SHA256=1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e' \
 'VIEW0_V1N1_G04_R1C_WA0_ARCHIVE_SHA256=6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7' \
 'VIEW0_V1N1_G04_R1C_RULE_ID=0x0000000030040001' \
 'VIEW0_V1N1_G04_R1C_RULE_SYMBOL=ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL' \
 'VIEW0_V1N1_G04_R1C_SEVERITY=ERROR' \
 'VIEW0_V1N1_G04_R1C_SOURCE_FINGERPRINT_SHA256=67008e43ca25b18af4eaede5e3eb6dda8958ad90a5ad4ebabd565f304b258092' \
 'VIEW0_V1N1_G04_R1C_NOSCRIPT_SOURCE_SHA256=41e5f6fb57bf5d3e77b42562a5044858dd6eced91ae7c7405862330247375cd8' \
 'VIEW0_V1N1_G04_R1C_CUSTOM_CORE_SOURCE_SHA256=ccc83b8961f62cbcbe114e3c4a92cf6c204298e2e708108f61b4b9b349f64c6c' \
 'VIEW0_V1N1_G04_R1C_LEXBOR_PARSER_H_SHA256=a69ace318e92fa41f9631c1666836b11ba0b89067ffde8e2dd9937b2c2da5f48' \
 'VIEW0_V1N1_G04_R1C_LEXBOR_COMMIT=2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' \
 'VIEW0_V1N1_G04_R1C_SCRIPTING_AUTHORITY_SHA256=97f820c2d30470438a3f78768d22e46d1fe4818ac3e27c7c15583c00c69db81a' \
 'VIEW0_V1N1_G04_R1C_FIXTURE_PLAN_SHA256=49727eb3791d5ce6b6bd51d37324e4bd2a0f94f95d0766b2fa5e8606b6334cb8' \
 'VIEW0_V1N1_G04_R1C_FIXTURE_PLAN_ROWS=12' \
 'VIEW0_V1N1_G04_R1C_CHECKER_SCRIPTING_MODE=DISABLED' \
 'VIEW0_V1N1_G04_R1C_LEXBOR_SCRIPTING_MODE_SET_EXPLICITLY=YES' \
 'VIEW0_V1N1_G04_R1C_LEXBOR_SCRIPTING_MODE_READBACK_REQUIRED=DISABLED' \
 'VIEW0_V1N1_G04_R1C_ENABLED_SCRIPTING_MODE=NOT_EXPOSED_REQUIRES_CROSS_EVALUATOR_REVIEW' \
 'VIEW0_V1N1_G04_R1C_NOSCRIPT_OUTSIDE_HEAD=TRANSPARENT' \
 'VIEW0_V1N1_G04_R1C_NOSCRIPT_IN_HEAD=NONTRANSPARENT_G04_R1_NOT_APPLICABLE' \
 'VIEW0_V1N1_G04_R1C_NOSCRIPT_SELECT_PARENT_MODEL=SELECT_TAIL' \
 'VIEW0_V1N1_G04_R1C_NOSCRIPT_OPTGROUP_PARENT_MODEL=OPTGROUP_TAIL' \
 'VIEW0_V1N1_G04_R1C_NOSCRIPT_OPTION_NO_LABEL_NON_DATALIST_PARENT_PART=PHRASING' \
 'VIEW0_V1N1_G04_R1C_NOSCRIPT_DEFERRAL_FLAG_PUBLICATION=RETIRED_ZERO_ON_SUCCESS_PATHS' \
 'VIEW0_V1N1_G04_R1C_STANDARD_ELEMENT_R1_COVERAGE=COMPLETE_FOR_FROZEN_SCRIPTING_DISABLED_CHECKER_MODE' \
 'VIEW0_V1N1_G04_R1C_AUTONOMOUS_CUSTOM_TRANSPARENCY=EXTERNAL_DEPENDENCY_G13_CUSTOM_IDENTITY_OWNER' \
 'VIEW0_V1N1_G04_R1C_R1_IMPLEMENTATION_COMPLETE=NO_G13_EXTERNAL_DEPENDENCY' \
 'VIEW0_V1N1_G04_R1C_R1_STANDARD_ELEMENT_IMPLEMENTATION_COMPLETE=YES' \
 'VIEW0_V1N1_G04_R1C_G04_R2_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G04_R1C_G04_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G04_R1C_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G04_R1C_LEXBOR_SOURCE_MUTATION=NO' \
 'VIEW0_V1N1_G04_R1C_PRODUCTION_VIEW_API_CHANGE=NO' \
 'VIEW0_V1N1_G04_R1C_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G04_R1C_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G04_R1C_RUNTIME_GLOBAL_REGISTRY=NO' \
 'VIEW0_V1N1_G04_R1C_EVALUATOR_WORKSPACE_BOUND_BYTES=1048576' \
 'VIEW0_V1N1_G04_R1C_PHASED_COMPILED_STACK_BOUND_BYTES=900000' \
 'VIEW0_V1N1_G04_R1C_STACK_THRESHOLD_WIDENING=NO' \
 'VIEW0_V1N1_G04_R1C_STACK_RAW_MEASUREMENTS=TOOLCHAIN_PROVENANCE_EVIDENCE_NOT_CROSS_TOOLCHAIN_ARCHIVE_IDENTITY' \
 'VIEW0_V1N1_G04_R1C_GLOBAL_FAILURE_ATOMICITY=SR2_TWO_PASS_EXTENDED_G04_ANCHOR_COLLECTION_RETAINED' \
 'VIEW0_V1N1_G04_R1C_LAST_FALLIBLE_OPERATION=EXACT_FAILURE_ATOMIC_LEXBOR_PARSE_PUBLICATION' \
 'VIEW0_V1N1_G04_R1C_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fxq "$item" "$contract" || fail "missing G04 R1C contract marker: $item"
done
need_eq "$(sha tests/data/view0_v1n1_g04_r1c_scripting_authority.tsv)" '97f820c2d30470438a3f78768d22e46d1fe4818ac3e27c7c15583c00c69db81a' 'R1C scripting authority'
need_eq "$(sha tests/data/view0_v1n1_g04_r1c_fixture_plan.tsv)" '49727eb3791d5ce6b6bd51d37324e4bd2a0f94f95d0766b2fa5e8606b6334cb8' 'R1C fixture plan'
for p in "$A1" "$WA0" "$R1B"; do [[ -f "$p" ]] || fail "missing required archive: $p"; audit "$p"; done
need_eq "$(sha "$A1")" "$A1_SHA" 'A1 archive'; need_eq "$(sha "$WA0")" "$WA0_SHA" 'WA0 archive'; need_eq "$(sha "$R1B")" "$R1B_SHA" 'R1B live evidence archive'
tmp=$(mktemp -d /tmp/arborcore-g04-r1c-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/a1" "$tmp/wa0" "$tmp/r1b"
tar -xzf "$A1" -C "$tmp/a1"; tar -xzf "$WA0" -C "$tmp/wa0"; tar -xzf "$R1B" -C "$tmp/r1b"
a1root=$(find "$tmp/a1" -mindepth 1 -maxdepth 1 -type d | head -1); wa0root=$(find "$tmp/wa0" -mindepth 1 -maxdepth 1 -type d | head -1); r1broot=$(find "$tmp/r1b" -mindepth 1 -maxdepth 1 -type d | head -1)
need_eq "$(sha "$a1root/a1-manifest.sha256")" "$A1_INTERNAL" 'A1 internal manifest'; (cd "$a1root" && sha256sum -c a1-manifest.sha256 >/dev/null)
need_eq "$(sha "$wa0root/wa0-manifest.sha256")" "$WA0_INTERNAL" 'WA0 internal manifest'; (cd "$wa0root" && sha256sum -c wa0-manifest.sha256 >/dev/null)
(cd "$r1broot" && sha256sum -c evidence-manifest.sha256 >/dev/null)
need_eq "$(sha "$a1root/general-sections/G04-transparent-content-line-14075.whatwg-source")" '67008e43ca25b18af4eaede5e3eb6dda8958ad90a5ad4ebabd565f304b258092' 'G04 R1 source'
need_eq "$(sha "$a1root/element-sections/104-noscript.whatwg-source")" '41e5f6fb57bf5d3e77b42562a5044858dd6eced91ae7c7405862330247375cd8' 'noscript source'
need_eq "$(sha "$a1root/element-sections/108-custom-elements-core-concepts.whatwg-source")" 'ccc83b8961f62cbcbe114e3c4a92cf6c204298e2e708108f61b4b9b349f64c6c' 'custom core source'
need_eq "$(cat "$r1broot/cumulative-candidate-tree.txt")" 'b1b6f2dd9dabfd7b6fb33c9a910cdb92a382f8bc' 'R1B candidate tree evidence'
need_eq "$(sha "$r1broot/cumulative-candidate-paths.txt")" '62a539a7f5fa5f64334c5d06d8cc262a19149dd9b7ec153cb85b949d473d69a4' 'R1B pathlist evidence'
need_eq "$(sha "$r1broot/cumulative-candidate-manifest.sha256")" 'bc36b5484442b9c29ef7040139f698a399199adfe3caf3654f307ebaa08af485' 'R1B manifest evidence'
LEX=build/view0-v1/native/lexbor-src
[[ -d "$LEX/.git" ]] || fail 'pinned Lexbor source missing'
need_eq "$(git -C "$LEX" rev-parse HEAD)" '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' 'Lexbor commit'
need_eq "$(sha "$LEX/source/lexbor/html/parser.h")" 'a69ace318e92fa41f9631c1666836b11ba0b89067ffde8e2dd9937b2c2da5f48' 'Lexbor parser.h'
grep -Fq 'lxb_html_parser_scripting_set(parser, false);' tools/c/view0_conformance/lexbor_adapter.c || fail 'explicit scripting-disabled setter missing'
grep -Fq 'lxb_html_parser_scripting(parser)' tools/c/view0_conformance/lexbor_adapter.c || fail 'scripting-mode readback missing'
python3 - "$a1root/v1n1-g04-g06-final-matrix.tsv" <<'PY'
import csv,sys
with open(sys.argv[1],newline='',encoding='utf-8') as f: rows=list(csv.DictReader(f,delimiter='\t'))
r=[x for x in rows if x['rule_id_hex']=='0x0000000030040001']
if len(r)!=1 or r[0]['deferral_boundary']!='NONE': raise SystemExit('FAIL: G04 R1 frozen authority drift')
print('VIEW0_V1N1_G04_R1C_AUTHORITY_RULE_ROWS=1_OF_1')
PY
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then
  [[ -f tools/c/view0_conformance/g04_r2a.c && -f tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 extension missing under R2 contract'
else
  [[ ! -e tools/c/view0_conformance/g04_r2a.c && ! -e tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 implementation appeared before R2'
fi
echo 'VIEW0_V1N1_G04_R1C_CONTRACT_VERIFY=PASS'
echo 'PASS: exact A1/WA0/R1B/noscript/Lexbor authority and R1C scripting-mode contract are bound'
