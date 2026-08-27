#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src
[[ -d "$LEX/.git" ]] || fail 'pinned Lexbor missing'
rm -f build/view0-v1/native/{native,lexbor_adapter,g03_c0_provenance,g03_r1a,g03_r2a,g03_r3a,g03_r4a,g03_r5a,g03_r7a,g04_r1a,g04_r2a,g05_c0,g05_r1a}.o \
      build/view0-v1/native/g05_r1a_* build/view0-v1/native/g05-r1a-* \
      build/view0-v1/native/g05_c0_* build/view0-v1/native/g05-c0-* \
      build/view0-v1/native/g05_r2a* build/view0-v1/native/g05-r2a-* \
      build/view0-v1/native/g05_r3a* build/view0-v1/native/g05-r3a-* \
      build/view0-v1/native/g04-r1*-test build/view0-v1/native/g04-r2-*test || true
echo 'VIEW0_V1N1_G05_R1A_DERIVED_RESET=PASS'
make -s view0-v1n1-g05-c0-catalog-test view0-v1n1-g05-c0-anchor-test \
        view0-v1n1-g05-r1a-test view0-v1n1-g05-r1a-adversarial-test
make -s view0-v1n1-g04-r1a-test view0-v1n1-g04-r1a-adversarial-test \
        view0-v1n1-g04-r1a-global-failure-atomicity-test \
        view0-v1n1-g04-r1b-test view0-v1n1-g04-r1b-adversarial-test \
        view0-v1n1-g04-r1c-test view0-v1n1-g04-r1c-adversarial-test \
        view0-v1n1-g04-r2-test view0-v1n1-g04-r2-adversarial-test \
        view0-v1n1-g04-r2-global-failure-atomicity-test

echo '### G05 R1A — GCC ANALYZER'
CC_BIN="${CC:-cc}"; CC_PATH=$(command -v "$CC_BIN") || fail 'compiler'; CC_VERSION=$($CC_BIN --version|head -1); CC_TARGET=$($CC_BIN -dumpmachine)
printf 'VIEW0_V1N1_G05_R1A_COMPILER_PATH=%s\n' "$CC_PATH"
printf 'VIEW0_V1N1_G05_R1A_COMPILER_VERSION=%s\n' "$CC_VERSION"
printf 'VIEW0_V1N1_G05_R1A_COMPILER_TARGET=%s\n' "$CC_TARGET"
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow \
  -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/g05_c0.c tools/c/view0_conformance/g05_r1a.c \
  tests/c/view0_v1n1_g05_r1a_global_attribute_test.c tests/c/view0_v1n1_g05_r1a_global_attribute_adversarial_test.c
echo 'VIEW0_V1N1_G05_R1A_GCC_FANALYZER=PASS'

