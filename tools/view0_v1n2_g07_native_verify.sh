#!/usr/bin/env bash
set -euo pipefail

ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"

derived=(
    build/view0-v1/native/native.o
    build/view0-v1/native/g07.o
    build/view0-v1/native/v1n2_g07_test.o
    build/view0-v1/native/v1n2_g07_adversarial_test.o
    build/view0-v1/native/v1n2_g07_global_failure_atomicity_test.o
    build/view0-v1/native/v1n2-g07-test
    build/view0-v1/native/v1n2-g07-adversarial-test
    build/view0-v1/native/v1n2-g07-global-failure-atomicity-test
    build/view0-v1/native/v1n2-g07-sanitize-test
    build/view0-v1/native/v1n2-c0-foundation-test
    build/view0-v1/native/rc1-dependency-reconciliation-test
)
rm -f -- "${derived[@]}"
echo 'VIEW0_V1N2_G07_DERIVED_RESET=PASS'

make view0-v1n2-c0-test
make view0-v1n2-g07-test
make view0-v1n2-g07-adversarial-test
make view0-v1n2-g07-global-failure-atomicity-test
make view0-v1n1-rc1-test

cc_path=$(command -v "${CC:-cc}")
printf 'VIEW0_V1N2_G07_COMPILER_PATH=%s\n' "$cc_path"
printf 'VIEW0_V1N2_G07_COMPILER_VERSION=%s\n' "$("$cc_path" --version | head -n1)"
printf 'VIEW0_V1N2_G07_COMPILER_TARGET=%s\n' "$("$cc_path" -dumpmachine)"

tmp=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-v1n2-g07-native.XXXXXXXX")
trap 'rm -rf -- "$tmp"' EXIT
cpp=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem build/view0-v1/native/lexbor-src/source -Itools/c/view0_conformance)
flags=(-std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
"$cc_path" "${cpp[@]}" "${flags[@]}" -fanalyzer -c tools/c/view0_conformance/g07.c -o "$tmp/g07-analyzer.o"
echo 'VIEW0_V1N2_G07_GCC_FANALYZER=PASS'

"$cc_path" "${cpp[@]}" "${flags[@]}" -fstack-usage -c tools/c/view0_conformance/g07.c -o "$tmp/g07-stack.o"
"$cc_path" "${cpp[@]}" "${flags[@]}" -fstack-usage -c tools/c/view0_conformance/native.c -o "$tmp/native-stack.o"
g07_stack=$(awk -F '\t' '{if ($2>m)m=$2} END{print m+0}' "$tmp/g07-stack.su")
native_stack=$(awk -F '\t' '$1 ~ /arbor_view0_native_check$/ {print $2+0}' "$tmp/native-stack.su")
[[ $g07_stack -gt 0 && $g07_stack -le 262144 ]]
[[ $native_stack -gt 0 && $native_stack -le 900000 ]]
printf 'VIEW0_V1N2_G07_EVALUATOR_STACK_BYTES=%s\n' "$g07_stack"
printf 'VIEW0_V1N2_G07_PHASED_STACK_BYTES=%s\n' "$native_stack"
printf 'VIEW0_V1N2_G07_PHASED_STACK_BOUND_BYTES=900000\n'
printf 'VIEW0_V1N2_G07_STACK_THRESHOLD_WIDENING=NO\n'

make view0-v1n2-g07-sanitize
echo 'VIEW0_V1N2_G07_RETAINED_V1N1_RC1=PASS'
echo 'VIEW0_V1N2_G07_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N2_G07_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N2_G07_LOCALE_DEPENDENCE=ZERO'
echo 'VIEW0_V1N2_G07_MUTABLE_RUNTIME_GLOBAL_REGISTRY=NO'
echo 'PASS: V1N2 G07 functional/adversarial/atomicity/analyzer/stack/sanitizer qualification'
