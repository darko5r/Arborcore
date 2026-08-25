#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src
[[ -d "$LEX/.git" ]] || fail 'exact Lexbor source missing'
echo '### RETAINED C0-SR1 SINGLE-PARSER / LINKER / FAILURE-ATOMICITY QUALIFICATION'
bash tools/view0_v1n1_g03_c0_sr1_native_verify.sh
echo '### RETAINED G03 R1A-R4A FUNCTIONAL / ADVERSARIAL REGRESSION'
make -s \
  view0-v1n1-g03-r1a-test view0-v1n1-g03-r1a-adversarial-test \
  view0-v1n1-g03-r2a-test view0-v1n1-g03-r2a-adversarial-test \
  view0-v1n1-g03-r3a-test view0-v1n1-g03-r3a-adversarial-test \
  view0-v1n1-g03-r4a-test view0-v1n1-g03-r4a-adversarial-test
echo '### RETAINED FROZEN G02 6-OF-6 FUNCTIONAL / ADVERSARIAL REGRESSION'
make -s \
  view0-v1n1-g02-r1-test view0-v1n1-g02-r1-adversarial-test \
  view0-v1n1-g02-r2-test view0-v1n1-g02-r2-adversarial-test \
  view0-v1n1-g02-r3-test view0-v1n1-g02-r3-adversarial-test \
  view0-v1n1-g02-r4-test view0-v1n1-g02-r4-adversarial-test \
  view0-v1n1-g02-r5-test view0-v1n1-g02-r5-adversarial-test \
  view0-v1n1-g02-r6-test view0-v1n1-g02-r6-adversarial-test
echo '### R5A FUNCTIONAL / ADVERSARIAL'
make -s view0-v1n1-g03-r5a-test view0-v1n1-g03-r5a-adversarial-test

tmp=$(mktemp -d /tmp/arborcore-g03-r5a.XXXXXX); trap 'rm -rf "$tmp"' EXIT
echo '### R5A GCC ANALYZER'
cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/g03_r2a.c tools/c/view0_conformance/g03_r3a.c \
  tools/c/view0_conformance/g03_r4a.c tools/c/view0_conformance/g03_r5a.c \
  tools/c/view0_conformance/native.c \
  tests/c/view0_v1n1_g03_r5a_explicit_html_element_allowance_test.c \
  tests/c/view0_v1n1_g03_r5a_explicit_html_element_allowance_adversarial_test.c
echo 'VIEW0_V1N1_G03_R5A_GCC_FANALYZER=PASS'

echo '### R5A COMPILED STACK BOUNDS'
common=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
  -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fstack-usage)
for r in r1a r2a r3a r4a r5a; do
  cc "${common[@]}" -c "tools/c/view0_conformance/g03_${r}.c" -o "$tmp/g03_${r}.o"
done
get_stack(){ awk -F '\t' -v re="$2" '$1 ~ re {print $2; exit}' "$1"; }
r1=$(get_stack "$tmp/g03_r1a.su" 'evaluate$')
r2=$(get_stack "$tmp/g03_r2a.su" 'evaluate$')
r3=$(get_stack "$tmp/g03_r3a.su" 'evaluate_with_r1_offsets$')
r4=$(get_stack "$tmp/g03_r4a.su" 'evaluate_with_r1_offsets$')
r5=$(get_stack "$tmp/g03_r5a.su" 'evaluate$')
r5_bound=$(sed -n 's/^VIEW0_V1N1_G03_R5A_EVALUATE_COMPILED_STACK_BOUND_BYTES=//p' view/arborcore-view-core-1.contract | head -1)
[[ -n "$r1" && -n "$r2" && -n "$r3" && -n "$r4" && -n "$r5" && -n "$r5_bound" ]] || fail 'compiled stack records/bound missing'
[[ "$r5_bound" == '40000' ]] || fail "R5A stack contract bound drift: $r5_bound"
(( r1 <= 800000 && r2 <= 700000 && r3 <= 300000 && r4 <= 200000 && r5 <= r5_bound )) || \
  fail "compiled stack bound exceeded r1=$r1 r2=$r2 r3=$r3 r4=$r4 r5=$r5 r5_bound=$r5_bound"
