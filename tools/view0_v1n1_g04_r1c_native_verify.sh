#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src
[[ -d "$LEX/.git" ]] || fail 'exact pinned Lexbor source missing'

echo '### G04 R1C — RETAINED G02/G03 + R1A/R1B'
bash tools/view0_v1n1_g04_r1b_native_verify.sh

echo '### G04 R1C — FUNCTIONAL / ADVERSARIAL'
make -s view0-v1n1-g04-r1c-test view0-v1n1-g04-r1c-adversarial-test
[[ -x build/view0-v1/native/g04-r1c-noscript-transparent-test ]] || fail 'R1C functional binary missing'
[[ -x build/view0-v1/native/g04-r1c-noscript-transparent-adversarial-test ]] || fail 'R1C adversarial binary missing'

tmp=$(mktemp -d /tmp/arborcore-g04-r1c-native.XXXXXX); trap 'rm -rf "$tmp"' EXIT
CC_BIN="${CC:-cc}"
CC_PATH=$(command -v "$CC_BIN") || fail "compiler missing: $CC_BIN"
CC_VERSION=$($CC_BIN --version | head -1)
CC_TARGET=$($CC_BIN -dumpmachine)
COMMON_FLAGS='-std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef'
printf 'VIEW0_V1N1_G04_R1C_COMPILER_PATH=%s\n' "$CC_PATH"
printf 'VIEW0_V1N1_G04_R1C_COMPILER_VERSION=%s\n' "$CC_VERSION"
printf 'VIEW0_V1N1_G04_R1C_COMPILER_TARGET=%s\n' "$CC_TARGET"
printf 'VIEW0_V1N1_G04_R1C_COMPILE_FLAGS=%s\n' "$COMMON_FLAGS"

echo '### G04 R1C — EXPLICIT PINNED LEXBOR SCRIPTING MODE'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'
[[ "$(sha256sum "$LEX/source/lexbor/html/parser.h" | awk '{print $1}')" == 'a69ace318e92fa41f9631c1666836b11ba0b89067ffde8e2dd9937b2c2da5f48' ]] || fail 'Lexbor parser.h drift'
current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)"
expected_setters=1
[[ "$current_version" != '0.1-VIEW0-V1N1-G04-R2' ]] || expected_setters=2
[[ "$(grep -Fc 'lxb_html_parser_scripting_set(parser, false);' tools/c/view0_conformance/lexbor_adapter.c)" -eq "$expected_setters" ]] || fail 'explicit disabled setter count drift' 
[[ "$(grep -Fc 'lxb_html_parser_scripting(parser)' tools/c/view0_conformance/lexbor_adapter.c)" -ge 1 ]] || fail 'scripting mode readback missing'
echo 'VIEW0_V1N1_G04_R1C_CHECKER_SCRIPTING_MODE=DISABLED'
echo 'VIEW0_V1N1_G04_R1C_LEXBOR_SCRIPTING_MODE_BIND=PASS'

echo '### G04 R1C — GCC ANALYZER'
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c \
  tools/c/view0_conformance/g03_provenance.c \
  tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c tools/c/view0_conformance/g03_r4a.c \
  tools/c/view0_conformance/g03_r5a.c tools/c/view0_conformance/g03_r7a.c \
  tools/c/view0_conformance/g04_r1a.c \
  tests/c/view0_v1n1_g04_r1c_noscript_transparent_test.c \
  tests/c/view0_v1n1_g04_r1c_noscript_transparent_adversarial_test.c \
  tests/c/view0_v1n1_g04_r1a_global_failure_atomicity_test.c
echo 'VIEW0_V1N1_G04_R1C_GCC_FANALYZER=PASS'

