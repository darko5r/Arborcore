#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src
[[ -d "$LEX/.git" ]] || fail 'exact Lexbor source missing'

echo '### RETAINED R6A / R5A / C0-SR1 / R1-R4 / G02 QUALIFICATION'
bash tools/view0_v1n1_g03_r6a_native_verify.sh

echo '### R7A FUNCTIONAL / ADVERSARIAL'
make -s view0-v1n1-g03-r7a-test view0-v1n1-g03-r7a-adversarial-test \
  view0-v1n1-g03-r7a-global-failure-atomicity-test
[[ -x build/view0-v1/native/g03-r7a-palpable-content-test ]] || fail 'R7A functional binary missing'
[[ -x build/view0-v1/native/g03-r7a-palpable-content-adversarial-test ]] || fail 'R7A adversarial binary missing'
[[ -x build/view0-v1/native/g03-r7a-global-failure-atomicity-test ]] || fail 'R7A SR2 failure-atomicity binary missing'

tmp=$(mktemp -d /tmp/arborcore-g03-r7a.XXXXXX); trap 'rm -rf "$tmp"' EXIT

echo '### R7A GCC ANALYZER'
cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c \
  tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c tools/c/view0_conformance/g03_r4a.c \
  tools/c/view0_conformance/g03_r5a.c tools/c/view0_conformance/g03_r7a.c \
  tests/c/view0_v1n1_g03_r7a_palpable_content_test.c \
  tests/c/view0_v1n1_g03_r7a_palpable_content_adversarial_test.c \
  tests/c/view0_v1n1_g03_r7a_global_failure_atomicity_test.c
echo 'VIEW0_V1N1_G03_R7A_GCC_FANALYZER=PASS'

echo '### R7A SR2 COMPILED STACK BOUNDS / PHASED COMPOSITION'
common=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fstack-usage)
for unit in native lexbor_adapter g03_r1a g03_r2a g03_r3a g03_r4a g03_r5a g03_r7a; do
  cc "${common[@]}" -c "tools/c/view0_conformance/$unit.c" -o "$tmp/$unit.o"
