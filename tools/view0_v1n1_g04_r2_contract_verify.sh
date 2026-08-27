#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz'
WA0_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1-WA0-implementation-surface-candidate-6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7.tar.gz'
E1_DEFAULT='/root/Downloads/Arborcore-VIEW0-V1N1-G04-R1C-SR1-E1-evidence-authority-correction-candidate-425ba21b3b599bc13b5fd0780095b36e0711ea213adb546c9e84f700ca52d03f.tar.gz'
A1="${ARBOR_A1_BUNDLE:-$A1_DEFAULT}"; WA0="${ARBOR_WA0_BUNDLE:-$WA0_DEFAULT}"; E1="${ARBOR_G04_R1C_E1_BUNDLE:-$E1_DEFAULT}"
A1_SHA='1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e'
WA0_SHA='6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7'
E1_SHA='425ba21b3b599bc13b5fd0780095b36e0711ea213adb546c9e84f700ca52d03f'
E1_INTERNAL='bc1b5632c37f908b85c02992fa67476af4b1d735b1b308ff31dea48f8e24f1c5'
fail(){ echo "FAIL: $*" >&2; exit 1; }
sha(){ sha256sum -- "$1" | awk '{print $1}'; }
need_eq(){ [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }
audit(){ python3 - "$1" <<'PY'
import pathlib,sys,tarfile
p=pathlib.Path(sys.argv[1])
with tarfile.open(p,'r:gz') as tf:
 for m in tf.getmembers():
  q=pathlib.PurePosixPath(m.name)
  if not m.name or m.name.startswith('/') or q.is_absolute() or '..' in q.parts or not (m.isdir() or m.isreg()):
   raise SystemExit('unsafe archive member '+repr(m.name))
PY
}
contract='view/arborcore-view-core-1.contract'
need_eq "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)" '0.1-VIEW0-V1N1-G04-R2' 'current contract'
for item in \
 'VIEW0_V1N1_G04_R2_CONTRACT_REVISION=0.1-VIEW0-V1N1-G04-R2' \
 'VIEW0_V1N1_G04_R2_SCOPE=EXPLICIT_FRAGMENT_MODEL_PARENTLESS_TRANSPARENT_FLOW_OVER_ACCEPTED_R1C_SR1_E1' \
 'VIEW0_V1N1_G04_R2_PREDECESSOR_R1C_PATH_COUNT=36' \
 'VIEW0_V1N1_G04_R2_PREDECESSOR_R1C_PATHLIST_SHA256=5f38e5e4058b6d00626194ca72395fc1ea3909910b36f3f25121672d35ba855e' \
 'VIEW0_V1N1_G04_R2_PREDECESSOR_R1C_MANIFEST_SHA256=3fb71710c764ae2b61f021b1c65d1bdb814d30ded3dc50105f0f269dc8252a6f' \
 'VIEW0_V1N1_G04_R2_PREDECESSOR_R1C_TREE=c935b324255be97d8fb10fc7e5ab463b1e34efa8' \
 'VIEW0_V1N1_G04_R2_PREDECESSOR_E1_ARCHIVE_SHA256=425ba21b3b599bc13b5fd0780095b36e0711ea213adb546c9e84f700ca52d03f' \
 'VIEW0_V1N1_G04_R2_PREDECESSOR_E1_INTERNAL_MANIFEST_SHA256=bc1b5632c37f908b85c02992fa67476af4b1d735b1b308ff31dea48f8e24f1c5' \
 'VIEW0_V1N1_G04_R2_RULE_ID=0x0000000030040002' \
 'VIEW0_V1N1_G04_R2_RULE_SYMBOL=ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW' \
 'VIEW0_V1N1_G04_R2_SEVERITY=ERROR' \
 'VIEW0_V1N1_G04_R2_SOURCE_FINGERPRINT_SHA256=67008e43ca25b18af4eaede5e3eb6dda8958ad90a5ad4ebabd565f304b258092' \
 'VIEW0_V1N1_G04_R2_AUTHORITY_TABLE_SHA256=8985a58d00732d13421571bc3eb51211247c5b9f777863db149fdfce4987837f' \
 'VIEW0_V1N1_G04_R2_PARENTLESS_SURFACE_SHA256=e15fcc99e1cceb864698e7a081729432ff6d8738859d8dfe2e2d18bf117afb6d' \
 'VIEW0_V1N1_G04_R2_FRAGMENT_SOURCE_BOUNDARY_SHA256=c972ccd0508e4fafc96f7dafe363f45cdbf84af843df7f229a4772fd9febf3b6' \
 'VIEW0_V1N1_G04_R2_FIXTURE_PLAN_SHA256=f8c57185210b56dd5e8398ae06858b2bd78f79c4c805872c259d3afd9ef0df08' \
 'VIEW0_V1N1_G04_R2_FRAGMENT_MODE=EXPLICIT_DEVELOPMENT_TOOL_ONLY' \
 'VIEW0_V1N1_G04_R2_FRAGMENT_CONTEXT=BODY_HTML_NAMESPACE' \
 'VIEW0_V1N1_G04_R2_FRAGMENT_CONTEXT_SYNTHETIC_WRAPPER=NOT_AN_AUTHORED_PARENT' \
 'VIEW0_V1N1_G04_R2_FRAGMENT_WHOLE_DOCUMENT_G02_G03=NOT_RUN' \
 'VIEW0_V1N1_G04_R2_CHECKER_SCRIPTING_MODE=DISABLED_RETAINED_FROM_R1C' \
 'VIEW0_V1N1_G04_R2_STANDARD_PARENTLESS_TRANSPARENT_ROOT_COUNT=10' \
 'VIEW0_V1N1_G04_R2_PARENTLESS_TRANSPARENT_PART_MODEL=FLOW_CONTENT' \
 'VIEW0_V1N1_G04_R2_ITERATIVE_TRANSPARENT_NESTING=YES' \
 'VIEW0_V1N1_G04_R2_AUTONOMOUS_CUSTOM_TRANSPARENCY=EXTERNAL_DEPENDENCY_G13_CUSTOM_IDENTITY_OWNER' \
 'VIEW0_V1N1_G04_R2_G13_CUSTOM_BEHAVIOR=DEFER_NO_R2_WARNING' \
 'VIEW0_V1N1_G04_R2_R2_STANDARD_ELEMENT_IMPLEMENTATION_COMPLETE=YES' \
 'VIEW0_V1N1_G04_R2_R2_IMPLEMENTATION_COMPLETE=NO_G13_EXTERNAL_DEPENDENCY' \
 'VIEW0_V1N1_G04_R2_G04_STANDARD_ELEMENT_IMPLEMENTATION_COMPLETE=YES_R1_DOCUMENT_AND_R2_FRAGMENT_MODES' \
 'VIEW0_V1N1_G04_R2_G04_IMPLEMENTATION_COMPLETE=NO_G13_EXTERNAL_DEPENDENCY' \
 'VIEW0_V1N1_G04_R2_G04_GROUP_FREEZE=NO_PENDING_INDEPENDENT_REVIEW' \
 'VIEW0_V1N1_G04_R2_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G04_R2_LEXBOR_SOURCE_MUTATION=NO' \
 'VIEW0_V1N1_G04_R2_PRODUCTION_VIEW_API_CHANGE=NO' \
 'VIEW0_V1N1_G04_R2_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G04_R2_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G04_R2_RUNTIME_GLOBAL_REGISTRY=NO' \
 'VIEW0_V1N1_G04_R2_EVALUATOR_WORKSPACE_BOUND_BYTES=1048576' \
 'VIEW0_V1N1_G04_R2_PHASED_COMPILED_STACK_BOUND_BYTES=900000' \
 'VIEW0_V1N1_G04_R2_STACK_THRESHOLD_WIDENING=NO' \
 'VIEW0_V1N1_G04_R2_LAST_FALLIBLE_OPERATION=EXACT_FAILURE_ATOMIC_LEXBOR_FRAGMENT_PARSE_PUBLICATION' \
 'VIEW0_V1N1_G04_R2_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
 grep -Fxq "$item" "$contract" || fail "missing G04 R2 contract marker: $item"
done
need_eq "$(sha tests/data/view0_v1n1_g04_r2_authority.tsv)" '8985a58d00732d13421571bc3eb51211247c5b9f777863db149fdfce4987837f' 'R2 authority table'
need_eq "$(sha tests/data/view0_v1n1_g04_r2_parentless_surface.tsv)" 'e15fcc99e1cceb864698e7a081729432ff6d8738859d8dfe2e2d18bf117afb6d' 'R2 parentless surface'
need_eq "$(sha tests/data/view0_v1n1_g04_r2_fragment_source_boundary.tsv)" 'c972ccd0508e4fafc96f7dafe363f45cdbf84af843df7f229a4772fd9febf3b6' 'R2 fragment source boundary'
need_eq "$(sha tests/data/view0_v1n1_g04_r2_fixture_plan.tsv)" 'f8c57185210b56dd5e8398ae06858b2bd78f79c4c805872c259d3afd9ef0df08' 'R2 fixture plan'
for p in "$A1" "$WA0" "$E1"; do [[ -f "$p" ]] || fail "missing required archive: $p"; audit "$p"; done
need_eq "$(sha "$A1")" "$A1_SHA" 'A1 archive'; need_eq "$(sha "$WA0")" "$WA0_SHA" 'WA0 archive'; need_eq "$(sha "$E1")" "$E1_SHA" 'E1 archive'
tmp=$(mktemp -d /tmp/arborcore-g04-r2-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/a1" "$tmp/wa0" "$tmp/e1"
tar -xzf "$A1" -C "$tmp/a1"; tar -xzf "$WA0" -C "$tmp/wa0"; tar -xzf "$E1" -C "$tmp/e1"
a1root=$(find "$tmp/a1" -mindepth 1 -maxdepth 1 -type d | head -1); wa0root=$(find "$tmp/wa0" -mindepth 1 -maxdepth 1 -type d | head -1); e1root=$(find "$tmp/e1" -mindepth 1 -maxdepth 1 -type d | head -1)
need_eq "$(sha "$e1root/evidence-manifest.sha256")" "$E1_INTERNAL" 'E1 internal manifest identity'; (cd "$e1root" && sha256sum -c evidence-manifest.sha256 >/dev/null)
need_eq "$(cat "$e1root/cumulative-candidate-tree.txt")" 'c935b324255be97d8fb10fc7e5ab463b1e34efa8' 'E1 predecessor candidate tree'
need_eq "$(sha "$e1root/cumulative-candidate-paths.txt")" '5f38e5e4058b6d00626194ca72395fc1ea3909910b36f3f25121672d35ba855e' 'E1 predecessor pathlist'
need_eq "$(sha "$e1root/cumulative-candidate-manifest.sha256")" '3fb71710c764ae2b61f021b1c65d1bdb814d30ded3dc50105f0f269dc8252a6f' 'E1 predecessor manifest'
grep -Fxq 'VIEW0_V1N1_G04_R1C_R1_STANDARD_ELEMENT_IMPLEMENTATION_COMPLETE=YES' "$e1root/authority.txt" || fail 'E1 R1 standard completion missing'
grep -Fxq 'VIEW0_V1N1_G04_R1C_R1_IMPLEMENTATION_COMPLETE=NO_G13_EXTERNAL_DEPENDENCY' "$e1root/authority.txt" || fail 'E1 R1 G13 boundary missing'
python3 - "$a1root" "$wa0root" <<'PY'
import csv,pathlib,sys
a1=pathlib.Path(sys.argv[1]); wa0=pathlib.Path(sys.argv[2])
with (a1/'v1n1-g04-g06-final-matrix.tsv').open(newline='',encoding='utf-8') as f:
 rows=[r for r in csv.DictReader(f,delimiter='\t') if r['rule_id_hex']=='0x0000000030040002']
if len(rows)!=1 or rows[0]['rule_symbol']!='ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW' or rows[0]['source_fingerprint_sha256']!='67008e43ca25b18af4eaede5e3eb6dda8958ad90a5ad4ebabd565f304b258092' or rows[0]['fixture_scope']!='FRAGMENT_MODEL':
 raise SystemExit('FAIL exact A1 G04 R2 authority row mismatch')
with (wa0/'wa0-wave-a-rule-identities.tsv').open(newline='',encoding='utf-8') as f:
 ids=[r for r in csv.DictReader(f,delimiter='\t') if r['rule_id_hex']=='0x0000000030040002']
if len(ids)!=1: raise SystemExit('FAIL WA0 G04 R2 identity missing')
print('VIEW0_V1N1_G04_R2_AUTHORITY_RULE_ROWS=1_OF_1')
PY
[[ "$(grep -Fc 'lxb_html_parse_fragment_chunk_begin(' tools/c/view0_conformance/lexbor_adapter.c)" -eq 1 ]] || fail 'fragment parse begin count drift'
[[ "$(grep -Fc 'lxb_html_parser_scripting_set(parser, false);' tools/c/view0_conformance/lexbor_adapter.c)" -eq 2 ]] || fail 'document+fragment scripting-disabled bind count drift'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/g04_r2a.c || fail 'direct heap allocation appeared in R2 evaluator'
echo 'VIEW0_V1N1_G04_R2_CONTRACT_VERIFY=PASS'
echo 'PASS: exact A1/WA0/E1 parentless fragment authority and R2 contract are bound'
