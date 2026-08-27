#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src
[[ -d "$LEX/.git" ]] || fail 'exact pinned Lexbor source missing'

echo '### G04 R2 — RETAINED G02/G03 + G04 R1A/R1B/R1C'
bash tools/view0_v1n1_g04_r1c_native_verify.sh

echo '### G04 R2 — FUNCTIONAL / ADVERSARIAL / GLOBAL ATOMICITY'
make -s view0-v1n1-g04-r2-test view0-v1n1-g04-r2-adversarial-test view0-v1n1-g04-r2-global-failure-atomicity-test
for p in build/view0-v1/native/g04-r2-parentless-flow-test build/view0-v1/native/g04-r2-parentless-flow-adversarial-test build/view0-v1/native/g04-r2-global-failure-atomicity-test; do [[ -x "$p" ]] || fail "R2 binary missing: $p"; done

CC_BIN="${CC:-cc}"; CC_PATH=$(command -v "$CC_BIN") || fail "compiler missing: $CC_BIN"
CC_VERSION=$($CC_BIN --version | head -1); CC_TARGET=$($CC_BIN -dumpmachine)
COMMON_FLAGS='-std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef'
printf 'VIEW0_V1N1_G04_R2_COMPILER_PATH=%s\n' "$CC_PATH"
printf 'VIEW0_V1N1_G04_R2_COMPILER_VERSION=%s\n' "$CC_VERSION"
printf 'VIEW0_V1N1_G04_R2_COMPILER_TARGET=%s\n' "$CC_TARGET"
printf 'VIEW0_V1N1_G04_R2_COMPILE_FLAGS=%s\n' "$COMMON_FLAGS"

echo '### G04 R2 — PINNED SINGLE-PARSER FRAGMENT MODE'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'
[[ "$(grep -Fc 'lxb_html_parse_fragment_chunk_begin(' tools/c/view0_conformance/lexbor_adapter.c)" -eq 1 ]] || fail 'fragment begin API count drift'
[[ "$(grep -Fc 'LXB_TAG_BODY, LXB_NS_HTML' tools/c/view0_conformance/lexbor_adapter.c)" -eq 1 ]] || fail 'fragment BODY/HTML context drift'
[[ "$(grep -Fc 'lxb_html_parser_scripting_set(parser, false);' tools/c/view0_conformance/lexbor_adapter.c)" -eq 2 ]] || fail 'document+fragment scripting mode bind drift'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/g04_r2a.c || fail 'direct R2 evaluator heap allocation appeared'
echo 'VIEW0_V1N1_G04_R2_SINGLE_PINNED_LEXBOR_PARSER=PASS'
echo 'VIEW0_V1N1_G04_R2_FRAGMENT_CONTEXT=BODY_HTML_NAMESPACE_PASS'
echo 'VIEW0_V1N1_G04_R2_FRAGMENT_SCRIPTING_MODE=DISABLED_PASS'
echo 'VIEW0_V1N1_G04_R2_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G04_R2_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'

echo '### G04 R2 — GCC ANALYZER'
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c \
  tools/c/view0_conformance/g03_provenance.c \
  tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c tools/c/view0_conformance/g03_r4a.c \
  tools/c/view0_conformance/g03_r5a.c tools/c/view0_conformance/g03_r7a.c \
  tools/c/view0_conformance/g04_r1a.c tools/c/view0_conformance/g04_r2a.c \
  tools/c/view0_conformance/main.c \
  tests/c/view0_v1n1_g04_r2_parentless_flow_test.c \
  tests/c/view0_v1n1_g04_r2_parentless_flow_adversarial_test.c \
  tests/c/view0_v1n1_g04_r2_global_failure_atomicity_test.c
echo 'VIEW0_V1N1_G04_R2_GCC_FANALYZER=PASS'