done
stack_field(){
  local file=$1 pattern=$2
  awk -F '\t' -v p="$pattern" '$1 ~ p {print $2; exit}' "$file"
}
native_check=$(stack_field "$tmp/native.su" 'arbor_view0_native_check$')
r7=$(stack_field "$tmp/g03_r7a.su" ':evaluate$')
r7_anchor=$(stack_field "$tmp/g03_r7a.su" 'arbor_view0_native_g03_r7a_collect_anchors$')
r5_eval=$(stack_field "$tmp/g03_r5a.su" ':evaluate$')
r5_anchor=$(stack_field "$tmp/g03_r5a.su" 'arbor_view0_native_g03_r5a_collect_anchors$')
r5_prior=$(stack_field "$tmp/g03_r5a.su" 'collect_prior_error_offsets')
r1_eval=$(stack_field "$tmp/g03_r1a.su" ':evaluate$')
r1_measure=$(stack_field "$tmp/g03_r1a.su" 'arbor_view0_native_g03_r1a_measure$')
lex_observe=$(stack_field "$tmp/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe$')
lex_exact=$(stack_field "$tmp/lexbor_adapter.su" 'arbor_view0_native_lexbor_collect_exact$')
lex_process=$(stack_field "$tmp/lexbor_adapter.su" ':lexbor_process$')
bound=$(sed -n 's/^VIEW0_V1N1_G03_R7A_EVALUATE_COMPILED_STACK_BOUND_BYTES=//p' view/arborcore-view-core-1.contract | head -1)
phase_bound=$(sed -n 's/^VIEW0_V1N1_G03_R7A_PHASED_COMPILED_STACK_BOUND_BYTES=//p' view/arborcore-view-core-1.contract | head -1)
for value in "$native_check" "$r7" "$r7_anchor" "$r5_eval" "$r5_anchor" "$r5_prior" "$r1_eval" "$r1_measure" "$lex_observe" "$lex_exact" "$lex_process" "$bound" "$phase_bound"; do
  [[ -n "$value" ]] || fail 'SR2 stack record/bound missing'
done
[[ "$bound" == '400000' ]] || fail "R7A stack bound drift: $bound"
[[ "$phase_bound" == '900000' ]] || fail "R7A phased stack bound drift: $phase_bound"
(( r7 <= bound )) || fail "R7A compiled stack bound exceeded observed=$r7 bound=$bound"
# Conservative deepest admitted phase: native_check -> R5 anchor/evaluate ->
# prior-owner collection -> R1 measure/evaluate -> Lexbor observe/process.
phase_observed=$((native_check + r5_anchor + r5_eval + r5_prior + r1_measure + r1_eval + lex_observe + lex_process))
(( phase_observed <= phase_bound )) || fail "R7A SR2 phased stack bound exceeded observed=$phase_observed bound=$phase_bound"
final_parse_phase=$((native_check + lex_exact + lex_process))
(( final_parse_phase <= phase_bound )) || fail 'R7A SR2 final parser publication phase stack bound exceeded'
echo "VIEW0_V1N1_G03_R7A_EVALUATE_COMPILED_STACK_BYTES=$r7"
echo "VIEW0_V1N1_G03_R7A_EVALUATE_COMPILED_STACK_BOUND_BYTES=$bound"
echo "VIEW0_V1N1_G03_R7A_SR2_NATIVE_CHECK_COMPILED_STACK_BYTES=$native_check"
echo "VIEW0_V1N1_G03_R7A_SR2_PHASED_COMPILED_STACK_OBSERVED_BYTES=$phase_observed"
echo "VIEW0_V1N1_G03_R7A_PHASED_COMPILED_STACK_BOUND_BYTES=$phase_bound"
echo 'VIEW0_V1N1_G03_R7A_SR2_STACK_THRESHOLD_WIDENING=NO'

echo '### R7A CLI WARNING / DIRECT-CHILD / DEFERRAL CONTROLS'
make -s view0-v1n0-tool
run_cli(){
  local name=$1 html=$2 expected_rc=$3
  printf '%s' "$html" > "$tmp/$name.html"
  set +e
  build/view0-v1/native/arborcore-view0-html-check --format=tsv "$tmp/$name.html" > "$tmp/$name.tsv"
  local rc=$?
  set -e
  [[ "$rc" -eq "$expected_rc" ]] || fail "$name CLI exit=$rc expected=$expected_rc"
}
run_cli empty_p '<!doctype html><title>x</title><body><p></p></body>' 0
grep -Fq $'warning\t0x0000000030030007\tARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY\tarborcore-authoring' "$tmp/empty_p.tsv" || fail 'R7A warning diagnostic missing'
awk -F '\t' '$2=="warning" && $3=="0x0000000030030007" { if ($6=="38" && $7=="1" && $8=="1" && $9=="39") ok=1 } END { exit ok?0:1 }' "$tmp/empty_p.tsv" || fail 'R7A exact warning source anchor drift'
grep -Fq $'g03_r7=partial' "$tmp/empty_p.tsv" || fail 'R7A CLI partial marker missing'
run_cli whitespace '<!doctype html><title>x</title><body><div>   </div></body>' 0
grep -Fq $'0x0000000030030007\tARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY' "$tmp/whitespace.tsv" || fail 'R7A whitespace-only warning missing'
run_cli visible '<!doctype html><title>x</title><body><div><span>x</span></div></body>' 0
! grep -Fq $'0x0000000030030007\tARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY' "$tmp/visible.tsv" || fail 'R7A false warning with visible palpable direct child'
run_cli g04 '<!doctype html><title>x</title><body><select><option><div></div></option></select></body>' 0
grep -Fq $'r7_g04_transparent_deferred=yes' "$tmp/g04.tsv" || fail 'R7A G04 CLI deferral marker missing'
run_cli g13 '<!doctype html><title>x</title><body><div><x-r7></x-r7></div></body>' 0
grep -Fq $'r7_g13_custom_deferred=yes' "$tmp/g13.tsv" || fail 'R7A G13 CLI deferral marker missing'
run_cli r7f017 '<!doctype html><title>x</title><body><dl><div><dt>x</dt><dd>y</dd></div></dl></body>' 0
! grep -Fq $'0x0000000030030007\tARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY' "$tmp/r7f017.tsv" || fail 'corrected R7F017 still produces R7 warning'
echo 'VIEW0_V1N1_G03_R7A_CLI_WARNING_ANCHOR=PASS_EXACT_OFFSET_38_LENGTH_1_LINE_1_COLUMN_39'
echo 'VIEW0_V1N1_G03_R7A_CLI_DIRECT_CHILD_CONTROL=PASS'
echo 'VIEW0_V1N1_G03_R7A_CLI_G04_DEFERRAL=PASS'
echo 'VIEW0_V1N1_G03_R7A_CLI_G13_DEFERRAL=PASS'
echo 'VIEW0_V1N1_G03_R7A_CLI_R7F017_ISOLATION=PASS'
echo 'PASS: G03 R7A functional/adversarial/analyzer/stack/CLI qualification'
