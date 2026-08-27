#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
LEX='build/view0-v1/native/lexbor-src'
[[ -d "$LEX/.git" ]] || fail 'pinned Lexbor missing'
[[ "$(git -C "$LEX" rev-parse HEAD)" == 2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe ]] ||
  fail 'Lexbor commit drift'

find build/view0-v1/native -maxdepth 1 -type f \
  \( -name 'native.o' -o -name 'g06*.o' -o -name 'g06-*test' -o -name 'g06_*test' \) -delete
echo 'VIEW0_V1N1_G06_WAVE_DERIVED_RESET=PASS'

make -s \
  view0-v1n1-g06-c0-test \
  view0-v1n1-g06-wave-test \
  view0-v1n1-g06-wave-adversarial-test \
  view0-v1n1-g06-wave-global-failure-atomicity-test

make -s \
  view0-v1n1-g05-c0-catalog-test view0-v1n1-g05-c0-anchor-test \
  view0-v1n1-g05-c0-sr1-input-state-test \
  view0-v1n1-g05-r1a-test view0-v1n1-g05-r1a-adversarial-test \
  view0-v1n1-g05-r2a-test view0-v1n1-g05-r2a-adversarial-test \
  view0-v1n1-g05-r3a-test view0-v1n1-g05-r3a-adversarial-test \
  view0-v1n1-g05-r3a-matrix-test view0-v1n1-g05-r3a-global-failure-atomicity-test \
  view0-v1n1-g05-r4a-test view0-v1n1-g05-r4a-adversarial-test \
  view0-v1n1-g05-r4a-global-failure-atomicity-test

make -s \
  view0-v1n1-g04-r1a-test view0-v1n1-g04-r1a-adversarial-test \
  view0-v1n1-g04-r1a-global-failure-atomicity-test \
  view0-v1n1-g04-r1b-test view0-v1n1-g04-r1b-adversarial-test \
  view0-v1n1-g04-r1c-test view0-v1n1-g04-r1c-adversarial-test \
  view0-v1n1-g04-r2-test view0-v1n1-g04-r2-adversarial-test \
  view0-v1n1-g04-r2-global-failure-atomicity-test

echo '### G06 R1-R17 — GCC ANALYZER'
CC_BIN="${CC:-cc}"
CC_PATH=$(command -v "$CC_BIN") || fail 'compiler missing'
CC_VERSION=$($CC_BIN --version | head -1)
CC_TARGET=$($CC_BIN -dumpmachine)
printf 'VIEW0_V1N1_G06_WAVE_COMPILER_PATH=%s\n' "$CC_PATH"
printf 'VIEW0_V1N1_G06_WAVE_COMPILER_VERSION=%s\n' "$CC_VERSION"
printf 'VIEW0_V1N1_G06_WAVE_COMPILER_TARGET=%s\n' "$CC_TARGET"
common=(
  -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include
  -Itools/c/view0_conformance -isystem "$LEX/source"
  -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion
  -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes
  -Wformat=2 -Wundef
)
"$CC_BIN" "${common[@]}" -fanalyzer -fsyntax-only \
  tools/c/view0_conformance/g06_c0.c tools/c/view0_conformance/g06.c \
  tools/c/view0_conformance/native.c \
  tests/c/view0_v1n1_g06_wave_test.c \
  tests/c/view0_v1n1_g06_wave_adversarial_test.c \
  tests/c/view0_v1n1_g06_wave_global_failure_atomicity_test.c
echo 'VIEW0_V1N1_G06_WAVE_GCC_FANALYZER=PASS'

echo '### G06 R1-R17 — COMPILED STACK / RETAINED 900000-BYTE ADMISSION'
stack_dir=$(mktemp -d /tmp/arborcore-g06-wave-stack.XXXXXX)
trap 'find "$stack_dir" -type f -delete; find "$stack_dir" -depth -type d -empty -delete' RETURN
stack_common=("${common[@]}" -fPIC -fstack-usage)
for unit in native lexbor_adapter g06_c0 g06; do
  "$CC_BIN" "${stack_common[@]}" -c "tools/c/view0_conformance/$unit.c" \
    -o "$stack_dir/$unit.o"
