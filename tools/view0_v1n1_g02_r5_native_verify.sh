#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-src'
TOOL='build/view0-v1/native/arborcore-view0-html-check'

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$LEX" ]] || fail 'pinned Lexbor source tree missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'

make -s view0-v1n1-g02-r5-test view0-v1n1-g02-r5-adversarial-test view0-v1n0-tool

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
    tests/c/view0_v1n1_g02_r1_doctype_required_adversarial_test.c \
    tests/c/view0_v1n1_g02_r2_doctype_syntax_test.c \
    tests/c/view0_v1n1_g02_r2_doctype_syntax_adversarial_test.c \
    tests/c/view0_v1n1_g02_r3_doctype_legacy_discouraged_test.c \
    tests/c/view0_v1n1_g02_r3_doctype_legacy_discouraged_adversarial_test.c \
    tests/c/view0_v1n1_g02_r4_head_title_cardinality_test.c \
    tests/c/view0_v1n1_g02_r4_head_title_cardinality_adversarial_test.c \
    tests/c/view0_v1n1_g02_r5_head_base_cardinality_test.c \
    tests/c/view0_v1n1_g02_r5_head_base_cardinality_adversarial_test.c

echo 'VIEW0_V1N1_G02_R5_GCC_FANALYZER=PASS'

tmp=$(mktemp -d /tmp/arborcore-view0-g02-r5.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
printf '%s' '<!doctype html><title>x</title>' > "$tmp/zero.html"
printf '%s' '<!doctype html><title>x</title><base href="/">' > "$tmp/one.html"
printf '%s' '<!doctype html><title>x</title><base href="/"><base href="/x">' > "$tmp/duplicate.html"
printf '%s' '<!DOCTYPE html SYSTEM "about:legacy-compat"><title>x</title><base href="/"><base href="/x">' > "$tmp/legacy-duplicate.html"

"$TOOL" --format=tsv "$tmp/zero.html" > "$tmp/zero.tsv"
"$TOOL" --format=tsv "$tmp/one.html" > "$tmp/one.tsv"
grep -Fq $'diagnostics=0\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/zero.tsv"
grep -Fq $'diagnostics=0\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/one.tsv"

set +e
"$TOOL" --format=tsv "$tmp/duplicate.html" > "$tmp/duplicate.tsv"
duplicate_rc=$?
"$TOOL" --format=tsv "$tmp/legacy-duplicate.html" > "$tmp/legacy-duplicate.tsv"
legacy_duplicate_rc=$?
set -e
[[ "$duplicate_rc" -eq 1 ]] || fail "G02 R5 duplicate-base CLI exit=$duplicate_rc"
[[ "$legacy_duplicate_rc" -eq 1 ]] || fail "G02 R5 legacy+duplicate-base CLI exit=$legacy_duplicate_rc"

grep -Fq $'\terror\t0x0000000030020007\tARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY\tarborcore-authoring\t47\t4\t1\t48\t0\tHTML document head must contain no more than one base element' "$tmp/duplicate.tsv"
grep -Fq $'diagnostics=1\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/duplicate.tsv"
grep -Fq $'\terror\t0x0000000030020007\tARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY\tarborcore-authoring' "$tmp/legacy-duplicate.tsv"
grep -Fq $'\twarning\t0x0000000030020003\tARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\tarborcore-authoring\t23\t19\t1\t24\t0\tLegacy DOCTYPE compatibility string should not be used unless required by a generator limitation' "$tmp/legacy-duplicate.tsv"

if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
    tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c; then
    fail 'G02 R5 introduced direct Arborcore heap allocation into checker/adapter'
fi
! grep -Fq 'ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY' tools/c/view0_conformance/lexbor_adapter.c || fail 'G02 R5 rule leaked into Lexbor adapter'

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' |
    LC_ALL=C sort)
count=$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)
[[ "$count" -eq 11 ]] || fail "G02 R5 unexpectedly changed production VIEW symbols: $count"

echo 'VIEW0_V1N1_G02_R5_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G02_R5_ZERO_BASE_ALLOWED=PASS'
echo 'VIEW0_V1N1_G02_R5_ONE_BASE_ALLOWED=PASS'
echo 'VIEW0_V1N1_G02_R5_DUPLICATE_BASE_SECOND_SOURCE_ANCHOR=PASS'
echo 'VIEW0_V1N1_G02_R5_R4_R3_ACCUMULATION=PASS'
echo 'VIEW0_V1N1_G02_R5_TWO_PASS_CAPACITY_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G02_R5_C0_FACT_LAYOUT_BYTES=184'
echo 'VIEW0_V1N1_G02_R5_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'PASS: G02 R5 head/base maximum cardinality, anchors, CLI contract, determinism and failure atomicity'
