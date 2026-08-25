#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

TOOL='build/view0-v1/native/arborcore-view0-html-check'
CANONICAL_SHA='b24838f999a81a114b1c47c438b7567ebedebfabb3554c0c6fd01ae0940be564'
MAX_INPUT='1048576'
MAX_DIAGNOSTICS='4096'
RLIMIT_KIB='262144'
TIMEOUT_SECONDS='10'

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

command -v git >/dev/null 2>&1 || fail 'required tool missing: git'
BASE='08b55954139a2e8daaf254f4935c8f7a31e3aa19'
required_paths=$(cat <<'__VIEW0_V1N0_PATHS__'
Makefile
docs/VIEW_CORE_VIEW0.md
include/arborcore/view.h
src/c/view.c
tests/asm/view0_c4_abi_test.asm
tests/c/view0_c1_adversarial_test.c
tests/c/view0_c1_test.c
tests/c/view0_c2_adversarial_test.c
tests/c/view0_c2_test.c
tests/c/view0_c3_adversarial_test.c
tests/c/view0_c3_test.c
tests/c/view0_c4_adversarial_test.c
tests/c/view0_c4_test.c
tests/c/view0_m1_adversarial_test.c
tests/c/view0_m1_integration_test.c
tests/c/view0_m1_utf8_test.c
tests/c/view0_t1_adversarial_test.c
tests/c/view0_t1_test.c
tests/c/view0_v1_native_foundation_adversarial_test.c
tests/c/view0_v1_native_foundation_test.c
tests/c/view0_v1_render_artifacts.c
tests/view0/v1/invalid/no-doctype.html
tests/view0/v1/invalid/ul-text.html
tests/view0/v1/native/parse-error-tokenizer.html
tests/view0/v1/native/parse-error-tree.html
tools/c/view0_conformance/lexbor_adapter.c
tools/c/view0_conformance/main.c
tools/c/view0_conformance/native.c
tools/include/arborcore/view0_conformance/native.h
tools/view0_c1_baseline_verify.sh
tools/view0_c1_contract_verify.sh
tools/view0_c1_gate.sh
tools/view0_c1_native_verify.sh
tools/view0_c1_scope_verify.sh
tools/view0_c2_baseline_verify.sh
tools/view0_c2_contract_verify.sh
tools/view0_c2_gate.sh
tools/view0_c2_native_verify.sh
tools/view0_c2_scope_verify.sh
tools/view0_c3_baseline_verify.sh
tools/view0_c3_contract_verify.sh
tools/view0_c3_gate.sh
tools/view0_c3_native_verify.sh
tools/view0_c3_scope_verify.sh
tools/view0_c4_abi_verify.sh
tools/view0_c4_baseline_verify.sh
tools/view0_c4_contract_verify.sh
tools/view0_c4_gate.sh
tools/view0_c4_scope_verify.sh
tools/view0_m1_baseline_verify.sh
tools/view0_m1_contract_verify.sh
tools/view0_m1_gate.sh
tools/view0_m1_native_verify.sh
tools/view0_m1_scope_verify.sh
tools/view0_t1_baseline_verify.sh
tools/view0_t1_contract_verify.sh
tools/view0_t1_gate.sh
tools/view0_t1_native_verify.sh
tools/view0_t1_scope_verify.sh
tools/view0_v1_baseline_verify.sh
tools/view0_v1_checker_acquire.sh
tools/view0_v1_conformance_verify.sh
tools/view0_v1_contract_verify.sh
tools/view0_v1_gate.sh
tools/view0_v1_lexbor_acquire.sh
tools/view0_v1_lexbor_build.sh
tools/view0_v1_native_foundation_gate.sh
tools/view0_v1_native_foundation_verify.sh
tools/view0_v1_native_verify.sh
tools/view0_v1_scope_verify.sh
view/arborcore-view-core-1.contract
__VIEW0_V1N0_PATHS__
)
actual_paths=$({ git diff --name-only "$BASE"; git ls-files --others --exclude-standard; } | LC_ALL=C sort -u)
while IFS= read -r path; do
    [[ -n "$path" ]] || continue
    grep -Fxq "$path" <<< "$actual_paths" || {
        echo "FAIL: retained V1N0 required path missing under cumulative extension: $path" >&2
        exit 1
    }