done
sf(){ awk -F '\t' -v pattern="$2" '$1~pattern{print $2;exit}' "$1"; }
native_stack=$(sf "$stack_dir/native.su" 'arbor_view0_native_check$')
evaluate_stack=$(sf "$stack_dir/g06.su" ':evaluate$')
observe_stack=$(sf "$stack_dir/lexbor_adapter.su" 'arbor_view0_native_lexbor_observe$')
process_stack=$(sf "$stack_dir/lexbor_adapter.su" ':lexbor_process$')
document_stack=$(sf "$stack_dir/lexbor_adapter.su" ':observe_document$')
complete_stack=$(sf "$stack_dir/g06.su" ':element_complete$')
comma_stack=$(sf "$stack_dir/g06.su" ':valid_comma_value')
space_stack=$(sf "$stack_dir/g06_c0.su" 'arbor_view0_native_g06_c0_space_tokens$')
for value in "$native_stack" "$evaluate_stack" "$observe_stack" "$process_stack" \
  "$document_stack" "$complete_stack" "$comma_stack" "$space_stack"; do
  [[ -n "$value" ]] || fail 'compiled stack record missing'
done
consumer_stack=$comma_stack
((space_stack > consumer_stack)) && consumer_stack=$space_stack
phase_stack=$((native_stack + evaluate_stack + observe_stack + process_stack +
               document_stack + complete_stack + consumer_stack))
bound=900000
((phase_stack <= bound)) || fail "G06 phased stack $phase_stack > $bound"
printf 'VIEW0_V1N1_G06_WAVE_EVALUATE_STACK_BYTES=%s\n' "$evaluate_stack"
printf 'VIEW0_V1N1_G06_WAVE_UNIQUE_TOKEN_STACK_BYTES=%s\n' "$consumer_stack"
printf 'VIEW0_V1N1_G06_WAVE_PHASED_STACK_BYTES=%s\n' "$phase_stack"
printf 'VIEW0_V1N1_G06_WAVE_PHASED_STACK_BOUND_BYTES=%s\n' "$bound"
echo 'VIEW0_V1N1_G06_WAVE_STACK_THRESHOLD_WIDENING=NO'
find "$stack_dir" -type f -delete
find "$stack_dir" -depth -type d -empty -delete
trap - RETURN

[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] ||
  fail 'production VIEW API count'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' \
  tools/c/view0_conformance/g06.c || fail 'direct G06 heap allocation'
! grep -Eq '\b(setlocale|localeconv|strtod|strtol|strtoul)\s*\(' \
  tools/c/view0_conformance/g06.c tools/c/view0_conformance/g06_c0.c ||
  fail 'locale-dependent G06 conversion'
echo 'VIEW0_V1N1_G06_WAVE_SINGLE_PINNED_LEXBOR_PARSER=PASS'
echo 'VIEW0_V1N1_G06_WAVE_PRODUCTION_VIEW_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G06_WAVE_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO'
echo 'VIEW0_V1N1_G06_WAVE_LOCALE_DEPENDENCE=ZERO'

make -s view0-v1n0-tool
cli_file=$(mktemp /tmp/g06-wave-cli.XXXXXX.html)
cli_out=$(mktemp /tmp/g06-wave-cli.XXXXXX.out)
trap 'find "$cli_file" "$cli_out" -type f -delete' RETURN
printf '%s' '<!doctype html><title>x</title><body><input type=checkbox checked=false></body>' \
  > "$cli_file"
set +e
build/view0-v1/native/arborcore-view0-html-check --format=tsv "$cli_file" > "$cli_out" 2>&1
cli_rc=$?
set -e
[[ "$cli_rc" -eq 1 ]] || fail "CLI rc=$cli_rc"
awk -F '\t' '$3=="0x0000000030060001" && $6==58 && $7==7 && $8==1 && $9==59 {n++} END{exit n==1?0:1}' \
  "$cli_out" || fail 'CLI G06 R1 frozen anchor'
echo 'VIEW0_V1N1_G06_WAVE_CLI_NEGATIVE_ANCHOR=PASS_OFFSET_58_LENGTH_7_LINE_1_COLUMN_59'
find "$cli_file" "$cli_out" -type f -delete
trap - RETURN

