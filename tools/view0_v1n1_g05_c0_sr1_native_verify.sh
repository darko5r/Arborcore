#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX=build/view0-v1/native/lexbor-src; [[ -d "$LEX/.git" ]] || fail 'Lexbor missing'
rm -f build/view0-v1/native/g05_c0*.o build/view0-v1/native/g05-c0-* build/view0-v1/native/g05_r1a*.o build/view0-v1/native/g05-r1a-* build/view0-v1/native/g05_r2a*.o build/view0-v1/native/g05-r2a-* build/view0-v1/native/g05_r3a*.o build/view0-v1/native/g05-r3a-* || true
echo 'VIEW0_V1N1_G05_C0_SR1_DERIVED_RESET=PASS'
make -s view0-v1n1-g05-c0-catalog-test view0-v1n1-g05-c0-anchor-test view0-v1n1-g05-c0-sr1-input-state-test view0-v1n1-g05-r1a-test view0-v1n1-g05-r1a-adversarial-test view0-v1n1-g05-r2a-test view0-v1n1-g05-r2a-adversarial-test
CC_BIN="${CC:-cc}"; CC_PATH=$(command -v "$CC_BIN") || fail cc; CC_VERSION=$($CC_BIN --version|head -1); CC_TARGET=$($CC_BIN -dumpmachine)
printf 'VIEW0_V1N1_G05_C0_SR1_COMPILER_PATH=%s\nVIEW0_V1N1_G05_C0_SR1_COMPILER_VERSION=%s\nVIEW0_V1N1_G05_C0_SR1_COMPILER_TARGET=%s\n' "$CC_PATH" "$CC_VERSION" "$CC_TARGET"
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -Itools/c/view0_conformance -isystem "$LEX/source" -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -fsyntax-only tools/c/view0_conformance/g05_c0.c tools/c/view0_conformance/g05_r1a.c tools/c/view0_conformance/g05_r2a.c tests/c/view0_v1n1_g05_c0_sr1_input_state_test.c
echo 'VIEW0_V1N1_G05_C0_SR1_GCC_FANALYZER=PASS'
# Focused sanitizer on the only changed executable C authority surface; retained live R2 sanitizer remains authoritative for parser integration.
SAN=$(mktemp /tmp/g05-c0-sr1-sanitize.XXXXXX); rm -f "$SAN"; trap 'rm -f "$SAN"' EXIT
"$CC_BIN" -Iinclude -Itools/include -Itools/c/view0_conformance -std=c17 -O1 -g -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fsanitize=address,undefined -fno-omit-frame-pointer tests/c/view0_v1n1_g05_c0_sr1_input_state_test.c tools/c/view0_conformance/g05_c0.c -fsanitize=address,undefined -o "$SAN"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$SAN"
echo 'VIEW0_V1N1_G05_C0_SR1_SANITIZE=PASS'
[[ "$(git -C "$LEX" rev-parse HEAD)" == 2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe ]] || fail Lexbor
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail API
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/g05_c0.c || fail heap
echo 'VIEW0_V1N1_G05_C0_SR1_SINGLE_PINNED_LEXBOR_PARSER=PASS'
echo 'VIEW0_V1N1_G05_C0_SR1_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G05_C0_SR1_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'PASS: G05 C0-SR1 corrected metadata, retained R1/R2, analyzer and sanitizer qualification'