done <<< "$required_paths"
required_count=$(printf '%s\n' "$required_paths" | sed '/^$/d' | wc -l)
cumulative_count=$(printf '%s\n' "$actual_paths" | sed '/^$/d' | wc -l)
[[ "$required_count" -eq 71 ]] || fail "internal V1N0 required path count drift: $required_count"
[[ "$cumulative_count" -ge "$required_count" ]] || fail "V1N0 cumulative path count below retained floor"
echo "VIEW0_V1N0_REQUIRED_PATH_COUNT=$required_count"
echo "VIEW0_V1N0_CUMULATIVE_PATH_COUNT=$cumulative_count"
echo 'VIEW0_V1N0_EXTENSION_AWARE_SCOPE=YES'
echo 'PASS: retained V1N0 cumulative source scope established under native extension'

for tool in make cc grep sha256sum awk sed timeout head tr stat nm mktemp wc git cp find sort; do
    command -v "$tool" >/dev/null 2>&1 || fail "required tool missing: $tool"
done

bash tools/view0_v1_lexbor_acquire.sh > build/view0-v1/native/source-reverify.log
grep -Fq 'VIEW0_V1_LEXBOR_SOURCE_TREE_EXTRA_POLICY=REJECT_MODIFIED_UNTRACKED_AND_IGNORED'     build/view0-v1/native/source-reverify.log
grep -Fq 'VIEW0_V1_LEXBOR_CANONICAL_SOURCE_MANIFEST_SHA256=a38edb39fe84f7fff90ff6206e6114aa3edab3c75ff363abaa11ee200d23e20d'     build/view0-v1/native/source-reverify.log
echo 'VIEW0_V1N0_EXACT_SOURCE_REVERIFICATION=PASS'

make -s view0-v1n0-tool view0-v1n0-test view0-v1n0-adversarial-test

LEX='build/view0-v1/native/lexbor-src'
LEX_COMPAT='build/view0-v1/native/lexbor-compat-src'
LEX_COMPAT_MANIFEST='build/view0-v1/native/lexbor-compat-source-manifest.sha256'
[[ -d "$LEX_COMPAT" && -f "$LEX_COMPAT_MANIFEST" ]] || fail 'derived Lexbor compatibility source evidence missing'
[[ "$(sha256sum "$LEX/source/lexbor/html/tree/insertion_mode/in_body.c" | awk '{print $1}')" == '28b8b1d15329f5f387005982a9a2788a16f66696505f740249f548e400be22ef' ]] || fail 'canonical Lexbor in_body.c changed'
[[ "$(sha256sum "$LEX_COMPAT/source/lexbor/html/tree/insertion_mode/in_body.c" | awk '{print $1}')" == '142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8' ]] || fail 'derived Lexbor ruby compatibility in_body.c mismatch'
[[ "$(sha256sum "$LEX_COMPAT_MANIFEST" | awk '{print $1}')" == 'e5e126ad79684b69f42a81a356c268d7cce978d0a0b3948214550683007a15e5' ]] || fail 'derived Lexbor compatibility manifest mismatch'
[[ "$(wc -l < "$LEX_COMPAT_MANIFEST" | tr -d ' ')" -eq 1055 ]] || fail 'derived Lexbor compatibility source file count mismatch'
[[ -z "$(git -C "$LEX" status --porcelain=v1 --untracked-files=all --ignored=matching)" ]] || fail 'canonical Lexbor source cache changed during compatibility preparation'
echo 'VIEW0_V1N0_LX1_CANONICAL_LEXBOR_SOURCE_MUTATED=NO'
echo 'VIEW0_V1N0_LX1_DERIVED_COMPATIBILITY_SOURCE=PASS'