sanitize_lexbor='build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a'
if command -v cmake >/dev/null 2>&1; then
  make -s view0-v1n0-lexbor-sanitize
  echo 'VIEW0_V1N1_G06_WAVE_SANITIZED_LEXBOR_PROVENANCE=FRESH_DETERMINISTIC_BUILD'
else
  [[ -f "$sanitize_lexbor" ]] || fail 'cmake unavailable and qualified sanitized Lexbor absent'
  [[ "$(sha256sum "$sanitize_lexbor" | awk '{print $1}')" == \
      8983bb6c246eb897257546fe72cb2414ca45704b7df99bfc5b1443ed0b0510aa ]] ||
    fail 'retained sanitized Lexbor identity drift'
  echo 'VIEW0_V1N1_G06_WAVE_SANITIZED_LEXBOR_PROVENANCE=RETAINED_EXACT_QUALIFIED_BUILD'
fi
SAN='build/view0-v1/native/g06-wave-sanitize-test'
"$CC_BIN" -Iinclude -D_POSIX_C_SOURCE=200809L -Itools/include \
  -Itools/c/view0_conformance -isystem "$LEX/source" \
  -std=c17 -O1 -g -Wall -Wextra -Wpedantic -Werror -Wconversion \
  -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes \
  -Wformat=2 -Wundef -fsanitize=address,undefined -fno-omit-frame-pointer \
  tests/c/view0_v1n1_g06_wave_adversarial_test.c \
  tools/c/view0_conformance/native.c tools/c/view0_conformance/lexbor_adapter.c \
  tools/c/view0_conformance/g03_provenance.c \
  tools/c/view0_conformance/g03_r1a.c tools/c/view0_conformance/g03_r2a.c \
  tools/c/view0_conformance/g03_r3a.c tools/c/view0_conformance/g03_r4a.c \
  tools/c/view0_conformance/g03_r5a.c tools/c/view0_conformance/g03_r7a.c \
  tools/c/view0_conformance/g04_r1a.c tools/c/view0_conformance/g04_r2a.c \
  tools/c/view0_conformance/g05_c0.c tools/c/view0_conformance/g05_r1a.c \
  tools/c/view0_conformance/g05_r2a.c tools/c/view0_conformance/g05_r3a.c \
  tools/c/view0_conformance/g05_r4a.c tools/c/view0_conformance/g06_c0.c \
  tools/c/view0_conformance/g06.c \
  build/libarborcore_view.a build/libarborcore_runtime.a build/libarborcore.a \
  "$sanitize_lexbor" \
  -no-pie -Wl,-z,relro,-z,now,-z,noexecstack \
  -Wl,--wrap=lxb_html_interface_create \
  -Wl,--wrap=lxb_html_tree_insert_foreign_element \
  -fsanitize=address,undefined -lm -o "$SAN"
sanitize_log=$(mktemp /tmp/g06-wave-sanitize.XXXXXX.log)
trap 'find "$sanitize_log" -type f -delete' RETURN
set +e
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  "$SAN" > "$sanitize_log" 2>&1
sanitize_rc=$?
set -e
if [[ "$sanitize_rc" -eq 0 ]]; then
  sed -n '1,240p' "$sanitize_log"
  echo 'VIEW0_V1N1_G06_WAVE_LEAK_SANITIZER=PASS'
elif grep -Fq 'LeakSanitizer has encountered a fatal error' "$sanitize_log" &&
     grep -Fq 'LeakSanitizer does not work under ptrace' "$sanitize_log"; then
  ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$SAN"
  echo 'VIEW0_V1N1_G06_WAVE_LEAK_SANITIZER=ENVIRONMENT_BLOCKED_RETAINED_LIVE_GATE_REQUIRED'
else
  sed -n '1,240p' "$sanitize_log" >&2
  fail "sanitizer rc=$sanitize_rc"
fi
find "$sanitize_log" -type f -delete
trap - RETURN
echo 'VIEW0_V1N1_G06_WAVE_SANITIZE=PASS'
echo 'PASS: G06 R1-R17 functional/adversarial/atomicity/analyzer/stack/CLI/sanitizer qualification'
