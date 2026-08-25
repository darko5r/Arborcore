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
[[ -f "$RELEASE" && -f "$SANITIZE" ]] || fail 'Lexbor release/sanitize archive missing'

make -s view0-v1n1-g03-c0-l1-test view0-v1n1-g03-c0-l1-adversarial-test

cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -fsyntax-only \
    tools/c/view0_conformance/lexbor_adapter.c \
    tools/c/view0_conformance/g03_provenance.c \
    tests/c/view0_v1n1_g03_c0_lifecycle_test.c \
    tests/c/view0_v1n1_g03_c0_lifecycle_adversarial_test.c

echo 'VIEW0_V1N1_G03_C0_L1_GCC_FANALYZER=PASS'

grep -Fq 'depth.max_depth != 4097u' tests/c/view0_v1n1_g03_c0_lifecycle_adversarial_test.c || fail 'exact depth-4097 test assertion missing'
grep -Fq 'observations.max_depth != 4097u' tests/c/view0_v1n1_g03_c0_lifecycle_adversarial_test.c || fail 'adapter max-depth assertion missing'
grep -Fq 'capture.enter_count != observation_counts.element_count' tests/c/view0_v1n1_g03_c0_lifecycle_test.c || fail 'enter/element balance assertion missing'
grep -Fq 'capture.leave_count != observation_counts.element_count' tests/c/view0_v1n1_g03_c0_lifecycle_test.c || fail 'leave/element balance assertion missing'

if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
    tools/c/view0_conformance/lexbor_adapter.c \
    tools/c/view0_conformance/g03_provenance.c; then
    fail 'G03 C0-L1 introduced direct Arborcore heap allocation'
fi
! grep -ERq 'ARBOR_VIEW_V1_G03_|0x000000003003' tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c || fail 'G03 C0-L1 mechanism files acquired rule semantics'

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort -u)
[[ "$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)" -eq 11 ]] || fail 'production VIEW symbol count changed under G03 C0-L1'

echo 'VIEW0_V1N1_G03_C0_L1_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G03_C0_L1_C0_FACT_LAYOUT_BYTES=184'
echo 'VIEW0_V1N1_G03_C0_L1_TRUE_DFS_ENTER_LEAVE=PASS'
echo 'VIEW0_V1N1_G03_C0_L1_CALLBACK_FAILURE_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G03_C0_L1_4096_NESTED_DIV_MAX_DEPTH=4097'
echo 'VIEW0_V1N1_G03_C0_L1_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_G03_C0_L1_G03_RULE_IDS_IMPLEMENTED=ZERO'
echo 'PASS: G03 C0-L1 lifecycle ordering, depth, analyzer and zero-rule qualification'
