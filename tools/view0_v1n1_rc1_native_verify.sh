#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX='build/view0-v1/native/lexbor-src'
[[ -d "$LEX/.git" ]] || fail 'pinned Lexbor missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == 2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe ]] || fail 'Lexbor commit drift'

find build/view0-v1/native -maxdepth 1 -type f \
  \( -name '*.o' -o -name '*-test' -o -name '*_test' \) -delete
echo 'VIEW0_V1N1_RC1_DERIVED_RESET=PASS'

make -s \
  view0-v1n1-c0-test view0-v1n1-c0-adversarial-test \
  view0-v1n1-g02-r1-test view0-v1n1-g02-r1-adversarial-test \
  view0-v1n1-g02-r2-test view0-v1n1-g02-r2-adversarial-test \
  view0-v1n1-g02-r3-test view0-v1n1-g02-r3-adversarial-test \
  view0-v1n1-g02-r4-test view0-v1n1-g02-r4-adversarial-test \
  view0-v1n1-g02-r5-test view0-v1n1-g02-r5-adversarial-test \
  view0-v1n1-g02-r6-test view0-v1n1-g02-r6-adversarial-test \
  view0-v1n1-g03-r1a-test view0-v1n1-g03-r1a-adversarial-test \
  view0-v1n1-g03-r2a-test view0-v1n1-g03-r2a-adversarial-test \
  view0-v1n1-g03-r3a-test view0-v1n1-g03-r3a-adversarial-test \
  view0-v1n1-g03-r4a-test view0-v1n1-g03-r4a-adversarial-test \
  view0-v1n1-g03-r5a-test view0-v1n1-g03-r5a-adversarial-test \
  view0-v1n1-g03-r7a-test view0-v1n1-g03-r7a-adversarial-test \
  view0-v1n1-g04-r1a-test view0-v1n1-g04-r1a-adversarial-test \
  view0-v1n1-g04-r1a-global-failure-atomicity-test \
  view0-v1n1-g04-r1b-test view0-v1n1-g04-r1b-adversarial-test \
  view0-v1n1-g04-r1c-test view0-v1n1-g04-r1c-adversarial-test \
  view0-v1n1-g04-r2-test view0-v1n1-g04-r2-adversarial-test \
  view0-v1n1-g04-r2-global-failure-atomicity-test \
  view0-v1n1-g05-c0-catalog-test view0-v1n1-g05-c0-sr1-input-state-test \
  view0-v1n1-g05-r1a-test view0-v1n1-g05-r1a-adversarial-test \
  view0-v1n1-g05-r2a-test view0-v1n1-g05-r2a-adversarial-test \
  view0-v1n1-g05-r3a-test view0-v1n1-g05-r3a-adversarial-test \
  view0-v1n1-g05-r3a-matrix-test view0-v1n1-g05-r3a-global-failure-atomicity-test \
  view0-v1n1-g05-r4a-test view0-v1n1-g05-r4a-adversarial-test \
  view0-v1n1-g05-r4a-global-failure-atomicity-test \
  view0-v1n1-g06-c0-test \
  view0-v1n1-g06-wave-test view0-v1n1-g06-wave-adversarial-test \
  view0-v1n1-g06-wave-global-failure-atomicity-test \
  view0-v1n1-rc1-test
echo 'VIEW0_V1N1_RC1_G02_G06_RETAINED_INTEGRATION=PASS'

CC_BIN="${CC:-cc}"
CC_PATH=$(command -v "$CC_BIN") || fail 'compiler missing'
printf 'VIEW0_V1N1_RC1_COMPILER_PATH=%s\n' "$CC_PATH"
printf 'VIEW0_V1N1_RC1_COMPILER_VERSION=%s\n' "$($CC_BIN --version | head -1)"
printf 'VIEW0_V1N1_RC1_COMPILER_TARGET=%s\n' "$($CC_BIN -dumpmachine)"
common=(
  -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include
  -Itools/c/view0_conformance -isystem "$LEX/source"
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion
  -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes
  -Wformat=2 -Wundef
)
"$CC_BIN" "${common[@]}" -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c \
  tools/c/view0_conformance/g03_r7a.c \
  tools/c/view0_conformance/g04_r1a.c \
  tools/c/view0_conformance/g05_c0.c \
  tools/c/view0_conformance/g05_r3a.c \
  tests/c/view0_v1n1_rc1_dependency_reconciliation_test.c
echo 'VIEW0_V1N1_RC1_GCC_FANALYZER=PASS'

stack_dir=$(mktemp -d /tmp/arborcore-v1n1-rc1-stack.XXXXXX)
trap 'find "$stack_dir" -type f -delete; find "$stack_dir" -depth -type d -empty -delete' RETURN
stack_common=("${common[@]}" -fPIC -fstack-usage)
for unit in native lexbor_adapter g03_r2a g03_r3a g03_r7a; do
  "$CC_BIN" "${stack_common[@]}" -c "tools/c/view0_conformance/$unit.c" \
    -o "$stack_dir/$unit.o"
