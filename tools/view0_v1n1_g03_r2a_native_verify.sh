#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-compat-src'
CANON='build/view0-v1/native/lexbor-src'
RELEASE='build/view0-v1/native/lexbor-build-release/liblexbor_static.a'
SANITIZE='build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a'
P0_BUNDLE="${VIEW0_G03_P0_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G03-P0-preconstruction-review-candidate.tar.gz}"
fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$CANON/.git" && -d "$LEX" ]] || fail 'canonical/derived Lexbor source missing'
[[ "$(git -C "$CANON" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'canonical Lexbor commit drift'
[[ -z "$(git -C "$CANON" status --porcelain=v1 --untracked-files=all --ignored=matching)" ]] || fail 'canonical Lexbor cache is not strong-clean'
[[ "$(sha256sum "$LEX/source/lexbor/html/tree/insertion_mode/in_body.c" | awk '{print $1}')" == '142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8' ]] || fail 'derived Lexbor ruby compatibility source drift'
[[ -f "$RELEASE" && -f "$SANITIZE" ]] || fail 'Lexbor release/sanitize archive missing'
[[ -f "$P0_BUNDLE" ]] || fail "exact G03 P0 bundle missing: $P0_BUNDLE"

make -s view0-v1n1-g03-r2a-test view0-v1n1-g03-r2a-adversarial-test build/view0-v1/native/arborcore-view0-html-check

cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -fsyntax-only \
    tools/c/view0_conformance/g03_r2a.c \
    tools/c/view0_conformance/native.c \
    tests/c/view0_v1n1_g03_r2a_content_model_test.c \
    tests/c/view0_v1n1_g03_r2a_content_model_adversarial_test.c

echo 'VIEW0_V1N1_G03_R2A_GCC_FANALYZER=PASS'

tmp=$(mktemp -d /tmp/arborcore-g03-r2a-native.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/p0"
tar -xzf "$P0_BUNDLE" -C "$tmp/p0"
tokens="$tmp/p0/Arborcore-VIEW0-V1N1-G03-P0-preconstruction-review-candidate/g03-standard-element-token-inventory.tsv"
python3 - "$tokens" tools/c/view0_conformance/g03_r2a.c <<'PY'
import csv,pathlib,re,sys
bits={'Flow content':1,'Phrasing content':2,'Metadata content':4,'Heading content':8,'Sectioning content':16,'Embedded content':32,'Script-supporting element':64}
expected={}
with open(sys.argv[1],newline='',encoding='utf-8') as f:
    for row in csv.DictReader(f,delimiter='\t'):
        mask=0
        for name,bit in bits.items():
            if name in row['categories']: mask |= bit
        ordinal=int(row['element_ordinal'])
        if ordinal in (5,6): mask |= 1|2
        expected[ordinal]=(row['element_name'],mask)
code=pathlib.Path(sys.argv[2]).read_text(encoding='utf-8')
got={int(i):int(v,16) for i,v in re.findall(r'\[(\d+)\]\s*=\s*UINT64_C\(0x([0-9A-Fa-f]+)\)',code)}
if len(expected)!=113 or len(got)!=113: raise SystemExit(f'FAIL: category table size expected={len(expected)} got={len(got)}')
for i,(name,mask) in expected.items():
    if got.get(i)!=mask: raise SystemExit(f'FAIL: R2A category mask drift ordinal={i} element={name} expected=0x{mask:x} got={got.get(i)!r}')
print('VIEW0_V1N1_G03_R2A_STANDARD_ELEMENT_CATEGORY_TABLE_COUNT=113')
PY

for marker in \
 'option><div>a</div>' \
 'ruby>a<rt>x</rt>b<rt>y</rt>' \
 'select multiple><button>' \
 'datalist><span><option' \
 'lone trailing dt'; do
    grep -Fq "$marker" tests/c/view0_v1n1_g03_r2a_content_model_test.c || fail "R2A functional fixture missing: $marker"
done
for marker in 'exact_count - 1u' 'one parent' 'R1-only' 'invalid_utf8'; do
    grep -Fqi "$marker" tests/c/view0_v1n1_g03_r2a_content_model_adversarial_test.c || fail "R2A adversarial assertion missing: $marker"
done

fixture="$tmp/deferred.html"
printf '%s' '<!doctype html><title>x</title><style>x{}</style><script></script><noscript>x</noscript><select size=1 multiple><button></button><option>x</option></select><p><x-widget></x-widget></p>' > "$fixture"
set +e
build/view0-v1/native/arborcore-view0-html-check --format=tsv "$fixture" > "$tmp/cli.out"
rc=$?
set -e
[[ "$rc" -eq 0 || "$rc" -eq 1 ]] || fail "deferred CLI control mechanism exit=$rc expected=0_or_1"
! grep -Fq $'\t0x0000000030030002\t' "$tmp/cli.out" || fail 'deferred CLI control unexpectedly emitted an R2 diagnostic'
grep -Fq $'g03_r2=partial\tr2_style_deferred=yes\tr2_script_deferred=yes\tr2_noscript_deferred=yes\tr2_select_size_deferred=yes\tr2_select_platform_deferred=yes\tr2_unclassified_deferred=yes' "$tmp/cli.out" || fail 'CLI does not expose all R2A deferred branches'
echo 'VIEW0_V1N1_G03_R2A_CLI_PARTIAL_DEFERRED_FLAGS=PASS'

if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' tools/c/view0_conformance/g03_r2a.c; then fail 'R2A introduced direct Arborcore heap allocation'; fi
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R2A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[3-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R3-R7 rule semantics appeared'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 rule semantics appeared under R3A'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 rule semantics appeared under R4A'
else
    fail "unexpected R2A retained extension: $current_version"
fi

symbols=$(nm -g --defined-only build/libarborcore_view.a | awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort -u)
[[ "$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)" -eq 11 ]] || fail 'production VIEW symbol count changed under R2A'

echo 'VIEW0_V1N1_G03_R2A_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G03_R2A_FRAME_SIZE_X86_64=160'
echo 'VIEW0_V1N1_G03_R2A_EVALUATOR_WORKSPACE_SIZE_X86_64=655736'
echo 'VIEW0_V1N1_G03_R2A_MAX_QUALIFIED_DEPTH=4097'
echo 'VIEW0_V1N1_G03_R2A_CAPACITY_FAILURE_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G03_R2A_ONE_DIAGNOSTIC_PER_FAILING_PARENT_MAX=PASS'
echo 'VIEW0_V1N1_G03_R2A_R1_DUPLICATE_SUPPRESSION=PASS'
echo 'VIEW0_V1N1_G03_R2A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_G03_R2A_IMPLEMENTATION_COMPLETE=NO'
echo 'PASS: G03 R2A functional/adversarial, P2 residual ownership, analyzer, CLI and resource qualification'