echo '### G05 R1A — STACK / RETAINED G04 ADMISSION'
st=$(mktemp -d /tmp/arborcore-g05-r1-stack.XXXXXX); trap 'rm -rf "$st"' RETURN
common=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fstack-usage)
for u in native lexbor_adapter g03_r1a g03_r5a g04_r1a g04_r2a g05_r1a; do "$CC_BIN" "${common[@]}" -c tools/c/view0_conformance/$u.c -o "$st/$u.o"; done
sf(){ awk -F '\t' -v p="$2" '$1 ~ p {print $2; exit}' "$1"; }
native=$(sf "$st/native.su" 'arbor_view0_native_check$'); lex=$(sf "$st/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe$'); lp=$(sf "$st/lexbor_adapter.su" ':lexbor_process$')
g05p=$(sf "$st/g05_r1a.su" 'arbor_view0_native_g05_r1a_measure$'); g05e=$(sf "$st/g05_r1a.su" ':evaluate$')
g04p=$(sf "$st/g04_r1a.su" 'arbor_view0_native_g04_r1a_measure$'); g04e=$(sf "$st/g04_r1a.su" ':evaluate$'); g04prior=$(sf "$st/g04_r1a.su" 'collect_prior_error_anchors'); g04wp=$(sf "$st/g04_r1a.su" 'evaluate_with_prior_anchors')
r5p=$(sf "$st/g03_r5a.su" 'arbor_view0_native_g03_r5a_measure$'); r5e=$(sf "$st/g03_r5a.su" ':evaluate$'); r5pr=$(sf "$st/g03_r5a.su" 'collect_prior_error_offsets')
r1p=$(sf "$st/g03_r1a.su" 'arbor_view0_native_g03_r1a_measure$'); r1e=$(sf "$st/g03_r1a.su" ':evaluate$')
frag=$(sf "$st/native.su" 'arbor_view0_native_check_fragment_model$'); r2m=$(sf "$st/g04_r2a.su" 'arbor_view0_native_g04_r2a_measure_fragment_model$'); r2e=$(sf "$st/g04_r2a.su" ':evaluate$'); fo=$(sf "$st/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe_fragment_model$'); fp=$(sf "$st/lexbor_adapter.su" ':lexbor_fragment_process$')
bound=900000
for v in "$native" "$lex" "$lp" "$g05p" "$g05e" "$g04p" "$g04e" "$g04prior" "$g04wp" "$r5p" "$r5e" "$r5pr" "$r1p" "$r1e" "$frag" "$r2m" "$r2e" "$fo" "$fp"; do [[ -n "$v" ]] || fail 'missing stack record'; done
g05phase=$((native+g05p+g05e+lex+lp))
g04priorphase=$((native+g04p+g04e+g04prior+r5p+r5e+r5pr+r1p+r1e+lex+lp))
g04transparent=$((native+g04p+g04e+g04wp+lex+lp+80))
r2phase=$((frag+r2m+r2e+fo+fp))
for pair in "G05_R1:$g05phase" "G04_PRIOR:$g04priorphase" "G04_TRANSPARENT:$g04transparent" "G04_R2:$r2phase"; do label=${pair%%:*}; val=${pair##*:}; ((val<=bound)) || fail "$label stack $val > $bound"; done
printf 'VIEW0_V1N1_G05_R1A_EVALUATE_PHASED_STACK_BYTES=%s\n' "$g05phase"
printf 'VIEW0_V1N1_G05_R1A_RETAINED_G04_R1_PRIOR_STACK_BYTES=%s\n' "$g04priorphase"
printf 'VIEW0_V1N1_G05_R1A_RETAINED_G04_R1_TRANSPARENT_STACK_BYTES=%s\n' "$g04transparent"
printf 'VIEW0_V1N1_G05_R1A_RETAINED_G04_R2_STACK_BYTES=%s\n' "$r2phase"
printf 'VIEW0_V1N1_G05_R1A_PHASED_STACK_BOUND_BYTES=%s\n' "$bound"
echo 'VIEW0_V1N1_G05_R1A_STACK_THRESHOLD_WIDENING=NO'
rm -rf "$st"; trap - RETURN

[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor drift'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'VIEW API count'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/g05_r1a.c || fail 'direct R1 heap'
echo 'VIEW0_V1N1_G05_R1A_SINGLE_PINNED_LEXBOR_PARSER=PASS'
echo 'VIEW0_V1N1_G05_R1A_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G05_R1A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'

rm -rf build/view0-v1/native/lexbor-build-sanitize
make -s view0-v1n0-lexbor-sanitize
SAN=build/view0-v1/native/g05-r1a-sanitize-test
R2_SAN_SOURCE=""; [[ ! -f tools/c/view0_conformance/g05_r2a.c ]] || R2_SAN_SOURCE=tools/c/view0_conformance/g05_r2a.c
R3_SAN_SOURCE=""; [[ ! -f tools/c/view0_conformance/g05_r3a.c ]] || R3_SAN_SOURCE=tools/c/view0_conformance/g05_r3a.c
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O1 -g -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  tests/c/view0_v1n1_g05_r1a_global_attribute_adversarial_test.c \
  tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c \
  tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c tools/c/view0_conformance/g03_r3a.c tools/c/view0_conformance/g03_r4a.c tools/c/view0_conformance/g03_r5a.c tools/c/view0_conformance/g03_r7a.c \
  tools/c/view0_conformance/g04_r1a.c tools/c/view0_conformance/g04_r2a.c tools/c/view0_conformance/g05_c0.c tools/c/view0_conformance/g05_r1a.c ${R2_SAN_SOURCE:-} ${R3_SAN_SOURCE:-} \
  build/libarborcore_view.a build/libarborcore_runtime.a build/libarborcore.a build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a \
  -no-pie -Wl,-z,relro,-z,now,-z,noexecstack -Wl,--wrap=lxb_html_interface_create -Wl,--wrap=lxb_html_tree_insert_foreign_element -fsanitize=address,undefined -lm -o "$SAN"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$SAN"
echo 'VIEW0_V1N1_G05_R1A_SANITIZE=PASS'
echo 'PASS: G05 R1A functional/adversarial/analyzer/stack/sanitizer qualification'
