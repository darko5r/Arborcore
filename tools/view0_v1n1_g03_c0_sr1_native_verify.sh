#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src
RELEASE=build/view0-v1/native/lexbor-build-release/liblexbor_static.a
SANITIZE=build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a
[[ -d "$LEX/.git" && -f "$RELEASE" && -f "$SANITIZE" ]] || fail 'exact Lexbor source/release/sanitize inputs missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'
[[ "$(git -C "$LEX" rev-parse HEAD^{tree})" == '70da8da84cabdc4f02d47378602c41090b2b610c' ]] || fail 'Lexbor tree drift'
[[ -z "$(git -C "$LEX" status --porcelain=v1 --untracked-files=all --ignored=matching)" ]] || fail 'canonical Lexbor source not strong-clean'

tmp=$(mktemp -d /tmp/arborcore-g03-c0-sr1.XXXXXX); trap 'rm -rf "$tmp"' EXIT
for spec in "$RELEASE release" "$SANITIZE sanitize"; do
  set -- $spec; lib=$1; label=$2
  nm -A "$lib" > "$tmp/$label.nm"
  u=$(awk '$2=="U" && $3=="lxb_html_tree_insert_foreign_element"{n++}END{print n+0}' "$tmp/$label.nm")
  d=$(awk '$2~/^[Tt]$/ && $3=="lxb_html_tree_insert_foreign_element"{n++}END{print n+0}' "$tmp/$label.nm")
  [[ "$u" -eq 11 && "$d" -eq 1 ]] || fail "$label insert-foreign wrappability drift: undefined=$u definition=$d"
done

python3 - "$LEX/source/lexbor/html" tests/data/view0_v1n1_g03_c0_sr1_insertion_mode_enum.tsv tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c <<'PY'
import csv,re,sys
from pathlib import Path
html=Path(sys.argv[1]); ledger=Path(sys.argv[2]); adapter=Path(sys.argv[3]).read_text(); prov=Path(sys.argv[4]).read_text()
rows=list(csv.DictReader(ledger.open(),delimiter='\t'))
if len(rows)!=24 or rows[0]['id']!='0': raise SystemExit('FAIL: insertion mode ledger shape')
funcs={r['lexbor_function'] for r in rows[1:]}
pat=re.compile(r'tree->mode\s*=\s*(lxb_html_tree_insertion_mode_[a-z0-9_]+)')
assigned=set()
for p in [html/'tree.c', *sorted((html/'tree').rglob('*.c'))]: assigned.update(pat.findall(p.read_text(errors='replace')))
if funcs != assigned: raise SystemExit(f'FAIL: exact mode assignment set drift missing={sorted(assigned-funcs)} extra={sorted(funcs-assigned)}')
for f in funcs:
    if f not in adapter or f not in prov: raise SystemExit(f'FAIL: mode mapping missing in implementation: {f}')
print('VIEW0_V1N1_G03_C0_SR1_MODE_MAP_EXACT_TREE_ASSIGNMENTS=PASS')
PY

make -s view0-v1n1-g03-c0-sr1-test view0-v1n1-g03-c0-sr1-adversarial-test
cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c \
  tests/c/view0_v1n1_g03_c0_sr1_source_repair_test.c \
  tests/c/view0_v1n1_g03_c0_sr1_source_repair_adversarial_test.c
nm build/view0-v1/native/g03-c0-sr1-source-repair-test > "$tmp/test.nm"
grep -Fq '__wrap_lxb_html_interface_create' "$tmp/test.nm" || fail 'source provenance wrapper absent'
grep -Fq '__wrap_lxb_html_tree_insert_foreign_element' "$tmp/test.nm" || fail 'source repair insertion wrapper absent'
! grep -Eq ' U __real_lxb_html_(interface_create|tree_insert_foreign_element)$' "$tmp/test.nm" || fail 'real Lexbor wrapper symbol unresolved'
echo 'VIEW0_V1N1_G03_C0_SR1_GCC_FANALYZER=PASS'
echo 'VIEW0_V1N1_G03_C0_SR1_SINGLE_LEXBOR_PARSE=PASS'
echo 'VIEW0_V1N1_G03_C0_SR1_TWO_POINT_CONTEXT=PASS'
echo 'VIEW0_V1N1_G03_C0_SR1_OBSERVER_FAILURE_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G03_C0_SR1_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_G03_C0_SR1_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'PASS: C0-SR1 functional/adversarial, exact mode map, linker boundary and analyzer qualification'