ruby_valid='build/view0-v1/native/ruby-valid-control.html'
ruby_invalid='build/view0-v1/native/ruby-invalid-rt-control.html'
printf '%s\n' '<!doctype html><title>x</title><ruby>a<rt>b</rt></ruby>' > "$ruby_valid"
printf '%s\n' '<!doctype html><title>x</title><rt>b</rt>' > "$ruby_invalid"
set +e
"$TOOL" --format=tsv "$ruby_valid" > build/view0-v1/native/ruby-valid-control.tsv
rc_ruby_valid=$?
"$TOOL" --format=tsv "$ruby_invalid" > build/view0-v1/native/ruby-invalid-rt-control.tsv
rc_ruby_invalid=$?
set -e
[[ "$rc_ruby_valid" -eq 0 ]] || fail "valid ruby compatibility control exit=$rc_ruby_valid expected=0"
grep -Eq $'SUMMARY\t.*\tdiagnostics=0\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' build/view0-v1/native/ruby-valid-control.tsv || fail 'valid ruby compatibility control is not parse/authoring clean'
[[ "$rc_ruby_invalid" -eq 1 ]] || fail "invalid rt compatibility control exit=$rc_ruby_invalid expected=1"
grep -Eq $'SUMMARY\t.*\ttokenizer=0\ttree=1\tparse_clean=no\tcomplete_conformance=no' build/view0-v1/native/ruby-invalid-rt-control.tsv || fail 'invalid rt compatibility control did not preserve one tree parse error'
echo 'VIEW0_V1N0_LX1_VALID_RUBY_PARSE_CLEAN=PASS'
echo 'VIEW0_V1N0_LX1_INVALID_RT_PARSE_ERROR_PRESERVED=PASS'

make -s view0-v1-render-artifacts
for doc in \
    build/view0-v1/documents/template.html \
    build/view0-v1/documents/native-c.html \
    build/view0-v1/documents/nasm.html; do
    [[ -s "$doc" ]] || fail "canonical VIEW document missing: $doc"
    [[ "$(sha256sum "$doc" | awk '{print $1}')" == "$CANONICAL_SHA" ]] ||
        fail "canonical VIEW document identity changed: $doc"
    "$TOOL" --format=tsv "$doc" > "$doc.v1n0.tsv"
    grep -Fq $'SUMMARY\t' "$doc.v1n0.tsv"
    grep -Fq $'diagnostics=0\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' "$doc.v1n0.tsv"
done

echo 'VIEW0_V1N0_CANONICAL_TEMPLATE_PARSE_CLEAN=PASS'
echo 'VIEW0_V1N0_CANONICAL_NATIVE_C_PARSE_CLEAN=PASS'
echo 'VIEW0_V1N0_CANONICAL_NASM_PARSE_CLEAN=PASS'

set +e
LC_ALL=C "$TOOL" --format=tsv tests/view0/v1/native/parse-error-tokenizer.html \
    > build/view0-v1/native/tokenizer-control.tsv 2>&1
rc_tokenizer=$?
LC_ALL=C "$TOOL" --format=tsv tests/view0/v1/native/parse-error-tree.html \
    > build/view0-v1/native/tree-control.tsv 2>&1
