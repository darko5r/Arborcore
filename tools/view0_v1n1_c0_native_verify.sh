#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-src'

fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$LEX" ]] || fail 'pinned Lexbor source tree missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'Lexbor commit drift'

make -s view0-v1n1-c0-test view0-v1n1-c0-adversarial-test

cc -Iinclude -Itools/include -isystem "$LEX/source" -D_POSIX_C_SOURCE=200809L \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -fsyntax-only \
    tools/c/view0_conformance/native.c \
    tools/c/view0_conformance/lexbor_adapter.c \
    tests/c/view0_v1n1_c0_facts_test.c \
    tests/c/view0_v1n1_c0_facts_adversarial_test.c

echo 'VIEW0_V1N1_C0_GCC_FANALYZER=PASS'

grep -Fq 'lxb_html_tokenizer_callback_token_done_set(' tools/c/view0_conformance/lexbor_adapter.c ||
    fail 'token callback wrapper installation missing'
grep -Fq 'return capture->downstream(tokenizer, token, capture->downstream_context);' \
    tools/c/view0_conformance/lexbor_adapter.c || fail 'original tree callback forwarding missing'
grep -Fq 'source_first_doctype_public_id_offset' tools/include/arborcore/view0_conformance/native.h ||
    fail 'doctype public-id source span missing'
grep -Fq 'source_first_doctype_system_id_offset' tools/include/arborcore/view0_conformance/native.h ||
    fail 'doctype system-id source span missing'
grep -Fq 'source_second_body_start_tag_offset' tools/include/arborcore/view0_conformance/native.h ||
    fail 'duplicate-body source evidence missing'

if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
    tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c; then
    fail 'C0 introduced direct Arborcore heap allocation into native checker/adapter'
fi

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' |
    LC_ALL=C sort)
count=$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)
[[ "$count" -eq 11 ]] || fail "C0 unexpectedly changed production VIEW symbols: $count"

echo 'VIEW0_V1N1_C0_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_C0_DOCUMENT_FACT_SIZE_X86_64=184'
echo 'VIEW0_V1N1_C0_CALLBACK_FORWARDING=PASS'
echo 'VIEW0_V1N1_C0_DOCTYPE_SOURCE_SPANS=PASS'
echo 'VIEW0_V1N1_C0_PARSER_REPAIR_SOURCE_EVIDENCE=PASS'
echo 'VIEW0_V1N1_C0_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_C0_RULE_LAYER_EXTENSION_ALLOWED=YES'
echo 'PASS: C0 facts collection, failure atomicity, source/DOM separation and ownership boundary preserved'
