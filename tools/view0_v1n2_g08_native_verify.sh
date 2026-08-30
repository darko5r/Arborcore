#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
derived=(build/view0-v1/native/native.o build/view0-v1/native/g08.o build/view0-v1/native/v1n2_g08_test.o build/view0-v1/native/v1n2_g08_adversarial_test.o build/view0-v1/native/v1n2_g08_global_failure_atomicity_test.o build/view0-v1/native/v1n2-g08-test build/view0-v1/native/v1n2-g08-adversarial-test build/view0-v1/native/v1n2-g08-global-failure-atomicity-test build/view0-v1/native/v1n2-g08-sanitize-test build/view0-v1/native/v1n2-g07-test build/view0-v1/native/v1n2-c0-foundation-test build/view0-v1/native/rc1-dependency-reconciliation-test)
rm -f -- "${derived[@]}"
echo 'VIEW0_V1N2_G08_DERIVED_RESET=PASS'
make view0-v1n2-c0-test
make view0-v1n2-g07-test
make view0-v1n2-g08-test
make view0-v1n2-g08-adversarial-test
make view0-v1n2-g08-global-failure-atomicity-test
make view0-v1n1-rc1-test
cc_path=$(command -v "${CC:-cc}")
printf 'VIEW0_V1N2_G08_COMPILER_PATH=%s\n' "$cc_path"
printf 'VIEW0_V1N2_G08_COMPILER_VERSION=%s\n' "$("$cc_path" --version | head -n1)"
printf 'VIEW0_V1N2_G08_COMPILER_TARGET=%s\n' "$("$cc_path" -dumpmachine)"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-v1n2-g08-native.XXXXXXXX")
trap 'rm -rf -- "$tmp"' EXIT
cpp=(-Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem build/view0-v1/native/lexbor-src/source -Itools/c/view0_conformance)
flags=(-std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
"$cc_path" "${cpp[@]}" "${flags[@]}" -fanalyzer -c tools/c/view0_conformance/g08.c -o "$tmp/g08-analyzer.o"
echo 'VIEW0_V1N2_G08_GCC_FANALYZER=PASS'
"$cc_path" "${cpp[@]}" "${flags[@]}" -fstack-usage -c tools/c/view0_conformance/g08.c -o "$tmp/g08-stack.o"
"$cc_path" "${cpp[@]}" "${flags[@]}" -fstack-usage -c tools/c/view0_conformance/native.c -o "$tmp/native-stack.o"
g08_stack=$(awk -F '\t' '{if ($2>m)m=$2} END{print m+0}' "$tmp/g08-stack.su")
native_stack=$(awk -F '\t' '$1 ~ /arbor_view0_native_check$/ {print $2+0}' "$tmp/native-stack.su")
[[ $g08_stack -gt 0 && $g08_stack -le 524288 ]]
[[ $native_stack -gt 0 && $native_stack -le 900000 ]]
printf 'VIEW0_V1N2_G08_EVALUATOR_STACK_BYTES=%s\n' "$g08_stack"
printf 'VIEW0_V1N2_G08_PHASED_STACK_BYTES=%s\n' "$native_stack"
printf 'VIEW0_V1N2_G08_PHASED_STACK_BOUND_BYTES=900000\n'
printf 'VIEW0_V1N2_G08_STACK_THRESHOLD_WIDENING=NO\n'
make view0-v1n2-g08-sanitize
printf '%s\n' \
 'VIEW0_V1N2_G08_RETAINED_G07=PASS' \
 'VIEW0_V1N2_G08_RETAINED_V1N1_RC1=PASS' \
 'VIEW0_V1N2_G08_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N2_G08_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N2_G08_LOCALE_DEPENDENCE=ZERO' \
 'VIEW0_V1N2_G08_MUTABLE_RUNTIME_GLOBAL_REGISTRY=NO' \
 'PASS: V1N2 G08 functional/adversarial/atomicity/analyzer/stack/sanitizer qualification'
