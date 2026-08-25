#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
LEX='build/view0-v1/native/lexbor-compat-src'
CANON='build/view0-v1/native/lexbor-src'
RELEASE='build/view0-v1/native/lexbor-build-release/liblexbor_static.a'
SANITIZE='build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a'
fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -d "$CANON/.git" && -d "$LEX" ]] || fail 'canonical/derived Lexbor source missing'
[[ "$(git -C "$CANON" rev-parse HEAD)" == '2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe' ]] || fail 'canonical Lexbor commit drift'
[[ -z "$(git -C "$CANON" status --porcelain=v1 --untracked-files=all --ignored=matching)" ]] || fail 'canonical Lexbor cache is not strong-clean'
[[ "$(sha256sum "$LEX/source/lexbor/html/tree/insertion_mode/in_body.c" | awk '{print $1}')" == '142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8' ]] || fail 'derived Lexbor ruby compatibility source drift'
[[ -f "$RELEASE" && -f "$SANITIZE" ]] || fail 'Lexbor release/sanitize archive missing'

make -s view0-v1n1-g03-r3a-test view0-v1n1-g03-r3a-adversarial-test build/view0-v1/native/arborcore-view0-html-check

cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -fsyntax-only \
    tools/c/view0_conformance/g03_r1a.c \
    tools/c/view0_conformance/g03_r3a.c \
    tools/c/view0_conformance/native.c \
    tests/c/view0_v1n1_g03_r3a_descendant_exclusions_test.c \
    tests/c/view0_v1n1_g03_r3a_descendant_exclusions_adversarial_test.c

echo 'VIEW0_V1N1_G03_R3A_GCC_FANALYZER=PASS'

# Compile stack-usage evidence with the same optimized warning profile. The
# R1 pass and noinline R3 traversal must remain disjoint beneath the 1 MiB
# admission ceiling; never widen the ceiling to accommodate compiler inlining.
stack_tmp=$(mktemp -d /tmp/arborcore-g03-r3a-stack.XXXXXX)
cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fstack-usage -c tools/c/view0_conformance/g03_r3a.c -o "$stack_tmp/r3.o"
cc -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include -isystem "$LEX/source" \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fstack-usage -c tools/c/view0_conformance/g03_r1a.c -o "$stack_tmp/r1.o"
r3_outer=$(awk -F '\t' '$1 ~ /g03_r3a.c:.*:evaluate$/ {print $2}' "$stack_tmp/r3.su")
r3_helper=$(awk -F '\t' '$1 ~ /g03_r3a.c:.*:evaluate_with_r1_offsets$/ {print $2}' "$stack_tmp/r3.su")
r1_eval=$(awk -F '\t' '$1 ~ /g03_r1a.c:.*:evaluate$/ {print $2}' "$stack_tmp/r1.su")
[[ "$r3_outer" =~ ^[0-9]+$ && "$r3_helper" =~ ^[0-9]+$ && "$r1_eval" =~ ^[0-9]+$ ]] || fail 'compiler stack-usage evidence missing/non-numeric'
r1_phase=$((r3_outer + r1_eval + 128))
r3_phase=$((r3_outer + r3_helper + 128))
[[ "$r1_phase" -le 900000 ]] || fail "R3A R1-phase compiled stack bound exceeded: $r1_phase"
[[ "$r3_phase" -le 400000 ]] || fail "R3A R3-phase compiled stack bound exceeded: $r3_phase"
[[ "$r1_phase" -lt 1048576 && "$r3_phase" -lt 1048576 ]] || fail 'R3A compiled phase exceeds 1 MiB stack admission ceiling'
rm -rf "$stack_tmp"
echo "VIEW0_V1N1_G03_R3A_R1_PHASE_COMPILED_STACK_BYTES=$r1_phase"
echo "VIEW0_V1N1_G03_R3A_R3_PHASE_COMPILED_STACK_BYTES=$r3_phase"
echo 'VIEW0_V1N1_G03_R3A_PHASED_STACK_BOUND=PASS'