rc_tree=$?
set -e
[[ "$rc_tokenizer" -eq 1 ]] || fail "tokenizer negative control exit=$rc_tokenizer"
[[ "$rc_tree" -eq 1 ]] || fail "tree negative control exit=$rc_tree"
grep -Fq 'html.parse.tokenizer.unexpected-question-mark-instead-of-tag-name' build/view0-v1/native/tokenizer-control.tsv
tokenizer_expected=$(awk '
    BEGIN { offset = 0 }
    {
        column = index($0, "?")
        if (column != 0) {
            printf "%d\t1\t%d\t%d", offset + column - 1, NR, column
            exit
        }
        offset += length($0) + 1
    }
' tests/view0/v1/native/parse-error-tokenizer.html)
[[ -n "$tokenizer_expected" ]] || fail 'unable to derive tokenizer control source location from exact fixture'
grep -Fq $'\t'"$tokenizer_expected"$'\t' build/view0-v1/native/tokenizer-control.tsv ||
    fail "tokenizer control location mismatch: expected $tokenizer_expected"

grep -Fq 'html.parse.tree.doctype-token-in-body-mode' build/view0-v1/native/tree-control.tsv
tree_expected=$(awk '
    BEGIN { offset = 0 }
    {
        lexical = index($0, "<!doctype html>")
        if (NR > 1 && lexical != 0) {
            column = lexical + 2
            printf "%d\t7\t%d\t%d", offset + column - 1, NR, column
            exit
        }
        offset += length($0) + 1
    }
' tests/view0/v1/native/parse-error-tree.html)
[[ -n "$tree_expected" ]] || fail 'unable to derive tree control source range from exact fixture'
grep -Fq $'\t'"$tree_expected"$'\t' build/view0-v1/native/tree-control.tsv ||
    fail "tree control location mismatch: expected $tree_expected"

echo "VIEW0_V1N0_TOKENIZER_CONTROL_EXPECTED=$tokenizer_expected"
echo "VIEW0_V1N0_TREE_CONTROL_EXPECTED=$tree_expected"
echo 'VIEW0_V1N0_LOCATION_EXPECTATION_SOURCE=DERIVED_FROM_EXACT_FIXTURES'
echo 'VIEW0_V1N0_TOKENIZER_NEGATIVE_CONTROL=REJECT_WITH_EXACT_LOCATION'
echo 'VIEW0_V1N0_TREE_NEGATIVE_CONTROL=REJECT_WITH_EXACT_LOCATION'

tmp=$(mktemp -d /tmp/arborcore-view0-v1n0.XXXXXX)
trap 'rm -rf "$tmp"' EXIT

LEX='build/view0-v1/native/lexbor-src'
[[ -f "$LEX/LICENSE" && -f "$LEX/NOTICE" ]] ||
    fail 'pinned Lexbor source must retain upstream LICENSE and NOTICE'
echo 'VIEW0_V1N0_LEXBOR_LICENSE_NOTICE_TRACEABILITY=PASS'

# The same acquire-script source-worktree validator used for the live cache must
# reject an otherwise-exact override containing an ignored extra file.
override="$tmp/lexbor-ignored-extra"
git clone -q --no-hardlinks "$LEX" "$override"
printf '%s\n' 'ignored-extra' >> "$override/.git/info/exclude"
printf '%s\n' 'unexpected ignored source-tree bytes' > "$override/ignored-extra"

test_root="$tmp/acquire-root"
mkdir -p "$test_root"
git init -q "$test_root"
printf '%s\n' 'build/' > "$test_root/.gitignore"

set +e
ARBORCORE_ROOT="$test_root" \
ARBORCORE_VIEW0_V1_LEXBOR_SOURCE="$override" \
    bash "$ROOT/tools/view0_v1_lexbor_acquire.sh" \
    > "$tmp/ignored-extra.out" 2>&1
rc_ignored_extra=$?
set -e
[[ "$rc_ignored_extra" -eq 1 ]] ||
    fail "ignored-extra Lexbor override must be rejected, got exit $rc_ignored_extra"
grep -Fq 'modified, untracked, or ignored source-tree files' "$tmp/ignored-extra.out"
echo 'VIEW0_V1N0_LEXBOR_IGNORED_EXTRA_REJECTION=PASS'

# CLI input is a bounded regular-file snapshot. Character devices are not
# admitted, and a zero-st_size proc-style pseudo-file must not be mistaken for
# an empty document merely because the initial metadata size is zero.
set +e
"$TOOL" /dev/null > "$tmp/dev-null.out" 2>&1
rc_dev_null=$?
set -e
[[ "$rc_dev_null" -eq 2 ]] || fail "/dev/null must be input-contract exit 2, got $rc_dev_null"
grep -Fq 'input must be a regular file' "$tmp/dev-null.out"
echo 'VIEW0_V1N0_CLI_NONREGULAR_FILE_REJECTION=PASS'

if [[ -r /proc/self/cmdline ]]; then
    set +e
    "$TOOL" /proc/self/cmdline > "$tmp/proc-self.out" 2>&1
    rc_proc_self=$?
    set -e
    [[ "$rc_proc_self" -eq 2 ]] ||
        fail "/proc/self/cmdline must fail stable-snapshot contract, got $rc_proc_self"
    grep -Fq 'input changed while reading' "$tmp/proc-self.out"
    echo 'VIEW0_V1N0_CLI_ZERO_ST_SIZE_CONTENT_EOF_GUARD=PASS'
else
    fail '/proc/self/cmdline unavailable on qualified Linux environment'
fi

: > "$tmp/empty-regular.html"
set +e
"$TOOL" --format=tsv "$tmp/empty-regular.html" > "$tmp/empty-regular.tsv"
rc_empty=$?
set -e
[[ "$rc_empty" -eq 0 || "$rc_empty" -eq 1 ]] ||
    fail "empty regular file must remain parser-clean under higher authoring extensions, got exit $rc_empty"
grep -Eq $'SUMMARY\t.*\tdiagnostics=[0-9]+\ttokenizer=0\ttree=0\tparse_clean=yes\tcomplete_conformance=no' \
    "$tmp/empty-regular.tsv"
echo 'VIEW0_V1N0_CLI_EMPTY_REGULAR_FILE_FOUNDATION_PARSE_CLEAN=PASS'

# TSV is a machine format: the path field may not contain TAB, LF, or CR.
for unsafe_kind in tab lf cr; do
    case "$unsafe_kind" in
        tab) unsafe_name=$'unsafe\tpath.html' ;;
        lf) unsafe_name=$'unsafe\npath.html' ;;
        cr) unsafe_name=$'unsafe\rpath.html' ;;
        *) fail 'internal unsafe path test kind' ;;
    esac
    unsafe_path="$tmp/$unsafe_name"
    cp tests/view0/v1/native/parse-error-tokenizer.html "$unsafe_path"
    set +e
    "$TOOL" --format=tsv "$unsafe_path" \
        > "$tmp/unsafe-$unsafe_kind.stdout" \
        2> "$tmp/unsafe-$unsafe_kind.stderr"
    rc_unsafe=$?
    set -e
    [[ "$rc_unsafe" -eq 2 ]] ||
        fail "unsafe TSV path kind=$unsafe_kind must exit 2, got $rc_unsafe"
    [[ ! -s "$tmp/unsafe-$unsafe_kind.stdout" ]] ||
        fail "unsafe TSV path kind=$unsafe_kind must publish no machine output"
    grep -Fq 'requires a FILE path without tab, LF, or CR bytes' \
        "$tmp/unsafe-$unsafe_kind.stderr"