echo '### G04 R2 — COMPILED STACK / EXISTING 900000-BYTE PHASED ADMISSION'
tmp=$(mktemp -d /tmp/arborcore-g04-r2-native.XXXXXX); trap 'rm -rf "$tmp"' EXIT
common=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fstack-usage)
for unit in native lexbor_adapter g04_r2a; do "$CC_BIN" "${common[@]}" -c "tools/c/view0_conformance/$unit.c" -o "$tmp/$unit.o"; done
stack_field(){ awk -F '\t' -v p="$2" '$1 ~ p {print $2; exit}' "$1"; }
fragment_check=$(stack_field "$tmp/native.su" 'arbor_view0_native_check_fragment_model$')
r2_measure=$(stack_field "$tmp/g04_r2a.su" 'arbor_view0_native_g04_r2a_measure_fragment_model$')
r2_collect=$(stack_field "$tmp/g04_r2a.su" 'arbor_view0_native_g04_r2a_collect_fragment_anchors$')
r2_eval=$(stack_field "$tmp/g04_r2a.su" ':evaluate$')
fragment_observe=$(stack_field "$tmp/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe_fragment_model$')
fragment_exact=$(stack_field "$tmp/lexbor_adapter.su" 'arbor_view0_native_lexbor_fragment_collect_exact$')
fragment_process=$(stack_field "$tmp/lexbor_adapter.su" ':lexbor_fragment_process$')
bound=$(sed -n 's/^VIEW0_V1N1_G04_R2_PHASED_COMPILED_STACK_BOUND_BYTES=//p' view/arborcore-view-core-1.contract | head -1)
for v in "$fragment_check" "$r2_measure" "$r2_collect" "$r2_eval" "$fragment_observe" "$fragment_exact" "$fragment_process" "$bound"; do [[ -n "$v" ]] || fail 'missing R2 stack record/bound'; done
[[ "$bound" == '900000' ]] || fail "R2 phased bound widened/drifted: $bound"
measure_phase=$((fragment_check + r2_measure + r2_eval + fragment_observe + fragment_process))
collect_phase=$((fragment_check + r2_collect + r2_eval + fragment_observe + fragment_process))
final_phase=$((fragment_check + fragment_exact + fragment_process))
(( measure_phase <= bound )) || fail "R2 measure stack exceeded observed=$measure_phase bound=$bound"
(( collect_phase <= bound )) || fail "R2 collect stack exceeded observed=$collect_phase bound=$bound"
(( final_phase <= bound )) || fail "R2 final fragment stack exceeded observed=$final_phase bound=$bound"
echo "VIEW0_V1N1_G04_R2_EVALUATE_COMPILED_STACK_BYTES=$r2_eval"
echo "VIEW0_V1N1_G04_R2_MEASURE_PHASED_COMPILED_STACK_OBSERVED_BYTES=$measure_phase"
echo "VIEW0_V1N1_G04_R2_COLLECT_PHASED_COMPILED_STACK_OBSERVED_BYTES=$collect_phase"
echo "VIEW0_V1N1_G04_R2_FINAL_FRAGMENT_PHASED_COMPILED_STACK_OBSERVED_BYTES=$final_phase"
echo "VIEW0_V1N1_G04_R2_PHASED_COMPILED_STACK_BOUND_BYTES=$bound"
echo 'VIEW0_V1N1_G04_R2_STACK_THRESHOLD_WIDENING=NO'

echo '### G04 R2 — CLI EXPLICIT FRAGMENT-MODE CONTROLS'
make -s view0-v1n0-tool
run_cli(){ local name=$1 html=$2 expected_rc=$3; printf '%s' "$html" > "$tmp/$name.html"; set +e; build/view0-v1/native/arborcore-view0-html-check --fragment-model --format=tsv "$tmp/$name.html" > "$tmp/$name.tsv"; local rc=$?; set -e; [[ "$rc" -eq "$expected_rc" ]] || fail "$name CLI exit=$rc expected=$expected_rc"; }
run_cli positive '<a><div>x</div></a>' 0
grep -Fq $'mode=fragment-model' "$tmp/positive.tsv" || fail 'fragment summary mode missing'
grep -Fq $'g04_r2=standard-complete' "$tmp/positive.tsv" || fail 'R2 standard completion summary missing'
run_cli negative '<a><html></html></a>' 1
awk -F '\t' '$2=="error" && $3=="0x0000000030040002" { if ($6=="4" && $7=="4" && $8=="1" && $9=="5" && $4=="ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW") ok=1 } END { exit ok?0:1 }' "$tmp/negative.tsv" || fail 'R2 CLI frozen negative anchor drift'
run_cli custom '<a><x-r2></x-r2></a>' 0
grep -Fq $'g04_r2_g13_custom_deferred=yes' "$tmp/custom.tsv" || fail 'R2 CLI G13 deferral missing'
set +e; build/view0-v1/native/arborcore-view0-html-check --fragment-model --fragment-model "$tmp/positive.html" >/dev/null 2>&1; duplicate_rc=$?; set -e
[[ "$duplicate_rc" -eq 2 ]] || fail "duplicate fragment-mode option exit=$duplicate_rc expected=2"
echo 'VIEW0_V1N1_G04_R2_CLI_FROZEN_MATRIX=2_OF_2'
echo 'VIEW0_V1N1_G04_R2_CLI_FRAGMENT_MODE_EXPLICIT=PASS'
echo 'VIEW0_V1N1_G04_R2_CLI_NEGATIVE_ANCHOR=PASS_OFFSET_4_LENGTH_4_LINE_1_COLUMN_5'
echo 'VIEW0_V1N1_G04_R2_CLI_G13_EXTERNAL_DEPENDENCY=PASS'
echo 'PASS: G04 R2 functional/adversarial/analyzer/stack/CLI qualification'
