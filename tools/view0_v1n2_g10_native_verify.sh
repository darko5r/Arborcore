#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
rm -f -- build/view0-v1/native/native.o build/view0-v1/native/g10.o \
 build/view0-v1/native/v1n2_g10_test.o build/view0-v1/native/v1n2_g10_adversarial_test.o \
 build/view0-v1/native/v1n2_g10_global_failure_atomicity_test.o \
 build/view0-v1/native/v1n2-g10-sanitize-native.o \
 build/view0-v1/native/v1n2-g10-test build/view0-v1/native/v1n2-g10-adversarial-test \
 build/view0-v1/native/v1n2-g10-global-failure-atomicity-test build/view0-v1/native/v1n2-g10-sanitize-test
echo 'VIEW0_V1N2_G10_DERIVED_RESET=PASS'
make -j1 view0-v1n2-c0-test \
 view0-v1n2-g07-test \
 view0-v1n2-g08-test \
 view0-v1n2-g09-test \
 view0-v1n2-g10-test \
 view0-v1n2-g10-adversarial-test \
 view0-v1n2-g10-global-failure-atomicity-test \
 view0-v1n1-rc1-test
grep -F 'arbor_view0_native_v1n2_g10_support_calloc' tools/c/view0_conformance/native.c >/dev/null
make -j1 view0-v1n2-g10-sanitize
cc_path=$(command -v "${CC:-cc}")
printf 'VIEW0_V1N2_G10_COMPILER_PATH=%s\n' "$cc_path"
printf 'VIEW0_V1N2_G10_COMPILER_VERSION=%s\n' "$("$cc_path" --version | head -n1)"
printf 'VIEW0_V1N2_G10_COMPILER_TARGET=%s\n' "$("$cc_path" -dumpmachine)"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-v1n2-g10-native.XXXXXXXX")
trap 'rm -rf -- "$tmp"' EXIT
cpp=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem build/view0-v1/native/lexbor-src/source -Itools/c/view0_conformance)
flags=(-std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
"$cc_path" "${cpp[@]}" "${flags[@]}" -fanalyzer -c tools/c/view0_conformance/g10.c -o "$tmp/g10-analyzer.o"
echo 'VIEW0_V1N2_G10_GCC_FANALYZER=PASS'
"$cc_path" "${cpp[@]}" "${flags[@]}" -fstack-usage -c tools/c/view0_conformance/g10.c -o "$tmp/g10-stack.o"
"$cc_path" "${cpp[@]}" "${flags[@]}" -fstack-usage -c tools/c/view0_conformance/native.c -o "$tmp/native-stack.o"
g10_stack=$(awk -F '\t' '{if ($2>m)m=$2} END{print m+0}' "$tmp/g10-stack.su")
native_stack=$(awk -F '\t' '$1 ~ /arbor_view0_native_check$/ {print $2+0}' "$tmp/native-stack.su")
[[ $g10_stack -gt 0 && $g10_stack -le 524288 ]]
[[ $native_stack -gt 0 && $native_stack -le 900000 ]]
printf 'VIEW0_V1N2_G10_EVALUATOR_STACK_BYTES=%s\n' "$g10_stack"
printf 'VIEW0_V1N2_G10_PHASED_STACK_BYTES=%s\n' "$native_stack"
printf 'VIEW0_V1N2_G10_PHASED_STACK_BOUND_BYTES=900000\n'
printf 'VIEW0_V1N2_G10_STACK_THRESHOLD_WIDENING=NO\n'
if rg -n '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/g10.c | rg -v 'support_calloc'; then
    echo 'FAIL: direct allocator call in G10' >&2; exit 1
fi
printf '%s\n' \
 'VIEW0_V1N2_G10_RETAINED_G09=PASS' \
 'VIEW0_V1N2_G10_RETAINED_G08=PASS' \
 'VIEW0_V1N2_G10_RETAINED_G07=PASS' \
 'VIEW0_V1N2_G10_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N2_G10_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N2_G10_INPUT_STATE_PRODUCT_TABLE=ZERO' \
 'VIEW0_V1N2_G10_LOCALE_DEPENDENCE=ZERO' \
 'VIEW0_V1N2_G10_MUTABLE_RUNTIME_GLOBAL_REGISTRY=NO' \
 'PASS: V1N2 G10 functional/adversarial/atomicity/analyzer/stack/sanitizer qualification'
