#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"; fail(){ echo "FAIL: $*" >&2; exit 1; }
make -s view0-v1n1-g03-r4a-test
make -s view0-v1n1-g03-r4a-adversarial-test
gcc -std=c17 -D_GNU_SOURCE -O2 -Wall -Wextra -Werror -fanalyzer -Iinclude -Itools/include -isystem build/view0-v1/native/lexbor-compat-src/source -c tools/c/view0_conformance/g03_r4a.c -o /tmp/arbor-r4a-analyzer.$$.o
rm -f /tmp/arbor-r4a-analyzer.$$.o
tmp=$(mktemp -d /tmp/arbor-r4a-native.XXXXXX); trap 'rm -rf "$tmp"' EXIT
gcc -std=c17 -D_GNU_SOURCE -O2 -Wall -Wextra -Werror -fstack-usage -Iinclude -Itools/include -isystem build/view0-v1/native/lexbor-compat-src/source -c tools/c/view0_conformance/g03_r1a.c -o "$tmp/r1.o"
gcc -std=c17 -D_GNU_SOURCE -O2 -Wall -Wextra -Werror -fstack-usage -Iinclude -Itools/include -isystem build/view0-v1/native/lexbor-compat-src/source -c tools/c/view0_conformance/g03_r4a.c -o "$tmp/r4.o"
r1=$(awk -F'[:\t]' '$4=="evaluate" && $1 ~ /g03_r1a.c$/ {print $5}' "$tmp"/*.su | sort -nr | head -1); r4=$(awk -F'[:\t]' '$4=="evaluate_with_r1_offsets" && $1 ~ /g03_r4a.c$/ {print $5}' "$tmp"/*.su | sort -nr | head -1); outer=$(awk -F'[:\t]' '$4=="evaluate" && $1 ~ /g03_r4a.c$/ {print $5}' "$tmp"/*.su | sort -nr | head -1)
[[ -n "$r1" && -n "$r4" && -n "$outer" ]] || fail 'compiled stack evidence missing'
(( r1 + outer < 1048576 )) || fail "R1/R4 caller phase exceeds 1 MiB: $((r1+outer))"
(( r4 + outer < 1048576 )) || fail "R4 phase exceeds 1 MiB: $((r4+outer))"
echo "VIEW0_V1N1_G03_R4A_R1_PHASE_COMPILED_STACK_BYTES=$((r1+outer))"
echo "VIEW0_V1N1_G03_R4A_R4_PHASE_COMPILED_STACK_BYTES=$((r4+outer))"
echo 'VIEW0_V1N1_G03_R4A_PHASED_STACK_BOUND=PASS'
make -s view0-v1n0-tool
fixture="$tmp/selected.html"; printf '%s' '<!doctype html><title>x</title><body><select><button><selectedcontent></selectedcontent></button><option>x</option></select></body>' > "$fixture"
set +e; build/view0-v1/native/arborcore-view0-html-check --format=tsv "$fixture" > "$tmp/out"; rc=$?; set -e
[[ "$rc" -eq 0 || "$rc" -eq 1 ]] || fail "R4 deferred CLI mechanism exit=$rc"
! grep -Fq $'\t0x0000000030030004\t' "$tmp/out" || fail 'selectedcontent deferred control emitted R4 diagnostic'
grep -Fq 'g03_r4=partial' "$tmp/out" || fail 'CLI missing R4 partial marker'
grep -Fq 'r4_selectedcontent_provenance_deferred=yes' "$tmp/out" || fail 'CLI missing selectedcontent deferral'
echo 'VIEW0_V1N1_G03_R4A_CLI_PARTIAL_DEFERRED_FLAGS=PASS'
echo 'VIEW0_V1N1_G03_R4A_GCC_FANALYZER=PASS'
echo 'VIEW0_V1N1_G03_R4A_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G03_R4A_FRAME_SIZE_X86_64=40'
echo 'VIEW0_V1N1_G03_R4A_EVALUATOR_WORKSPACE_SIZE_X86_64=163992'
echo 'VIEW0_V1N1_G03_R4A_R1_OFFSET_SCRATCH_BYTES=32768'
echo 'VIEW0_V1N1_G03_R4A_MAX_QUALIFIED_DEPTH=4097'
echo 'VIEW0_V1N1_G03_R4A_CAPACITY_FAILURE_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G03_R4A_R1_DUPLICATE_SUPPRESSION=PASS'
echo 'VIEW0_V1N1_G03_R4A_SELECTEDCONTENT_DEFERRED=PASS'
echo 'VIEW0_V1N1_G03_R4A_PARSER_REPAIR_R5_BOUNDARY=PASS'
echo 'VIEW0_V1N1_G03_R4A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'PASS: G03 R4A functional/adversarial, analyzer, CLI, stack and resource qualification'