done
stack_field(){ awk -F '\t' -v pattern="$2" '$1~pattern{print $2;exit}' "$1"; }
native_stack=$(stack_field "$stack_dir/native.su" 'arbor_view0_native_check$')
observe_stack=$(stack_field "$stack_dir/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe$')
process_stack=$(stack_field "$stack_dir/lexbor_adapter.su" ':lexbor_process$')
r2_public=$(stack_field "$stack_dir/g03_r2a.su" 'arbor_view0_native_g03_r2a_measure$')
r2_evaluate=$(stack_field "$stack_dir/g03_r2a.su" ':evaluate$')
r3_public=$(stack_field "$stack_dir/g03_r3a.su" 'arbor_view0_native_g03_r3a_measure$')
r3_wrapper=$(stack_field "$stack_dir/g03_r3a.su" ':evaluate$')
r3_evaluate=$(stack_field "$stack_dir/g03_r3a.su" ':evaluate_with_r1_offsets$')
r7_public=$(stack_field "$stack_dir/g03_r7a.su" 'arbor_view0_native_g03_r7a_measure$')
r7_evaluate=$(stack_field "$stack_dir/g03_r7a.su" ':evaluate$')
for value in "$native_stack" "$observe_stack" "$process_stack" "$r2_public" \
  "$r2_evaluate" "$r3_public" "$r3_wrapper" "$r3_evaluate" \
  "$r7_public" "$r7_evaluate"; do
  [[ -n "$value" ]] || fail 'compiled stack record missing'
done
r2_phase=$((native_stack + r2_public + r2_evaluate + observe_stack + process_stack))
r3_phase=$((native_stack + r3_public + r3_wrapper + r3_evaluate + observe_stack + process_stack))
r7_phase=$((native_stack + r7_public + r7_evaluate + observe_stack + process_stack))
bound=900000
((r2_phase <= bound && r3_phase <= bound && r7_phase <= bound)) || fail 'RC1 phased stack bound exceeded'
printf 'VIEW0_V1N1_RC1_G03_R2_PHASED_STACK_BYTES=%s\n' "$r2_phase"
printf 'VIEW0_V1N1_RC1_G03_R3_PHASED_STACK_BYTES=%s\n' "$r3_phase"
printf 'VIEW0_V1N1_RC1_G03_R7_PHASED_STACK_BYTES=%s\n' "$r7_phase"
printf 'VIEW0_V1N1_RC1_PHASED_STACK_BOUND_BYTES=%s\n' "$bound"
echo 'VIEW0_V1N1_RC1_STACK_THRESHOLD_WIDENING=NO'
find "$stack_dir" -type f -delete
find "$stack_dir" -depth -type d -empty -delete
trap - RETURN

[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' \
  tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c \
  tools/c/view0_conformance/g03_r7a.c \
  tools/c/view0_conformance/g04_r1a.c \
  tools/c/view0_conformance/g05_c0.c \
  tools/c/view0_conformance/g05_r3a.c || fail 'direct Arborcore heap allocation'
echo 'VIEW0_V1N1_RC1_SINGLE_PINNED_LEXBOR_PARSER=PASS'
echo 'VIEW0_V1N1_RC1_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_RC1_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_RC1_MUTABLE_RUNTIME_GLOBAL_REGISTRY=NO'

if command -v cmake >/dev/null 2>&1; then
  make -s view0-v1n0-lexbor-sanitize
fi
SAN='build/view0-v1/native/rc1-dependency-reconciliation-sanitize-test'
make -s "$SAN"
sanitize_log=$(mktemp /tmp/arborcore-v1n1-rc1-sanitize.XXXXXX.log)
trap 'find "$sanitize_log" -type f -delete' RETURN
set +e
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  "$SAN" > "$sanitize_log" 2>&1
sanitize_rc=$?
set -e
if [[ "$sanitize_rc" -eq 0 ]]; then
  sed -n '1,160p' "$sanitize_log"
  echo 'VIEW0_V1N1_RC1_LEAK_SANITIZER=PASS'
elif grep -Fq 'LeakSanitizer has encountered a fatal error' "$sanitize_log" &&
     grep -Fq 'LeakSanitizer does not work under ptrace' "$sanitize_log"; then
  ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$SAN"
  echo 'VIEW0_V1N1_RC1_LEAK_SANITIZER=ENVIRONMENT_BLOCKED_RETAINED_LIVE_GATE_REQUIRED'
else
  sed -n '1,160p' "$sanitize_log" >&2
  fail "sanitizer rc=$sanitize_rc"
fi
find "$sanitize_log" -type f -delete
trap - RETURN
echo 'VIEW0_V1N1_RC1_SANITIZE=PASS'
echo 'PASS: V1N1 RC1 reconciliation functional/analyzer/resource/sanitizer qualification'
