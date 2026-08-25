#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
JAR='build/view0-v1/oracle/vnu.jar'
VNU_SHA256='c70279e260e5f4f9e95b3890ef6c9548f90ebdaec2a75219ec41694ab6241e34'
DOC_DIR='build/view0-v1/documents'

fail() { printf 'FAIL: %s\n' "$*" >&2; exit 1; }

bash tools/view0_v1_checker_acquire.sh
[[ -f "$JAR" ]] || fail 'V1 oracle jar missing'
[[ "$(sha256sum "$JAR" | awk '{print $1}')" == "$VNU_SHA256" ]] || fail 'V1 oracle jar identity mismatch'
mkdir -p "$DOC_DIR"
make -s view0-v1-render-artifacts

checker=(java -jar "$JAR" --asciiquotes --no-langdetect --Werror --format gnu)
for doc in \
    "$DOC_DIR/template.html" \
    "$DOC_DIR/native-c.html" \
    "$DOC_DIR/nasm.html"; do
    [[ -s "$doc" ]] || fail "missing generated V1 document: $doc"
    LC_ALL=C "${checker[@]}" "$doc"
done

echo 'VIEW0_V1_TEMPLATE_DOCUMENT_CONFORMANCE=PASS'
echo 'VIEW0_V1_NATIVE_C_DOCUMENT_CONFORMANCE=PASS'
echo 'VIEW0_V1_NASM_DOCUMENT_CONFORMANCE=PASS'

for invalid in \
    tests/view0/v1/invalid/ul-text.html \
    tests/view0/v1/invalid/no-doctype.html; do
    output="build/view0-v1/$(basename "$invalid").diagnostic.txt"
    if LC_ALL=C java -jar "$JAR" --asciiquotes --no-langdetect --errors-only --format gnu \
        "$invalid" >"$output" 2>&1; then
        fail "pinned checker unexpectedly accepted intentionally nonconforming fixture: $invalid"
    fi
    [[ -s "$output" ]] || fail "checker rejected $invalid without diagnostic output"
done

echo 'VIEW0_V1_NEGATIVE_ORACLE_UL_TEXT=REJECT'
echo 'VIEW0_V1_NEGATIVE_ORACLE_NO_DOCTYPE=REJECT'
echo 'VIEW0_V1_CONFORMANCE_ERRORS=BLOCKING'
echo 'VIEW0_V1_WARNINGS=BLOCKING_FOR_VIEW0_QUALIFIED_ARTIFACT_ADMISSION'
echo 'PASS: pinned external oracle accepts all three generated VIEW documents and rejects known-invalid controls'