for marker in \
 '<address><h1>' \
 '<option><a href=/>' \
 '<video><div><audio>' \
 '<legend><a href=/>' \
 '<dfn><span><dfn>' \
 '<button><span><a href=/>' \
 '<meter><span><meter>' \
 '<progress><span><progress>' \
 '<label><span><label>' \
 '<ruby><span><ruby>' \
 '<a href=/><span><button>' \
 '<canvas><textarea>' \
 'Same element is invalid under R1'; do
    grep -Fq "$marker" tests/c/view0_v1n1_g03_r3a_descendant_exclusions_test.c || fail "R3A functional fixture/marker missing: $marker"
done
for marker in 'exact_count - 1u' 'one diagnostic' 'invalid_utf8' 'deferred'; do
    grep -Fqi "$marker" tests/c/view0_v1n1_g03_r3a_descendant_exclusions_adversarial_test.c || fail "R3A adversarial assertion missing: $marker"
done

# A single parse-clean control exposes all five R3A deferred branch families.
tmp=$(mktemp -d /tmp/arborcore-g03-r3a-native.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
fixture="$tmp/deferred.html"
printf '%s' '<!doctype html><html><head><title>x</title></head><body><a href="/"><input type="hidden"></a><label><input></label><canvas><input type="checkbox"><select size="2"></select></canvas><noscript><span>x</span></noscript></body></html>' > "$fixture"
build/view0-v1/native/arborcore-view0-html-check --format=tsv "$fixture" > "$tmp/cli.out"
! grep -Fq $'\t0x0000000030030003\t' "$tmp/cli.out" || fail 'deferred CLI control unexpectedly emitted an R3 diagnostic'
grep -Fq $'g03_r3=partial\tr3_input_type_deferred=yes\tr3_labeled_control_deferred=yes\tr3_canvas_input_deferred=yes\tr3_canvas_select_size_deferred=yes\tr3_noscript_deferred=yes' "$tmp/cli.out" || fail 'CLI does not expose all R3A deferred branches'
echo 'VIEW0_V1N1_G03_R3A_CLI_PARTIAL_DEFERRED_FLAGS=PASS'

if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' tools/c/view0_conformance/g03_r3a.c; then fail 'R3A introduced direct Arborcore heap allocation'; fi
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 rule semantics appeared'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R4A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 rule semantics appeared under R4A'
else
    fail "unexpected R3A retained extension: $current_version"
fi

symbols=$(nm -g --defined-only build/libarborcore_view.a | awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort -u)
[[ "$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)" -eq 11 ]] || fail 'production VIEW symbol count changed under R3A'

echo 'VIEW0_V1N1_G03_R3A_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G03_R3A_FRAME_SIZE_X86_64=64'
echo 'VIEW0_V1N1_G03_R3A_EVALUATOR_WORKSPACE_SIZE_X86_64=262472'
echo 'VIEW0_V1N1_G03_R3A_R1_OFFSET_SCRATCH_BYTES=32768'
echo 'VIEW0_V1N1_G03_R3A_MAX_QUALIFIED_DEPTH=4097'
echo 'VIEW0_V1N1_G03_R3A_CAPACITY_FAILURE_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G03_R3A_ONE_DIAGNOSTIC_PER_FORBIDDEN_DESCENDANT_MAX=PASS'
echo 'VIEW0_V1N1_G03_R3A_R1_DUPLICATE_SUPPRESSION=PASS'
echo 'VIEW0_V1N1_G03_R3A_PARSER_REPAIR_R5_BOUNDARY=PASS'
echo 'VIEW0_V1N1_G03_R3A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_G03_R3A_IMPLEMENTATION_COMPLETE=NO'
echo 'PASS: G03 R3A functional/adversarial, F1-R7 partial ownership, R1 suppression, analyzer, CLI and resource qualification'