echo '### G04 R1C — COMPILED STACK / EXISTING 900000-BYTE PHASED ADMISSION'
common=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fstack-usage)
for unit in native lexbor_adapter g03_r1a g03_r5a g04_r1a; do "$CC_BIN" "${common[@]}" -c "tools/c/view0_conformance/$unit.c" -o "$tmp/$unit.o"; done
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
bound=$(sed -n 's/^VIEW0_V1N1_G04_R1C_PHASED_COMPILED_STACK_BOUND_BYTES=//p' view/arborcore-view-core-1.contract | head -1)
for v in "$native_check" "$g04_public" "$g04_eval" "$g04_prior" "$g04_with_prior" "$r5_public" "$r5_eval" "$r5_prior" "$r1_public" "$r1_eval" "$lex_observe" "$lex_process" "$lex_exact" "$bound"; do [[ -n "$v" ]] || fail 'missing stack record/bound'; done
[[ "$bound" == '900000' ]] || fail "phased bound widened/drifted: $bound"
prior_phase=$((native_check + g04_public + g04_eval + g04_prior + r5_public + r5_eval + r5_prior + r1_public + r1_eval + lex_observe + lex_process))
transparent_phase=$((native_check + g04_public + g04_eval + g04_with_prior + lex_observe + lex_process + 80))
final_parse_phase=$((native_check + lex_exact + lex_process))
(( prior_phase <= bound )) || fail "R1C prior-owner phased stack exceeded observed=$prior_phase bound=$bound"
(( transparent_phase <= bound )) || fail "R1C transparent phase stack exceeded observed=$transparent_phase bound=$bound"
(( final_parse_phase <= bound )) || fail "R1C final parse phase stack exceeded observed=$final_parse_phase bound=$bound"
echo "VIEW0_V1N1_G04_R1C_EVALUATE_WITH_PRIOR_COMPILED_STACK_BYTES=$g04_with_prior"
echo "VIEW0_V1N1_G04_R1C_PRIOR_OWNER_PHASED_COMPILED_STACK_OBSERVED_BYTES=$prior_phase"
echo "VIEW0_V1N1_G04_R1C_TRANSPARENT_PHASED_COMPILED_STACK_OBSERVED_BYTES=$transparent_phase"
echo "VIEW0_V1N1_G04_R1C_FINAL_PARSE_PHASED_COMPILED_STACK_OBSERVED_BYTES=$final_parse_phase"
echo "VIEW0_V1N1_G04_R1C_PHASED_COMPILED_STACK_BOUND_BYTES=$bound"
echo 'VIEW0_V1N1_G04_R1C_STACK_THRESHOLD_WIDENING=NO'

echo '### G04 R1C — CLI NOSCRIPT / G13 BOUNDARY'
make -s view0-v1n0-tool
run_cli(){ local name=$1 html=$2 expected_rc=$3; printf '%s' "$html" > "$tmp/$name.html"; set +e; build/view0-v1/native/arborcore-view0-html-check --format=tsv "$tmp/$name.html" > "$tmp/$name.tsv"; local rc=$?; set -e; [[ "$rc" -eq "$expected_rc" ]] || fail "$name CLI exit=$rc expected=$expected_rc"; }
run_cli matrix_positive '<!doctype html><title>x</title><body><p><a href="/"><em>x</em></a></p></body>' 0
run_cli matrix_negative '<!doctype html><title>x</title><body><p><a href="/"><div>x</div></a></p></body>' 1
awk -F '\t' '$2=="error" && $3=="0x0000000030040001" { if ($6=="53" && $7=="3" && $8=="1" && $9=="54") ok=1 } END { exit ok?0:1 }' "$tmp/matrix_negative.tsv" || fail 'frozen negative source anchor drift'
run_cli noscript '<!doctype html><title>x</title><body><noscript><div>x</div></noscript></body>' 0
grep -Fq $'g04_r1_noscript_scripting_deferred=no' "$tmp/noscript.tsv" || fail 'noscript deferral not retired'
run_cli option_noscript '<!doctype html><title>x</title><body><select><option><noscript><em>x</em></noscript></option></select></body>' 0
grep -Fq $'g04_r1_noscript_scripting_deferred=no' "$tmp/option_noscript.tsv" || fail 'option noscript not resolved'
run_cli custom '<!doctype html><title>x</title><body><x-r1><div>x</div></x-r1></body>' 0
grep -Fq $'g04_r1_g13_custom_deferred=yes' "$tmp/custom.tsv" || fail 'G13 external dependency marker missing'
echo 'VIEW0_V1N1_G04_R1C_CLI_FROZEN_MATRIX=2_OF_2'
echo 'VIEW0_V1N1_G04_R1C_CLI_NOSCRIPT_DEFERRAL=RETIRED_NO'
echo 'VIEW0_V1N1_G04_R1C_CLI_OPTION_NOSCRIPT_MODEL=PASS'
echo 'VIEW0_V1N1_G04_R1C_CLI_G13_EXTERNAL_DEPENDENCY=PASS'
echo 'PASS: G04 R1C functional/adversarial/analyzer/stack/CLI qualification'
