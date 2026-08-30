#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
CC_BIN="${CC:-cc}"
CC_PATH=$(command -v "$CC_BIN") || fail "compiler missing: $CC_BIN"
printf 'VIEW0_V1N2_C0_COMPILER_PATH=%s\n' "$CC_PATH"
printf 'VIEW0_V1N2_C0_COMPILER_VERSION=%s\n' "$($CC_BIN --version | head -1)"
printf 'VIEW0_V1N2_C0_COMPILER_TARGET=%s\n' "$($CC_BIN -dumpmachine)"
rm -f \
  build/view0-v1/native/native.o \
  build/view0-v1/native/v1n2_c0.o \
  build/view0-v1/native/v1n2_c0_foundation_test.o \
  build/view0-v1/native/v1n2-c0-foundation-test \
  build/view0-v1/native/rc1-dependency-reconciliation-test
echo 'VIEW0_V1N2_C0_DERIVED_RESET=NATIVE_C0_OBJECTS_TEST_AND_RETAINED_RC1_TEST'
make -s view0-v1n2-c0-test
"$CC_BIN" -Iinclude -Itools/include -Itools/c/view0_conformance \
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer \
  -fsyntax-only tools/c/view0_conformance/v1n2_c0.c \
  tests/c/view0_v1n2_c0_foundation_test.c
echo 'VIEW0_V1N2_C0_GCC_FANALYZER=PASS'
tmp=$(mktemp -d /tmp/arborcore-v1n2-c0-native.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
"$CC_BIN" -Iinclude -Itools/include -Itools/c/view0_conformance \
  -std=c17 -O2 -fstack-usage -c tools/c/view0_conformance/v1n2_c0.c \
  -o "$tmp/v1n2_c0.o"
stack_bytes=$(awk -F '\t' '$1 ~ /arbor_view0_native_v1n2_c0_validate$/ {print $2; exit}' "$tmp/v1n2_c0.su")
[[ -n "$stack_bytes" ]] || fail 'V1N2 C0 compiled stack record missing'
(( stack_bytes <= 16384 )) || fail "V1N2 C0 stack bound exceeded: $stack_bytes"
printf 'VIEW0_V1N2_C0_VALIDATOR_COMPILED_STACK_BYTES=%s\n' "$stack_bytes"
echo 'VIEW0_V1N2_C0_VALIDATOR_COMPILED_STACK_BOUND_BYTES=16384'
echo 'VIEW0_V1N2_C0_PHASED_STACK_BOUND_BYTES=900000'
echo 'VIEW0_V1N2_C0_STACK_THRESHOLD_WIDENING=NO'
"$CC_BIN" -Iinclude -Itools/include -Itools/c/view0_conformance \
  -std=c17 -O1 -g -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  tests/c/view0_v1n2_c0_foundation_test.c tools/c/view0_conformance/v1n2_c0.c \
  -fsanitize=address,undefined -o "$tmp/v1n2-c0-sanitize-test"
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  "$tmp/v1n2-c0-sanitize-test"
echo 'VIEW0_V1N2_C0_SANITIZE=PASS'
make -s view0-v1n1-rc1-test
echo 'VIEW0_V1N2_C0_RETAINED_V1N1_RC1=PASS'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" == 11 ]] || fail 'production VIEW API count changed'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/v1n2_c0.c || fail 'direct Arborcore heap allocation introduced'
! grep -Eq '\b(setlocale|localeconv|strtod|strtold|strtof)\s*\(' tools/c/view0_conformance/v1n2_c0.c || fail 'locale dependence introduced'
! grep -Pq '^\s*static\s+(?!const\b)[^()]*=' tools/c/view0_conformance/v1n2_c0.c || fail 'mutable runtime global suspected'
[[ "$(grep -Fc 'arbor_view0_native_v1n2_c0_validate()' tools/c/view0_conformance/native.c)" == 1 ]] || fail 'C0 coordinator integration drift'
echo 'VIEW0_V1N2_C0_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N2_C0_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N2_C0_LOCALE_DEPENDENCE=ZERO'
echo 'VIEW0_V1N2_C0_MUTABLE_RUNTIME_GLOBAL_REGISTRY=NO'
echo 'PASS: V1N2 C0 analyzer, stack, sanitizer and retained V1N1 qualification'
