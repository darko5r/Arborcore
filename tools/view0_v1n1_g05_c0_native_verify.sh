#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src
[[ -d "$LEX/.git" ]] || fail 'pinned Lexbor missing'
rm -f build/view0-v1/native/{native,lexbor_adapter,g03_c0_provenance,g03_r1a,g03_r2a,g03_r3a,g03_r4a,g03_r5a,g03_r7a,g04_r1a,g04_r2a,g05_c0,g05_r1a}.o \
      build/view0-v1/native/g05_c0_* build/view0-v1/native/g05-c0-* \
      build/view0-v1/native/g04-r1*-test build/view0-v1/native/g04-r2-*test || true
echo 'VIEW0_V1N1_G05_C0_DERIVED_RESET=PASS'
make -s view0-v1n1-g05-c0-catalog-test view0-v1n1-g05-c0-anchor-test
make -s view0-v1n1-g04-r1a-test view0-v1n1-g04-r1a-adversarial-test \
        view0-v1n1-g04-r1a-global-failure-atomicity-test \
        view0-v1n1-g04-r1b-test view0-v1n1-g04-r1b-adversarial-test \
        view0-v1n1-g04-r1c-test view0-v1n1-g04-r1c-adversarial-test \
        view0-v1n1-g04-r2-test view0-v1n1-g04-r2-adversarial-test \
        view0-v1n1-g04-r2-global-failure-atomicity-test

echo '### G05 C0 — GCC ANALYZER / PRIVATE SINGLE-PARSER BOUNDARY'
CC_BIN="${CC:-cc}"
CC_PATH=$(command -v "$CC_BIN") || fail "compiler missing: $CC_BIN"
CC_VERSION=$($CC_BIN --version | head -1)
CC_TARGET=$($CC_BIN -dumpmachine)
printf 'VIEW0_V1N1_G05_C0_COMPILER_PATH=%s\n' "$CC_PATH"
printf 'VIEW0_V1N1_G05_C0_COMPILER_VERSION=%s\n' "$CC_VERSION"
printf 'VIEW0_V1N1_G05_C0_COMPILER_TARGET=%s\n' "$CC_TARGET"
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow \
  -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/g05_c0.c tools/c/view0_conformance/lexbor_adapter.c \
  ${ARBOR_G05_C0_EXTENSION_ANALYZER_SOURCE:-} \
  tests/c/view0_v1n1_g05_c0_catalog_test.c tests/c/view0_v1n1_g05_c0_source_attribute_anchor_test.c
echo 'VIEW0_V1N1_G05_C0_GCC_FANALYZER=PASS'

echo '### G05 C0 — RETAINED G04 COMPILED-STACK ADMISSION AFTER OBSERVER-LAYOUT EXTENSION'
stack_tmp=$(mktemp -d /tmp/arborcore-g05-c0-stack.XXXXXX)
trap 'rm -rf "$stack_tmp"' RETURN
stack_common=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fstack-usage)
for unit in native lexbor_adapter g03_r1a g03_r5a g04_r1a g04_r2a; do
  "$CC_BIN" "${stack_common[@]}" -c "tools/c/view0_conformance/$unit.c" -o "$stack_tmp/$unit.o"
