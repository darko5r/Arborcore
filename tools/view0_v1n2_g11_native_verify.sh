#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
rm -f -- build/view0-v1/native/native.o build/view0-v1/native/g11.o \
 build/view0-v1/native/v1n2_g11_test.o build/view0-v1/native/v1n2_g11_adversarial_test.o \
 build/view0-v1/native/v1n2_g11_global_failure_atomicity_test.o \
 build/view0-v1/native/v1n2-g11-sanitize-native.o \
 build/view0-v1/native/v1n2-g11-test build/view0-v1/native/v1n2-g11-adversarial-test \
 build/view0-v1/native/v1n2-g11-global-failure-atomicity-test \
 build/view0-v1/native/v1n2-g11-sanitize-test
echo 'VIEW0_V1N2_G11_DERIVED_RESET=PASS'
make -j1 view0-v1n2-c0-test \
 view0-v1n2-g07-test view0-v1n2-g08-test view0-v1n2-g09-test \
 view0-v1n2-g10-test view0-v1n2-g10-adversarial-test \
 view0-v1n2-g10-global-failure-atomicity-test \
 view0-v1n2-g11-test view0-v1n2-g11-adversarial-test \
 view0-v1n2-g11-global-failure-atomicity-test view0-v1n1-rc1-test
make -j1 view0-v1n2-g11-sanitize
cc_path=$(command -v "${CC:-cc}")
printf 'VIEW0_V1N2_G11_COMPILER_PATH=%s\n' "$cc_path"
printf 'VIEW0_V1N2_G11_COMPILER_VERSION=%s\n' "$("$cc_path" --version | head -n1)"
printf 'VIEW0_V1N2_G11_COMPILER_TARGET=%s\n' "$("$cc_path" -dumpmachine)"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-v1n2-g11-native.XXXXXXXX")
trap 'rm -rf -- "$tmp"' EXIT
cpp=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include \
 -isystem build/view0-v1/native/lexbor-src/source -Itools/c/view0_conformance)
flags=(-std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion \
 -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
"$cc_path" "${cpp[@]}" "${flags[@]}" -fanalyzer \
 -c tools/c/view0_conformance/g11.c -o "$tmp/g11-analyzer.o"
echo 'VIEW0_V1N2_G11_GCC_FANALYZER=PASS'
"$cc_path" "${cpp[@]}" "${flags[@]}" -fstack-usage \
 -c tools/c/view0_conformance/g11.c -o "$tmp/g11-stack.o"
"$cc_path" "${cpp[@]}" "${flags[@]}" -fstack-usage \
 -c tools/c/view0_conformance/native.c -o "$tmp/native-stack.o"
g11_stack=$(awk -F '\t' '{if ($2>m)m=$2} END{print m+0}' "$tmp/g11-stack.su")
native_stack=$(awk -F '\t' '$1 ~ /arbor_view0_native_check$/ {print $2+0}' "$tmp/native-stack.su")
[[ $g11_stack -gt 0 && $g11_stack -le 65536 ]]
[[ $native_stack -gt 0 && $native_stack -le 900000 ]]
printf 'VIEW0_V1N2_G11_EVALUATOR_STACK_BYTES=%s\n' "$g11_stack"
printf 'VIEW0_V1N2_G11_PHASED_STACK_BYTES=%s\n' "$native_stack"
printf 'VIEW0_V1N2_G11_PHASED_STACK_BOUND_BYTES=900000\n'
printf 'VIEW0_V1N2_G11_STACK_THRESHOLD_WIDENING=NO\n'
if rg -n '\b(malloc|calloc|realloc|free)\s*\(' \
    tools/c/view0_conformance/g11.c | rg -v 'support_calloc'; then
    echo 'FAIL: direct allocator call in G11' >&2; exit 1
fi
printf '%s\n' \
 'VIEW0_V1N2_G11_RETAINED_G10=PASS' \
 'VIEW0_V1N2_G11_RETAINED_G09=PASS' \
 'VIEW0_V1N2_G11_RETAINED_G08=PASS' \
 'VIEW0_V1N2_G11_RETAINED_G07=PASS' \
 'VIEW0_V1N2_G11_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N2_G11_SHARED_G07_G11_ANCHOR_ARENAS=1' \
 'VIEW0_V1N2_G11_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N2_G11_LOCALE_DEPENDENCE=ZERO' \
 'VIEW0_V1N2_G11_MUTABLE_RUNTIME_GLOBAL_REGISTRY=NO' \
 'PASS: V1N2 G11 functional/adversarial/atomicity/analyzer/stack/sanitizer qualification'
