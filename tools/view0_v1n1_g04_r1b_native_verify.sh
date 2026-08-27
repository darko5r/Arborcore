#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src
[[ -d "$LEX/.git" ]] || fail 'exact pinned Lexbor source missing'

echo '### G04 R1B — RETAINED G02/G03 + R1A CORE'
bash tools/view0_v1n1_g04_r1a_native_verify.sh

echo '### G04 R1B — FUNCTIONAL / ADVERSARIAL'
make -s view0-v1n1-g04-r1b-test view0-v1n1-g04-r1b-adversarial-test
[[ -x build/view0-v1/native/g04-r1b-transparent-parent-model-test ]] || fail 'G04 R1B functional binary missing'
[[ -x build/view0-v1/native/g04-r1b-transparent-parent-model-adversarial-test ]] || fail 'G04 R1B adversarial binary missing'

tmp=$(mktemp -d /tmp/arborcore-g04-r1b-native.XXXXXX); trap 'rm -rf "$tmp"' EXIT
CC_BIN="${CC:-cc}"
CC_PATH=$(command -v "$CC_BIN") || fail "compiler missing: $CC_BIN"
CC_VERSION=$($CC_BIN --version | head -1)
CC_TARGET=$($CC_BIN -dumpmachine)
COMMON_FLAGS='-std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef'
printf 'VIEW0_V1N1_G04_R1B_COMPILER_PATH=%s\n' "$CC_PATH"
printf 'VIEW0_V1N1_G04_R1B_COMPILER_VERSION=%s\n' "$CC_VERSION"
printf 'VIEW0_V1N1_G04_R1B_COMPILER_TARGET=%s\n' "$CC_TARGET"
printf 'VIEW0_V1N1_G04_R1B_COMPILE_FLAGS=%s\n' "$COMMON_FLAGS"

echo '### G04 R1B — GCC ANALYZER'
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c \
  tools/c/view0_conformance/g03_provenance.c \
  tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c tools/c/view0_conformance/g03_r4a.c \
  tools/c/view0_conformance/g03_r5a.c tools/c/view0_conformance/g03_r7a.c \
  tools/c/view0_conformance/g04_r1a.c \
  tests/c/view0_v1n1_g04_r1b_transparent_parent_model_test.c \
  tests/c/view0_v1n1_g04_r1b_transparent_parent_model_adversarial_test.c \
  tests/c/view0_v1n1_g04_r1a_global_failure_atomicity_test.c
echo 'VIEW0_V1N1_G04_R1B_GCC_FANALYZER=PASS'

echo '### G04 R1B — COMPILED STACK / EXISTING 900000-BYTE PHASED ADMISSION'
common=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fstack-usage)
for unit in native lexbor_adapter g03_r1a g03_r5a g04_r1a; do
  "$CC_BIN" "${common[@]}" -c "tools/c/view0_conformance/$unit.c" -o "$tmp/$unit.o"