done
echo 'VIEW0_V1N0_CLI_TSV_SAFE_PATH_POLICY=PASS'

# Exactly one-byte-over input must fail before Lexbor is entered.
head -c $((MAX_INPUT + 1)) /dev/zero | tr '\000' 'a' > "$tmp/oversize.html"
set +e
"$TOOL" "$tmp/oversize.html" > "$tmp/oversize.out" 2>&1
rc_oversize=$?
set -e
[[ "$rc_oversize" -eq 2 ]] || fail "oversize input must exit 2, got $rc_oversize"
grep -Fq 'input exceeds V1N0 limit' "$tmp/oversize.out"
echo 'VIEW0_V1N0_INPUT_SIZE_BOUND=PASS'

# Exercise the exact admitted 1 MiB input limit under the qualification
# process-memory and wall-clock guards using one bounded comment node.
prefix='<!doctype html><html><head><title>x</title></head><body><!--'
suffix='--></body></html>'
prefix_len=${#prefix}
suffix_len=${#suffix}
pad_len=$((MAX_INPUT - prefix_len - suffix_len))
printf '%s' "$prefix" > "$tmp/max-valid.html"
head -c "$pad_len" /dev/zero | tr '\000' 'a' >> "$tmp/max-valid.html"
printf '%s' "$suffix" >> "$tmp/max-valid.html"
[[ "$(stat -c %s "$tmp/max-valid.html")" -eq "$MAX_INPUT" ]] || fail 'max-valid fixture size mismatch'

timeout "$TIMEOUT_SECONDS" bash -c \
    'ulimit -v "$1"; exec "$2" --format=tsv "$3"' \
    _ "$RLIMIT_KIB" "$TOOL" "$tmp/max-valid.html" \
    > "$tmp/max-valid.tsv"
grep -Fq 'diagnostics=0' "$tmp/max-valid.tsv"
echo 'VIEW0_V1N0_MAX_INPUT_PARSE_UNDER_RESOURCE_GUARDS=PASS'

# Stress the configured diagnostic ceiling with source-derived tokenizer errors.
{
    printf '<!doctype html><html><head><title>x</title></head><body>'
    awk -v n="$MAX_DIAGNOSTICS" 'BEGIN { for (i = 0; i < n; ++i) printf "<?bad>" }'
    printf '</body></html>'
} > "$tmp/max-errors.html"
set +e
timeout "$TIMEOUT_SECONDS" bash -c \
    'ulimit -v "$1"; exec "$2" --format=tsv "$3"' \
    _ "$RLIMIT_KIB" "$TOOL" "$tmp/max-errors.html" \
    > "$tmp/max-errors.tsv" 2>&1
rc_max_errors=$?
set -e
[[ "$rc_max_errors" -eq 1 ]] || fail "4096-diagnostic stress must be document-invalid exit 1, got $rc_max_errors"
grep -Fq $'diagnostics=4096\ttokenizer=4096\ttree=0' "$tmp/max-errors.tsv"
echo 'VIEW0_V1N0_DIAGNOSTIC_COUNT_BOUND=PASS'

{
    printf '<!doctype html><html><head><title>x</title></head><body>'
    awk -v n="$((MAX_DIAGNOSTICS + 1))" 'BEGIN { for (i = 0; i < n; ++i) printf "<?bad>" }'
    printf '</body></html>'
} > "$tmp/too-many-errors.html"
set +e
"$TOOL" "$tmp/too-many-errors.html" > "$tmp/too-many-errors.out" 2>&1
rc_too_many=$?
set -e
[[ "$rc_too_many" -eq 3 ]] || fail "diagnostic overflow must be mechanism/resource exit 3, got $rc_too_many"
grep -Fq 'checker mechanism failure' "$tmp/too-many-errors.out"
echo 'VIEW0_V1N0_DIAGNOSTIC_OVERFLOW_FAIL_CLOSED=PASS'

# Active V1N0 source/gate path must have no Java/JAR dependency.
if grep -ERni --include='*.sh' --include='*.c' --include='*.h' \
    '(^|[^A-Za-z])(java|vnu\.jar)([^A-Za-z]|$)' \
    tools/view0_v1_native_foundation_gate.sh \
    tools/view0_v1_lexbor_acquire.sh \
    tools/view0_v1_lexbor_build.sh \
    tools/c/view0_conformance \
    tools/include/arborcore/view0_conformance \
    tests/c/view0_v1_native_foundation_test.c \
    tests/c/view0_v1_native_foundation_adversarial_test.c; then
    fail 'active V1N0 native checker unexpectedly references Java/vnu.jar'
fi
echo 'VIEW0_V1N0_JAVA_PROCESS_OR_JAR_REQUIRED=NO'
echo 'VIEW0_V1N0_LEXBOR_SOURCE_TREE_EXTRA_POLICY=REJECT_MODIFIED_UNTRACKED_AND_IGNORED'
echo 'VIEW0_V1N0_CLI_FILE_INPUT=REGULAR_FILE_BOUNDED_SNAPSHOT'
echo 'VIEW0_V1N0_CLI_EOF_GUARD=ONE_BYTE_AFTER_DECLARED_SIZE'
echo 'VIEW0_V1N0_CLI_TSV_PATH_POLICY=REJECT_TAB_LF_CR'

# Lexbor types stay out of Arborcore's public VIEW API and private adapter types
# stay out of production headers.
! grep -Rq 'lxb_' include/arborcore/view.h
! grep -Rq 'lexbor' include/arborcore/view.h
grep -Fq 'lxb_' tools/c/view0_conformance/lexbor_adapter.c
! grep -Fq 'lxb_' tools/include/arborcore/view0_conformance/native.h
echo 'VIEW0_V1N0_LEXBOR_PUBLIC_TYPE_LEAK=NO'

symbols=$(nm -g --defined-only build/libarborcore_view.a |
    awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' |
    LC_ALL=C sort)
count=$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)
[[ "$count" -eq 11 ]] || fail "V1N0 unexpectedly changed production VIEW symbols: $count"
echo 'VIEW0_V1N0_PRODUCTION_VIEW_SYMBOL_COUNT=11'

cc -Iinclude -Itools/include -isystem "$LEX/source" -D_POSIX_C_SOURCE=200809L \
    -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -fsyntax-only \
    tools/c/view0_conformance/native.c \
    tools/c/view0_conformance/lexbor_adapter.c \
    tools/c/view0_conformance/main.c \
    tests/c/view0_v1_native_foundation_test.c \
    tests/c/view0_v1_native_foundation_adversarial_test.c

echo 'VIEW0_V1N0_GCC_FANALYZER=PASS'
echo "VIEW0_V1N0_INPUT_MAX_BYTES=$MAX_INPUT"
echo "VIEW0_V1N0_DIAGNOSTIC_MAX_COUNT=$MAX_DIAGNOSTICS"
echo "VIEW0_V1N0_QUALIFICATION_RLIMIT_AS_KIB=$RLIMIT_KIB"
echo "VIEW0_V1N0_QUALIFICATION_TIMEOUT_SECONDS=$TIMEOUT_SECONDS"
echo 'VIEW0_V1N0_AUTHORING_RULE_GROUPS_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N0_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'PASS: VIEW0 V1N0 native C/Lexbor parser foundation, diagnostics, bounds and no-Java boundary'