echo "VIEW0_V1N1_G03_R5A_R1_COMPILED_STACK_BYTES=$r1"
echo "VIEW0_V1N1_G03_R5A_R2_COMPILED_STACK_BYTES=$r2"
echo "VIEW0_V1N1_G03_R5A_R3_INNER_COMPILED_STACK_BYTES=$r3"
echo "VIEW0_V1N1_G03_R5A_R4_INNER_COMPILED_STACK_BYTES=$r4"
echo "VIEW0_V1N1_G03_R5A_EVALUATE_COMPILED_STACK_BYTES=$r5"
echo "VIEW0_V1N1_G03_R5A_EVALUATE_COMPILED_STACK_BOUND_BYTES=$r5_bound"
echo 'VIEW0_V1N1_G03_R5A_PHASED_COMPILED_STACK_BOUND_BYTES=900000'

echo '### R5A CLI SOURCE-ANCHOR / SUPPRESSION CHECK'
make -s view0-v1n0-tool
printf '%s' '<!doctype html><title>x</title><body><table><p>x</p></table></body>' > "$tmp/foster.html"
set +e
build/view0-v1/native/arborcore-view0-html-check --format=tsv "$tmp/foster.html" > "$tmp/foster.tsv"
rc=$?
set -e
[[ "$rc" -eq 1 ]] || fail "foster CLI exit=$rc expected=1"
grep -Fq $'error\t0x0000000030030005\tARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE\tarborcore-authoring\t45\t1' "$tmp/foster.tsv" || fail 'R5A foster CLI diagnostic/anchor missing'
grep -Fq $'g03_r5=partial' "$tmp/foster.tsv" || fail 'R5A CLI partial marker missing'
printf '%s' '<!doctype html><title>x</title><body><table><li>x</li></table></body>' > "$tmp/suppressed.html"
set +e
build/view0-v1/native/arborcore-view0-html-check --format=tsv "$tmp/suppressed.html" > "$tmp/suppressed.tsv"
rc=$?
set -e
[[ "$rc" -eq 1 ]] || fail "suppression CLI exit=$rc expected=1"
! grep -Fq $'0x0000000030030005\tARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE' "$tmp/suppressed.tsv" || fail 'R5A duplicate survived prior-R1 suppression'
grep -Fq $'0x0000000030030001\tARBOR_VIEW_V1_G03_ELEMENT_CONTEXT' "$tmp/suppressed.tsv" || fail 'expected prior R1 owner missing'
printf '%s' '<!doctype html><title>x</title><body><table><svg><foreignObject><p>x</p></foreignObject></svg></table></body>' > "$tmp/foreignobject.html"
set +e
build/view0-v1/native/arborcore-view0-html-check --format=tsv "$tmp/foreignobject.html" > "$tmp/foreignobject.tsv"
rc=$?
set -e
[[ "$rc" -eq 0 ]] || fail "foreignObject applicability-control CLI exit=$rc expected=0"
! grep -Fq $'0x0000000030030005\tARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE' "$tmp/foreignobject.tsv" || fail 'R5A false positive survived non-table foster-state applicability control'
grep -Fq $'g03_r5=partial' "$tmp/foreignobject.tsv" || fail 'R5A CLI partial marker missing on applicability control'
echo 'VIEW0_V1N1_G03_R5A_CLI_FOSTER_SOURCE_ANCHOR=PASS'
echo 'VIEW0_V1N1_G03_R5A_CLI_PRIOR_OWNER_SUPPRESSION=PASS'
echo 'VIEW0_V1N1_G03_R5A_CLI_FOREIGNOBJECT_APPLICABILITY_CONTROL=PASS'
echo 'PASS: G03 R5A functional/adversarial/regression/analyzer/stack/CLI qualification'
