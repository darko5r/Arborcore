#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-src'
TOOL='build/view0-v1/native/arborcore-view0-html-check'

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$LEX" ]] || fail 'pinned Lexbor source tree missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'

make -s view0-v1n1-g02-r6-test view0-v1n1-g02-r6-adversarial-test view0-v1n0-tool

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
    tests/c/view0_v1n1_g02_r5_head_base_cardinality_adversarial_test.c \
    tests/c/view0_v1n1_g02_r6_body_singleton_test.c \
    tests/c/view0_v1n1_g02_r6_body_singleton_adversarial_test.c

echo 'VIEW0_V1N1_G02_R6_GCC_FANALYZER=PASS'

tmp=$(mktemp -d /tmp/arborcore-view0-g02-r6.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
printf '%s' '<!doctype html><title>x</title><p>ok</p>' > "$tmp/omitted.html"
printf '%s' '<!doctype html><html><head><title>x</title></head><body><p>ok</p></body></html>' > "$tmp/one.html"
printf '%s' '<!doctype html><html><head><title>x</title></head><body></body><body></body></html>' > "$tmp/duplicate.html"
printf '%s' '<!doctype html><html><head><title>x</title></head><frameset></frameset></html>' > "$tmp/missing.html"
printf '%s' '<!DOCTYPE html SYSTEM "about:legacy-compat"><title>x</title><body></body><body></body>' > "$tmp/legacy-duplicate.html"

"$TOOL" --format=tsv "$tmp/omitted.html" > "$tmp/omitted.tsv"
"$TOOL" --format=tsv "$tmp/one.html" > "$tmp/one.tsv"
grep -Fq $'diagnostics=0\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/omitted.tsv"
grep -Fq $'diagnostics=0\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/one.tsv"

set +e
"$TOOL" --format=tsv "$tmp/duplicate.html" > "$tmp/duplicate.tsv"
duplicate_rc=$?
"$TOOL" --format=tsv "$tmp/missing.html" > "$tmp/missing.tsv"
missing_rc=$?
"$TOOL" --format=tsv "$tmp/legacy-duplicate.html" > "$tmp/legacy-duplicate.tsv"
legacy_duplicate_rc=$?
set -e
[[ "$duplicate_rc" -eq 1 ]] || fail "G02 R6 duplicate-body CLI exit=$duplicate_rc"
[[ "$missing_rc" -eq 1 ]] || fail "G02 R6 missing-body CLI exit=$missing_rc"
[[ "$legacy_duplicate_rc" -eq 1 ]] || fail "G02 R6 legacy+duplicate-body CLI exit=$legacy_duplicate_rc"

grep -Fq $'\terror\t0x0000000030020008\tARBOR_VIEW_V1_G02_BODY_SINGLETON\tarborcore-authoring\t64\t4\t1\t65\t0\tHTML document must contain exactly one logical body element' "$tmp/duplicate.tsv"
grep -Fq $'diagnostics=3\ttokenizer=0\ttree=2\tparse_clean=no\tcomplete_conformance=no' "$tmp/duplicate.tsv"
grep -Fq $'\terror\t0x0000000030020008\tARBOR_VIEW_V1_G02_BODY_SINGLETON\tarborcore-authoring\t0\t0\t1\t1\t0\tHTML document must contain exactly one logical body element' "$tmp/missing.tsv"
grep -Fq $'diagnostics=1\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/missing.tsv"
grep -Fq $'\twarning\t0x0000000030020003\tARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\tarborcore-authoring' "$tmp/legacy-duplicate.tsv"
grep -Fq $'\terror\t0x0000000030020008\tARBOR_VIEW_V1_G02_BODY_SINGLETON\tarborcore-authoring' "$tmp/legacy-duplicate.tsv"

if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
    tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c; then
    fail 'G02 R6 introduced direct Arborcore heap allocation into checker/adapter'
fi
! grep -Fq 'ARBOR_VIEW_V1_G02_BODY_SINGLETON' tools/c/view0_conformance/lexbor_adapter.c || fail 'G02 R6 rule leaked into Lexbor adapter'

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort -u)
[[ "$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)" -eq 11 ]] || fail 'production VIEW symbol count changed under G02 R6'

echo 'VIEW0_V1N1_G02_R6_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G02_R6_OMITTED_BODY_ALLOWED=PASS'
echo 'VIEW0_V1N1_G02_R6_SINGLE_BODY_ALLOWED=PASS'
echo 'VIEW0_V1N1_G02_R6_DUPLICATE_BODY_SECOND_SOURCE_ANCHOR=PASS'
echo 'VIEW0_V1N1_G02_R6_MISSING_LOGICAL_BODY=PASS'
echo 'VIEW0_V1N1_G02_R6_PARSER_AND_AUTHORING_DIAGNOSTICS_COEXIST=PASS'
echo 'VIEW0_V1N1_G02_R6_TWO_PASS_CAPACITY_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G02_R6_C0_FACT_LAYOUT_BYTES=184'
echo 'VIEW0_V1N1_G02_R6_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'PASS: G02 R6 logical-body singleton, parser-repair provenance, CLI contract, determinism and failure atomicity'
