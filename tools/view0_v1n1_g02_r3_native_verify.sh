#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-src'
TOOL='build/view0-v1/native/arborcore-view0-html-check'

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$LEX" ]] || fail 'pinned Lexbor source tree missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'

make -s view0-v1n1-g02-r3-test view0-v1n1-g02-r3-adversarial-test view0-v1n0-tool

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
    tests/c/view0_v1n1_g02_r3_doctype_legacy_discouraged_adversarial_test.c

echo 'VIEW0_V1N1_G02_R3_GCC_FANALYZER=PASS'

tmp=$(mktemp -d /tmp/arborcore-view0-g02-r3.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
printf '%s' '<!DOCTYPE html><title>x</title>' > "$tmp/canonical.html"
printf '%s' '<!DOCTYPE html SYSTEM "about:legacy-compat"><title>x</title>' > "$tmp/legacy.html"
printf '%s' '<!DOCTYPE html SYSTEM "ABOUT:legacy-compat"><title>x</title>' > "$tmp/bad-legacy.html"
printf '%s' '<!DOCTYPE svg><title>x</title>' > "$tmp/bad-name.html"

"$TOOL" --format=tsv "$tmp/canonical.html" > "$tmp/canonical.tsv"
grep -Fq $'diagnostics=0\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/canonical.tsv"

set +e
"$TOOL" --format=tsv "$tmp/legacy.html" > "$tmp/legacy.tsv"
legacy_rc=$?
"$TOOL" --format=tsv "$tmp/bad-legacy.html" > "$tmp/bad-legacy.tsv"
bad_legacy_rc=$?
"$TOOL" --format=tsv "$tmp/bad-name.html" > "$tmp/bad-name.tsv"
bad_name_rc=$?
set -e
[[ "$legacy_rc" -eq 0 ]] || fail "G02 R3 warning-only CLI exit=$legacy_rc"
[[ "$bad_legacy_rc" -eq 1 ]] || fail "G02 R3 R2-syntax-error CLI exit=$bad_legacy_rc"
[[ "$bad_name_rc" -eq 1 ]] || fail "G02 R3 bad-name CLI exit=$bad_name_rc"

grep -Fq $'\twarning\t0x0000000030020003\tARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\tarborcore-authoring\t23\t19\t1\t24\t0\tLegacy DOCTYPE compatibility string should not be used unless required by a generator limitation' "$tmp/legacy.tsv"
grep -Fq $'diagnostics=1\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$tmp/legacy.tsv"
! grep -Fq $'0x0000000030020002\tARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX' "$tmp/legacy.tsv" || fail 'R2 syntax error appeared for admitted legacy form'
grep -Fq $'\terror\t0x0000000030020002\tARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX\tarborcore-authoring' "$tmp/bad-legacy.tsv"
! grep -Fq $'0x0000000030020003\tARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED' "$tmp/bad-legacy.tsv" || fail 'R3 warning appeared on R2-invalid legacy form'
! grep -Fq $'0x0000000030020003\tARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED' "$tmp/bad-name.tsv" || fail 'R3 warning appeared on invalid doctype name'

if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
    tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c; then
    fail 'G02 R3 introduced direct Arborcore heap allocation into checker/adapter'
fi
! grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED' tools/c/view0_conformance/lexbor_adapter.c || fail 'G02 R3 rule leaked into Lexbor adapter'

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' |
    LC_ALL=C sort)
count=$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)
[[ "$count" -eq 11 ]] || fail "G02 R3 unexpectedly changed production VIEW symbols: $count"

echo 'VIEW0_V1N1_G02_R3_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G02_R3_LEGACY_WARNING=PASS'
echo 'VIEW0_V1N1_G02_R3_WARNING_ONLY_CLI_EXIT_ZERO=PASS'
echo 'VIEW0_V1N1_G02_R3_R2_SYNTAX_PRECEDENCE=PASS'
echo 'VIEW0_V1N1_G02_R3_WARNING_PARSE_CLEAN=PASS'
echo 'VIEW0_V1N1_G02_R3_TWO_PASS_CAPACITY_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G02_R3_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'PASS: G02 R3 exact legacy warning, non-hard CLI policy, anchors, determinism and failure atomicity'