done
stack_field(){ awk -F '\t' -v p="$2" '$1 ~ p {print $2; exit}' "$1"; }
native_check=$(stack_field "$tmp/native.su" 'arbor_view0_native_check$')
g04_public=$(stack_field "$tmp/g04_r1a.su" 'arbor_view0_native_g04_r1a_measure$')
g04_eval=$(stack_field "$tmp/g04_r1a.su" ':evaluate$')
g04_prior=$(stack_field "$tmp/g04_r1a.su" 'collect_prior_error_anchors')
g04_with_prior=$(stack_field "$tmp/g04_r1a.su" 'evaluate_with_prior_anchors')
r5_public=$(stack_field "$tmp/g03_r5a.su" 'arbor_view0_native_g03_r5a_measure$')
r5_eval=$(stack_field "$tmp/g03_r5a.su" ':evaluate$')
r5_prior=$(stack_field "$tmp/g03_r5a.su" 'collect_prior_error_offsets')
r1_public=$(stack_field "$tmp/g03_r1a.su" 'arbor_view0_native_g03_r1a_measure$')
r1_eval=$(stack_field "$tmp/g03_r1a.su" ':evaluate$')
lex_observe=$(stack_field "$tmp/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe$')
lex_process=$(stack_field "$tmp/lexbor_adapter.su" ':lexbor_process$')
lex_exact=$(stack_field "$tmp/lexbor_adapter.su" 'arbor_view0_native_lexbor_collect_exact$')
bound=$(sed -n 's/^VIEW0_V1N1_G04_R1B_PHASED_COMPILED_STACK_BOUND_BYTES=//p' view/arborcore-view-core-1.contract | head -1)
for v in "$native_check" "$g04_public" "$g04_eval" "$g04_prior" "$g04_with_prior" "$r5_public" "$r5_eval" "$r5_prior" "$r1_public" "$r1_eval" "$lex_observe" "$lex_process" "$lex_exact" "$bound"; do [[ -n "$v" ]] || fail 'missing stack record/bound'; done
[[ "$bound" == '900000' ]] || fail "phased bound widened/drifted: $bound"
prior_phase=$((native_check + g04_public + g04_eval + g04_prior + r5_public + r5_eval + r5_prior + r1_public + r1_eval + lex_observe + lex_process))
(( prior_phase <= bound )) || fail "G04 R1B prior-owner phased stack exceeded observed=$prior_phase bound=$bound"
transparent_phase=$((native_check + g04_public + g04_eval + g04_with_prior + lex_observe + lex_process + 80))
(( transparent_phase <= bound )) || fail "G04 R1B transparent phase stack exceeded observed=$transparent_phase bound=$bound"
final_parse_phase=$((native_check + lex_exact + lex_process))
(( final_parse_phase <= bound )) || fail 'final exact Lexbor publication stack exceeded bound'
echo "VIEW0_V1N1_G04_R1B_EVALUATE_WITH_PRIOR_COMPILED_STACK_BYTES=$g04_with_prior"
echo "VIEW0_V1N1_G04_R1B_PRIOR_OWNER_PHASED_COMPILED_STACK_OBSERVED_BYTES=$prior_phase"
echo "VIEW0_V1N1_G04_R1B_TRANSPARENT_PHASED_COMPILED_STACK_OBSERVED_BYTES=$transparent_phase"
echo "VIEW0_V1N1_G04_R1B_FINAL_PARSE_PHASED_COMPILED_STACK_OBSERVED_BYTES=$final_parse_phase"
echo "VIEW0_V1N1_G04_R1B_PHASED_COMPILED_STACK_BOUND_BYTES=$bound"
echo 'VIEW0_V1N1_G04_R1B_STACK_THRESHOLD_WIDENING=NO'

echo '### G04 R1B — CLI OPTION / SELECT TEXT / RESIDUAL CONTROLS'
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
run_cli matrix_positive '<!doctype html><title>x</title><body><p><a href="/"><em>x</em></a></p></body>' 0
! grep -Fq $'0x0000000030040001\tARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL' "$tmp/matrix_positive.tsv" || fail 'false G04 diagnostic on frozen positive'
run_cli matrix_negative '<!doctype html><title>x</title><body><p><a href="/"><div>x</div></a></p></body>' 1
awk -F '\t' '$2=="error" && $3=="0x0000000030040001" { if ($6=="53" && $7=="3" && $8=="1" && $9=="54") ok=1 } END { exit ok?0:1 }' "$tmp/matrix_negative.tsv" || fail 'G04 frozen negative source anchor drift'
run_cli option '<!doctype html><title>x</title><body><select><option><div><span>x</span></div></option></select></body>' 0
grep -Fq $'g04_r1_option_branch_deferred=no' "$tmp/option.tsv" || fail 'option deferral not retired'
run_cli select_text '<!doctype html><title>x</title><body><select><div>bad</div></select></body>' 1
grep -Fq $'error\t0x0000000030040001\tARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL\tarborcore-authoring' "$tmp/select_text.tsv" || fail 'select text G04 diagnostic missing'
run_cli select_ws '<!doctype html><title>x</title><body><select><div> &#32; </div></select></body>' 0
! grep -Fq $'0x0000000030040001\tARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL' "$tmp/select_ws.tsv" || fail 'whitespace-only select text falsely rejected'
run_cli select_amp '<!doctype html><title>x</title><body><select><div>&amp;</div></select></body>' 1
grep -Fq $'0x0000000030040001\tARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL' "$tmp/select_amp.tsv" || fail 'decoded non-whitespace reference not rejected'
run_cli noscript '<!doctype html><title>x</title><body><noscript><div>x</div></noscript></body>' 0
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then
  grep -Fq $'g04_r1_noscript_scripting_deferred=no' "$tmp/noscript.tsv" || fail 'R1C noscript closure not retained'
else
  grep -Fq $'g04_r1_noscript_scripting_deferred=yes' "$tmp/noscript.tsv" || fail 'noscript residual marker missing'
fi
run_cli custom '<!doctype html><title>x</title><body><x-r1><div>x</div></x-r1></body>' 0
grep -Fq $'g04_r1_g13_custom_deferred=yes' "$tmp/custom.tsv" || fail 'G13 custom residual marker missing'
echo 'VIEW0_V1N1_G04_R1B_CLI_FROZEN_MATRIX=2_OF_2'
echo 'VIEW0_V1N1_G04_R1B_CLI_OPTION_BRANCH=RESOLVED_NO_DEFERRAL'
echo 'VIEW0_V1N1_G04_R1B_CLI_SELECT_SOURCE_TEXT=PASS'
echo 'VIEW0_V1N1_G04_R1B_CLI_CHARACTER_REFERENCES=PASS'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then echo 'VIEW0_V1N1_G04_R1B_CLI_RETAINED_UNDER_R1C=PASS_G13_ONLY'; else echo 'VIEW0_V1N1_G04_R1B_CLI_REMAINING_RESIDUALS=NOSCRIPT_G13'; fi
echo 'PASS: G04 R1B functional/adversarial/analyzer/stack/CLI qualification'
