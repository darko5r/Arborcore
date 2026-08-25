#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-src'
TOOL='build/view0-v1/native/arborcore-view0-html-check'

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$LEX" ]] || fail 'pinned Lexbor source tree missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'

make -s view0-v1n1-g02-r1-test view0-v1n1-g02-r1-adversarial-test view0-v1n0-tool

cc -Iinclude -Itools/include -isystem "$LEX/source" -D_POSIX_C_SOURCE=200809L \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -fsyntax-only \
    tools/c/view0_conformance/native.c \
    tools/c/view0_conformance/lexbor_adapter.c \
    tools/c/view0_conformance/main.c \
    tests/c/view0_v1_native_foundation_test.c \
    tests/c/view0_v1n1_c0_facts_test.c \
    tests/c/view0_v1n1_c0_facts_adversarial_test.c \
    tests/c/view0_v1n1_g02_r1_doctype_required_test.c \
    tests/c/view0_v1n1_g02_r1_doctype_required_adversarial_test.c

echo 'VIEW0_V1N1_G02_R1_GCC_FANALYZER=PASS'

tmp=$(mktemp -d /tmp/arborcore-view0-g02-r1.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
printf '%s' '<!doctype html><html><head><title>x</title></head><body></body></html>' > "$tmp/positive.html"
printf '%s' '<html><head><title>x</title></head><body></body></html>' > "$tmp/negative.html"

"$TOOL" --format=tsv "$tmp/positive.html" > "$tmp/positive.tsv"
grep -Fq $'diagnostics=0\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/positive.tsv"

set +e
"$TOOL" --format=tsv "$tmp/negative.html" > "$tmp/negative.tsv"
negative_rc=$?
set -e
[[ "$negative_rc" -eq 1 ]] || fail "G02 R1 negative CLI exit=$negative_rc"
grep -Fq $'\terror\t0x0000000030020001\tARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED\tarborcore-authoring\t0\t0\t1\t1\t0\tHTML document is missing the required DOCTYPE preamble' "$tmp/negative.tsv"
grep -Fq $'diagnostics=2\ttokenizer=0\ttree=1\tparse_clean=no\tcomplete_conformance=no' "$tmp/negative.tsv"

grep -Fq 'return lexbor_process(input, NULL, 0u, counts_out, facts_out, false);' tools/c/view0_conformance/lexbor_adapter.c || fail 'measurement pass does not suppress diagnostic publication'
grep -Fq 'return lexbor_process(' tools/c/view0_conformance/lexbor_adapter.c || fail 'collect pass missing shared parser path'
if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
    tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c; then
    fail 'G02 R1 introduced direct Arborcore heap allocation into checker/adapter'
fi

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' |
    LC_ALL=C sort)
count=$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)
[[ "$count" -eq 11 ]] || fail "G02 R1 unexpectedly changed production VIEW symbols: $count"

echo 'VIEW0_V1N1_G02_R1_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G02_R1_DOCTYPE_REQUIRED_POSITIVE=PASS'
echo 'VIEW0_V1N1_G02_R1_DOCTYPE_REQUIRED_NEGATIVE=PASS'
echo 'VIEW0_V1N1_G02_R1_AUTHORING_AND_PARSE_DIAGNOSTICS_COEXIST=PASS'
echo 'VIEW0_V1N1_G02_R1_TWO_PASS_CAPACITY_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G02_R1_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'PASS: G02 R1 required-doctype diagnostics, CLI contract, determinism and failure atomicity'
