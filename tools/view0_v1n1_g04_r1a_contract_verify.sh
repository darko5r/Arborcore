#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz'
WA0_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1-WA0-implementation-surface-candidate-6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7.tar.gz'
A1="${ARBOR_A1_BUNDLE:-$A1_DEFAULT}"
WA0="${ARBOR_WA0_BUNDLE:-$WA0_DEFAULT}"
A1_SHA='1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e'
WA0_SHA='6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7'
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
[[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1A' || "$current_version" == '0.1-VIEW0-V1N1-G04-R1B' || "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail "unsupported current contract: $current_version"
for item in \
 'VIEW0_V1N1_G04_R1A_CONTRACT_REVISION=0.1-VIEW0-V1N1-G04-R1A' \
 'VIEW0_V1N1_G04_R1A_SCOPE=PARTIAL_TRANSPARENT_PARENT_MODEL_OVER_ACCEPTED_G03_R7A_SR2' \
 'VIEW0_V1N1_G04_R1A_BASE_COMMIT=115d5dcee8e755edcfd0f5c447f9cfc9a0e38893' \
 'VIEW0_V1N1_G04_R1A_BASE_TREE=91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f' \
 'VIEW0_V1N1_G04_R1A_A0_WAVE_A_MATRIX_SHA256=00aba77d7dbece5f7e2929b88d7393be7fb07e79ee061e4df331c18e6905e3c2' \
 'VIEW0_V1N1_G04_R1A_A1_ARCHIVE_SHA256=1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e' \
 'VIEW0_V1N1_G04_R1A_WA0_ARCHIVE_SHA256=6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7' \
 'VIEW0_V1N1_G04_R1A_RULE_ID=0x0000000030040001' \
 'VIEW0_V1N1_G04_R1A_RULE_SYMBOL=ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL' \
 'VIEW0_V1N1_G04_R1A_SEVERITY=ERROR' \
 'VIEW0_V1N1_G04_R1A_SOURCE_FINGERPRINT_SHA256=67008e43ca25b18af4eaede5e3eb6dda8958ad90a5ad4ebabd565f304b258092' \
 'VIEW0_V1N1_G04_R1A_REQUIREMENT=TRANSPARENT_CONTENT_MODEL_DERIVED_FROM_CONTAINING_PARENT_PART_ITERATIVELY' \
 'VIEW0_V1N1_G04_R1A_DIAGNOSTIC_ANCHOR=AUTHORED_INVALID_CHILD_START_TAG_NAME' \
 'VIEW0_V1N1_G04_R1A_SOURCE_BOUNDARY_SHA256=1f1b87b58dfe965f97bdd09d9de829495891c7edc0e0535e5a3a563da4517075' \
 'VIEW0_V1N1_G04_R1A_TRANSPARENT_SURFACE_SHA256=f7c4133f0717cbdae7abdd4a51ebee96d11faeda25e1606c40420eb9b5c1abaa' \
 'VIEW0_V1N1_G04_R1A_TRANSPARENT_SURFACE_ROWS=12' \
 'VIEW0_V1N1_G04_R1A_FIXTURE_PLAN_SHA256=5883921db67784ae2305dbdb863a53a983fbe596a3be814f2a9d33b38559a8c5' \
 'VIEW0_V1N1_G04_R1A_FIXTURE_PLAN_ROWS=13' \
 'VIEW0_V1N1_G04_R1A_FROZEN_MATRIX_FIXTURES=2_OF_2' \
 'VIEW0_V1N1_G04_R1A_ITERATIVE_TRANSPARENT_RESOLUTION=YES' \
 'VIEW0_V1N1_G04_R1A_PARSER_REPAIR_SOURCE_CONTEXT=C0_SR1_SAME_PINNED_LEXBOR_TREE_BUILDER' \
 'VIEW0_V1N1_G04_R1A_PRIOR_OWNER_SUPPRESSION=G03_R1_R5_ERROR_ANCHORS' \
 'VIEW0_V1N1_G04_R1A_RESULT_FLAG_PARTIAL=0x0000000000400000' \
 'VIEW0_V1N1_G04_R1A_RESULT_FLAG_NOSCRIPT_SCRIPTING_DEFERRED=0x0000000000800000' \
 'VIEW0_V1N1_G04_R1A_RESULT_FLAG_OPTION_BRANCH_DEFERRED=0x0000000001000000' \
 'VIEW0_V1N1_G04_R1A_RESULT_FLAG_G13_CUSTOM_DEFERRED=0x0000000002000000' \
 'VIEW0_V1N1_G04_R1A_NOSCRIPT_SCRIPTING_MODE=DEFER_NO_WARNING_UNTIL_EXPLICIT_CHECKER_MODE_EXISTS' \
 'VIEW0_V1N1_G04_R1A_OPTION_TRANSPARENT_DIV_ATTRIBUTE_DATALIST_BRANCH=DEFER_NO_WARNING_UNTIL_AUTHORED_BRANCH_STATE_IS_BOUND' \
 'VIEW0_V1N1_G04_R1A_AUTONOMOUS_CUSTOM_TRANSPARENCY=DEFER_NO_WARNING_TO_G13_OWNER' \
 'VIEW0_V1N1_G04_R1A_SELECT_FAMILY_TRANSPARENT_NONWHITESPACE_TEXT=STATIC_PARTIAL_NO_G04_WARNING_SOURCE_TEXT_TOKEN_PROVENANCE_NOT_YET_BOUND' \
 'VIEW0_V1N1_G04_R1A_PARENTLESS_TRANSPARENT_FALLBACK=NOT_IN_R1A_G04_R2_OWNER' \
 'VIEW0_V1N1_G04_R1A_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G04_R1A_LEXBOR_SOURCE_MUTATION=NO' \
 'VIEW0_V1N1_G04_R1A_PRODUCTION_VIEW_API_CHANGE=NO' \
 'VIEW0_V1N1_G04_R1A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G04_R1A_RUNTIME_GLOBAL_REGISTRY=NO' \
 'VIEW0_V1N1_G04_R1A_EVALUATOR_WORKSPACE_BOUND_BYTES=1048576' \
 'VIEW0_V1N1_G04_R1A_GLOBAL_FAILURE_ATOMICITY=SR2_TWO_PASS_EXTENDED_WITH_G04_ANCHOR_COLLECTION' \
 'VIEW0_V1N1_G04_R1A_LAST_FALLIBLE_OPERATION=EXACT_FAILURE_ATOMIC_LEXBOR_PARSE_PUBLICATION' \
 'VIEW0_V1N1_G04_R1A_IMPLEMENTATION_COMPLETE=NO_NOSCRIPT_OPTION_G13_AND_SELECT_TEXT_PARTIAL' \
 'VIEW0_V1N1_G04_R1A_G04_R2_IMPLEMENTED=NO' \
 'VIEW0_V1N1_G04_R1A_G04_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G04_R1A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G04_R1A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'VIEW0_V1N1_G04_R1A_PRIOR_OWNER_ANCHOR_RECORD_BYTES=8' \
 'VIEW0_V1N1_G04_R1A_PRIOR_OWNER_ANCHOR_WORKSPACE_BYTES=32768' \
 'VIEW0_V1N1_G04_R1A_TRANSPARENT_FRAME_BYTES=24' \
 'VIEW0_V1N1_G04_R1A_PHASED_COMPILED_STACK_BOUND_BYTES=900000' \
 'VIEW0_V1N1_G04_R1A_STACK_THRESHOLD_WIDENING=NO'; do
  grep -Fxq "$item" "$contract" || fail "missing G04 R1A contract marker: $item"
done
need_eq "$(sha tests/data/view0_v1n1_g04_r1a_source_boundary.tsv)" '1f1b87b58dfe965f97bdd09d9de829495891c7edc0e0535e5a3a563da4517075' 'source boundary'
need_eq "$(sha tests/data/view0_v1n1_g04_r1a_transparent_surface.tsv)" 'f7c4133f0717cbdae7abdd4a51ebee96d11faeda25e1606c40420eb9b5c1abaa' 'transparent surface'
need_eq "$(sha tests/data/view0_v1n1_g04_r1a_fixture_plan.tsv)" '5883921db67784ae2305dbdb863a53a983fbe596a3be814f2a9d33b38559a8c5' 'fixture plan'
[[ -f "$A1" ]] || fail "missing A1 archive: $A1"
[[ -f "$WA0" ]] || fail "missing WA0 archive: $WA0"
need_eq "$(sha "$A1")" "$A1_SHA" 'A1 archive'
need_eq "$(sha "$WA0")" "$WA0_SHA" 'WA0 archive'
audit "$A1"; audit "$WA0"
tmp=$(mktemp -d /tmp/arborcore-g04-r1a-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/a1" "$tmp/wa0"
tar -xzf "$A1" -C "$tmp/a1"; tar -xzf "$WA0" -C "$tmp/wa0"
a1root=$(find "$tmp/a1" -mindepth 1 -maxdepth 1 -type d | head -1)
wa0root=$(find "$tmp/wa0" -mindepth 1 -maxdepth 1 -type d | head -1)
need_eq "$(sha "$a1root/a1-manifest.sha256")" "$A1_INTERNAL" 'A1 manifest'
need_eq "$(sha "$wa0root/wa0-manifest.sha256")" "$WA0_INTERNAL" 'WA0 manifest'
(cd "$a1root" && sha256sum -c a1-manifest.sha256 >/dev/null)
(cd "$wa0root" && sha256sum -c wa0-manifest.sha256 >/dev/null)
need_eq "$(sha "$a1root/general-sections/G04-transparent-content-line-14075.whatwg-source")" '67008e43ca25b18af4eaede5e3eb6dda8958ad90a5ad4ebabd565f304b258092' 'G04 normative source'
need_eq "$(sha "$wa0root/wa0-g04-element-context-surface.tsv")" 'fd082fe8426a71ec893c9df21e882d4534b6b34d32fa7bff552dae56d5c06bc9' 'WA0 G04 surface'
python3 - "$a1root/v1n1-g04-g06-final-matrix.tsv" "$wa0root/wa0-g04-element-context-surface.tsv" <<'PY'
import csv,sys
with open(sys.argv[1],newline='',encoding='utf-8') as f: rows=list(csv.DictReader(f,delimiter='\t'))
r=[x for x in rows if x['rule_id_hex']=='0x0000000030040001']
if len(r)!=1: raise SystemExit(f'FAIL: exact G04 R1 row count={len(r)}')
if r[0]['rule_symbol']!='ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL': raise SystemExit('FAIL: G04 R1 symbol drift')
with open(sys.argv[2],newline='',encoding='utf-8') as f: surface=list(csv.DictReader(f,delimiter='\t'))
transparent=[x for x in surface if 'transparent' in x['content_model'].lower()]
if len(transparent)!=12: raise SystemExit(f'FAIL: transparent surface count={len(transparent)} expected=12')
print('VIEW0_V1N1_G04_R1A_AUTHORITY_RULE_ROWS=1_OF_1')
print('VIEW0_V1N1_G04_R1A_WA0_TRANSPARENT_SURFACE_ROWS=12')
PY
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then
  [[ -f tools/c/view0_conformance/g04_r2a.c && -f tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 extension missing under R2 contract'
else
  [[ ! -e tools/c/view0_conformance/g04_r2a.c && ! -e tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 implementation appeared before R2'
fi
echo "VIEW0_V1N1_G04_R1A_CONTRACT_VERIFY=PASS_CURRENT_$current_version"
echo 'PASS: exact A1/WA0 G04 R1A authority retained under current admitted extension'
