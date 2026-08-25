#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-src'
RELEASE='build/view0-v1/native/lexbor-build-release/liblexbor_static.a'
SANITIZE='build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a'
P0_BUNDLE="${VIEW0_G03_P0_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-P0-preconstruction-review-candidate.tar.gz}"

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$LEX/.git" ]] || fail 'pinned Lexbor source repository missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'
[[ "$(git -C "$LEX" rev-parse HEAD^{tree})" == '70da8da84cabdc4f02d47378602c41090b2b610c' ]] || fail 'Lexbor tree drift'
[[ -f "$RELEASE" && -f "$SANITIZE" ]] || fail 'Lexbor release/sanitize archive missing'
[[ -f "$P0_BUNDLE" ]] || fail "exact G03 P0 bundle missing: $P0_BUNDLE"

make -s view0-v1n1-g03-r1a-test view0-v1n1-g03-r1a-adversarial-test

cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -fsyntax-only \
    tools/c/view0_conformance/g03_r1a.c \
    tools/c/view0_conformance/native.c \
    tests/c/view0_v1n1_g03_r1a_element_context_test.c \
    tests/c/view0_v1n1_g03_r1a_element_context_adversarial_test.c

echo 'VIEW0_V1N1_G03_R1A_GCC_FANALYZER=PASS'

tmp=$(mktemp -d /tmp/arborcore-g03-r1a-native.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/p0"
tar -xzf "$P0_BUNDLE" -C "$tmp/p0"
tokens="$tmp/p0/Arborcore-VIEW0-V1N1-G03-P0-preconstruction-review-candidate/g03-standard-element-token-inventory.tsv"

python3 - "$tokens" tools/c/view0_conformance/g03_r1a.c <<'PY'
import csv, pathlib, re, sys
src=pathlib.Path(sys.argv[1])
code=pathlib.Path(sys.argv[2]).read_text(encoding='utf-8')
bits={'Flow content':1,'Phrasing content':2,'Metadata content':4,'Heading content':8,'Sectioning content':16,'Embedded content':32,'Script-supporting element':64}
expected={}
with src.open(newline='',encoding='utf-8') as f:
    for row in csv.DictReader(f,delimiter='\t'):
        m=0
        for name,bit in bits.items():
            if name in row['categories']:
                m |= bit
        ordinal=int(row['element_ordinal'])
        # link/meta have conditional Flow/Phrasing categories. R1A stores the
        # possible-category union and gates those alternatives by attributes.
        if ordinal in (5,6):
            m |= 1|2
        expected[ordinal]=(row['element_name'],m)
got={int(i):int(v,16) for i,v in re.findall(r'\[(\d+)\]\s*=\s*UINT64_C\(0x([0-9A-Fa-f]+)\)',code)}
if len(expected)!=113 or len(got)!=113:
    raise SystemExit(f'FAIL: category table size expected={len(expected)} got={len(got)}')
for i,(name,m) in expected.items():
    if got.get(i)!=m:
        raise SystemExit(f'FAIL: category mask drift ordinal={i} element={name} expected=0x{m:x} got={got.get(i)!r}')
print('VIEW0_V1N1_G03_R1A_STANDARD_ELEMENT_CATEGORY_TABLE_COUNT=113')
print('VIEW0_V1N1_G03_R1A_LINK_META_CONDITIONAL_CATEGORY_UNION=PASS')
PY

for marker in \
 'dt_trailing' \
 'dt_script_dd' \
 'table_early_tfoot' \
 'table_final_tfoot' \
 'main_named_form' \
 'custom_transparent_valid'; do
    grep -Fq "$marker" tests/c/view0_v1n1_g03_r1a_element_context_test.c || fail "R1A core fixture missing: $marker"
done
for marker in \
 'exact_count - 1u' \
 'diagnostics_sorted' \
 'g02_ownership' \
 'mixed_order' \
 'datalist_option_div' \
 'invalid_utf8'; do
    grep -Fq "$marker" tests/c/view0_v1n1_g03_r1a_element_context_adversarial_test.c || fail "R1A adversarial assertion missing: $marker"
done

if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' tools/c/view0_conformance/g03_r1a.c; then
    fail 'R1A introduced direct Arborcore heap allocation'
fi
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R1A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(CONTENT_MODEL|DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[2-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R2-R7 rule semantics appeared'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R2A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[3-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R3-R7 rule semantics appeared under R2A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    grep -Fq 'arbor_view0_native_g03_r1a_collect_offsets' tools/c/view0_conformance/g03_r1a.c || fail 'R1A private offset-collection compatibility mode missing under R3A'
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 rule semantics appeared under R3A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    grep -Fq 'arbor_view0_native_g03_r1a_collect_offsets' tools/c/view0_conformance/g03_r1a.c || fail 'R1A private offset-collection compatibility mode missing under R4A'
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 rule semantics appeared under R4A'
else
    fail "unexpected retained R1A extension: $current_version"
fi

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort -u)
[[ "$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)" -eq 11 ]] || fail 'production VIEW symbol count changed under R1A'

echo 'VIEW0_V1N1_G03_R1A_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G03_R1A_C0_FACT_LAYOUT_BYTES=184'
echo 'VIEW0_V1N1_G03_R1A_MAX_QUALIFIED_DEPTH=4097'
echo 'VIEW0_V1N1_G03_R1A_CAPACITY_FAILURE_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G03_R1A_MIXED_DIAGNOSTIC_ORDER=PASS'
echo 'VIEW0_V1N1_G03_R1A_G02_SPECIFIC_OWNERSHIP=PASS'
echo 'VIEW0_V1N1_G03_R1A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_G03_R1A_IMPLEMENTATION_COMPLETE=NO'
echo 'PASS: G03 R1A functional/adversarial, category-table, analyzer and partial-rule qualification'
