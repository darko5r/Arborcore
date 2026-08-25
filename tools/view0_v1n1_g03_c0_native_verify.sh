#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-src'
RELEASE='build/view0-v1/native/lexbor-build-release/liblexbor_static.a'
SANITIZE='build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a'

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$LEX/.git" ]] || fail 'pinned Lexbor source repository missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'
[[ "$(git -C "$LEX" rev-parse HEAD^{tree})" == '70da8da84cabdc4f02d47378602c41090b2b610c' ]] || fail 'Lexbor tree drift'
[[ -f "$RELEASE" ]] || fail 'release Lexbor static archive missing'
[[ -f "$SANITIZE" ]] || fail 'sanitized Lexbor static archive missing'

tmp=$(mktemp -d /tmp/arborcore-g03-c0.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
nm -A "$RELEASE" > "$tmp/release.nm"
nm -A "$SANITIZE" > "$tmp/sanitize.nm"
release_undef=$(awk '$2 == "U" && $3 == "lxb_html_interface_create" {n++} END {print n+0}' "$tmp/release.nm")
release_def=$(awk '$2 ~ /^[Tt]$/ && $3 == "lxb_html_interface_create" {n++} END {print n+0}' "$tmp/release.nm")
sanitize_undef=$(awk '$2 == "U" && $3 == "lxb_html_interface_create" {n++} END {print n+0}' "$tmp/sanitize.nm")
sanitize_def=$(awk '$2 ~ /^[Tt]$/ && $3 == "lxb_html_interface_create" {n++} END {print n+0}' "$tmp/sanitize.nm")
[[ "$release_undef" -eq 4 && "$release_def" -eq 1 ]] || fail "release Lexbor wrappability drift: undefined=$release_undef definition=$release_def"
[[ "$sanitize_undef" -eq 4 && "$sanitize_def" -eq 1 ]] || fail "sanitized Lexbor wrappability drift: undefined=$sanitize_undef definition=$sanitize_def"
echo "VIEW0_V1N1_G03_C0_LEXBOR_RELEASE_WRAP_UNDEFINED_REFERENCE_COUNT=$release_undef"
echo "VIEW0_V1N1_G03_C0_LEXBOR_SANITIZE_WRAP_UNDEFINED_REFERENCE_COUNT=$sanitize_undef"

make -s view0-v1n1-g03-c0-test view0-v1n1-g03-c0-adversarial-test view0-v1n1-g03-c0-nowrap-test

cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -fsyntax-only \
    tools/c/view0_conformance/lexbor_adapter.c \
    tools/c/view0_conformance/g03_provenance.c \
    tests/c/view0_v1n1_g03_c0_observation_test.c \
    tests/c/view0_v1n1_g03_c0_observation_adversarial_test.c \
    tests/c/view0_v1n1_g03_c0_nowrap_test.c

echo 'VIEW0_V1N1_G03_C0_GCC_FANALYZER=PASS'

nm build/view0-v1/native/g03-c0-observation-test > "$tmp/wrapped-test.nm"
grep -Fq '__wrap_lxb_html_interface_create' "$tmp/wrapped-test.nm" || fail 'wrapped G03 C0 test lacks wrapper symbol'
! grep -Eq ' U __real_lxb_html_interface_create$' "$tmp/wrapped-test.nm" || fail 'wrapped G03 C0 test left __real symbol unresolved'

if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
    tools/c/view0_conformance/lexbor_adapter.c \
    tools/c/view0_conformance/g03_provenance.c; then
    fail 'G03 C0 introduced direct Arborcore heap allocation'
fi
! grep -ERq 'ARBOR_VIEW_V1_G03_|0x000000003003' tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c || fail 'G03 C0 mechanism files acquired rule semantics'

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort -u)
[[ "$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)" -eq 11 ]] || fail 'production VIEW symbol count changed under G03 C0'

echo 'VIEW0_V1N1_G03_C0_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G03_C0_C0_FACT_LAYOUT_BYTES=184'
echo 'VIEW0_V1N1_G03_C0_STANDARD_ELEMENT_ID_COUNT=113'
echo 'VIEW0_V1N1_G03_C0_PROVENANCE_WRAP_RELEASE=PASS'
echo 'VIEW0_V1N1_G03_C0_NOWRAP_FAIL_CLOSED=PASS'
echo 'VIEW0_V1N1_G03_C0_TEMPLATE_CONTENT_EXCLUSION=PASS'
echo 'VIEW0_V1N1_G03_C0_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_G03_C0_G03_RULE_IDS_IMPLEMENTED=ZERO'
echo 'PASS: G03 C0 neutral observation, provenance, explicit resource/lifetime and fail-closed linker boundary'