done
stack_field(){ awk -F '\t' -v p="$2" '$1 ~ p {print $2; exit}' "$1"; }
native_check=$(stack_field "$stack_tmp/native.su" 'arbor_view0_native_check$')
g04_public=$(stack_field "$stack_tmp/g04_r1a.su" 'arbor_view0_native_g04_r1a_measure$')
g04_eval=$(stack_field "$stack_tmp/g04_r1a.su" ':evaluate$')
g04_prior=$(stack_field "$stack_tmp/g04_r1a.su" 'collect_prior_error_anchors')
g04_with_prior=$(stack_field "$stack_tmp/g04_r1a.su" 'evaluate_with_prior_anchors')
r5_public=$(stack_field "$stack_tmp/g03_r5a.su" 'arbor_view0_native_g03_r5a_measure$')
r5_eval=$(stack_field "$stack_tmp/g03_r5a.su" ':evaluate$')
r5_prior=$(stack_field "$stack_tmp/g03_r5a.su" 'collect_prior_error_offsets')
r1_public=$(stack_field "$stack_tmp/g03_r1a.su" 'arbor_view0_native_g03_r1a_measure$')
r1_eval=$(stack_field "$stack_tmp/g03_r1a.su" ':evaluate$')
lex_observe=$(stack_field "$stack_tmp/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe$')
lex_process=$(stack_field "$stack_tmp/lexbor_adapter.su" ':lexbor_process$')
fragment_check=$(stack_field "$stack_tmp/native.su" 'arbor_view0_native_check_fragment_model$')
r2_measure=$(stack_field "$stack_tmp/g04_r2a.su" 'arbor_view0_native_g04_r2a_measure_fragment_model$')
r2_collect=$(stack_field "$stack_tmp/g04_r2a.su" 'arbor_view0_native_g04_r2a_collect_fragment_anchors$')
r2_eval=$(stack_field "$stack_tmp/g04_r2a.su" ':evaluate$')
fragment_observe=$(stack_field "$stack_tmp/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe_fragment_model$')
fragment_process=$(stack_field "$stack_tmp/lexbor_adapter.su" ':lexbor_fragment_process$')
bound=$(sed -n 's/^VIEW0_V1N1_G04_R2_PHASED_COMPILED_STACK_BOUND_BYTES=//p' view/arborcore-view-core-1.contract | head -1)
for v in "$native_check" "$g04_public" "$g04_eval" "$g04_prior" "$g04_with_prior" "$r5_public" "$r5_eval" "$r5_prior" "$r1_public" "$r1_eval" "$lex_observe" "$lex_process" "$fragment_check" "$r2_measure" "$r2_collect" "$r2_eval" "$fragment_observe" "$fragment_process" "$bound"; do [[ -n "$v" ]] || fail 'missing retained G04 stack record/bound'; done
[[ "$bound" == '900000' ]] || fail "retained G04 phased bound widened/drifted: $bound"
r1_prior_phase=$((native_check + g04_public + g04_eval + g04_prior + r5_public + r5_eval + r5_prior + r1_public + r1_eval + lex_observe + lex_process))
r1_transparent_phase=$((native_check + g04_public + g04_eval + g04_with_prior + lex_observe + lex_process + 80))
r2_measure_phase=$((fragment_check + r2_measure + r2_eval + fragment_observe + fragment_process))
r2_collect_phase=$((fragment_check + r2_collect + r2_eval + fragment_observe + fragment_process))
for pair in "R1_PRIOR:$r1_prior_phase" "R1_TRANSPARENT:$r1_transparent_phase" "R2_MEASURE:$r2_measure_phase" "R2_COLLECT:$r2_collect_phase"; do
  label=${pair%%:*}; value=${pair##*:}; (( value <= bound )) || fail "retained G04 $label stack exceeded observed=$value bound=$bound"
done
printf 'VIEW0_V1N1_G05_C0_RETAINED_G04_R1_PRIOR_STACK_BYTES=%s\n' "$r1_prior_phase"
printf 'VIEW0_V1N1_G05_C0_RETAINED_G04_R1_TRANSPARENT_STACK_BYTES=%s\n' "$r1_transparent_phase"
printf 'VIEW0_V1N1_G05_C0_RETAINED_G04_R2_MEASURE_STACK_BYTES=%s\n' "$r2_measure_phase"
printf 'VIEW0_V1N1_G05_C0_RETAINED_G04_R2_COLLECT_STACK_BYTES=%s\n' "$r2_collect_phase"
printf 'VIEW0_V1N1_G05_C0_RETAINED_G04_PHASED_STACK_BOUND_BYTES=%s\n' "$bound"
echo 'VIEW0_V1N1_G05_C0_RETAINED_G04_STACK_THRESHOLD_WIDENING=NO'
rm -rf "$stack_tmp"
trap - RETURN

[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'
[[ "$(grep -Fc 'source_pointer_range(capture, attr->name_begin, attr->name_end' tools/c/view0_conformance/lexbor_adapter.c)" -eq 1 ]] || fail 'attribute source range binding drift'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'VIEW API count changed'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/g05_c0.c || fail 'direct G05 C0 heap allocation'
echo 'VIEW0_V1N1_G05_C0_SINGLE_PINNED_LEXBOR_PARSER=PASS'
echo 'VIEW0_V1N1_G05_C0_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G05_C0_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'

rm -rf build/view0-v1/native/lexbor-build-sanitize
make -s view0-v1n0-lexbor-sanitize
SAN=build/view0-v1/native/g05-c0-anchor-sanitize-test
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O1 -g -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow \
  -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fsanitize=address,undefined -fno-omit-frame-pointer \
  tests/c/view0_v1n1_g05_c0_source_attribute_anchor_test.c \
  tools/c/view0_conformance/native.c \
  tools/c/view0_conformance/lexbor_adapter.c \
  tools/c/view0_conformance/g03_provenance.c \
  tools/c/view0_conformance/g03_r1a.c \
  tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c \
  tools/c/view0_conformance/g03_r4a.c \
  tools/c/view0_conformance/g03_r5a.c \
  tools/c/view0_conformance/g03_r7a.c \
  tools/c/view0_conformance/g04_r1a.c \
  tools/c/view0_conformance/g04_r2a.c \
  tools/c/view0_conformance/g05_c0.c \
  ${ARBOR_G05_C0_EXTENSION_SANITIZE_SOURCE:-} \
  build/libarborcore_view.a build/libarborcore_runtime.a build/libarborcore.a \
  build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a \
  -no-pie -Wl,-z,relro,-z,now,-z,noexecstack -Wl,--wrap=lxb_html_interface_create -Wl,--wrap=lxb_html_tree_insert_foreign_element \
  -fsanitize=address,undefined -lm -o "$SAN"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$SAN"
echo 'VIEW0_V1N1_G05_C0_SANITIZE=PASS'
echo 'PASS: G05 C0 foundation catalogs, authored attribute anchors, retained G04 and sanitizer qualification'
