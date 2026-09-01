#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

make -s view0-d1-example-documents

example=build/view0-d1/render-example
asm_obj=build/view0-d1/nasm_view.o
doc_dir=build/view0-d1/documents
docs=("$doc_dir/template.html" "$doc_dir/native-c.html" "$doc_dir/nasm.html")

[[ -x "$example" ]] || { echo 'FAIL: D1 runnable example binary missing' >&2; exit 1; }
[[ -s "$asm_obj" ]] || { echo 'FAIL: D1 NASM example object missing' >&2; exit 1; }
for doc in "${docs[@]}"; do [[ -s "$doc" ]] || { echo "FAIL: D1 generated document missing: $doc" >&2; exit 1; }; done
cmp -s "${docs[0]}" "${docs[1]}" || { echo 'FAIL: template/native-C D1 output differs' >&2; exit 1; }
cmp -s "${docs[0]}" "${docs[2]}" || { echo 'FAIL: template/NASM D1 output differs' >&2; exit 1; }

# The example NASM object must remain an application consumer of the existing C VIEW API.
[[ "$(nm -g --defined-only "$asm_obj" | awk '$3 ~ /^arborcore_view0_d1_/ {n++} END{print n+0}')" -eq 1 ]] || {
  echo 'FAIL: D1 NASM example must define exactly one example-local global symbol' >&2; exit 1;
}
nm -g --defined-only "$asm_obj" | awk '{print $3}' | grep -Fxq arborcore_view0_d1_nasm_render_html_text
for sym in arbor_view_measure_add arbor_view_html_text_measure arbor_view_output_begin arbor_view_output_append arbor_view_html_text_append arbor_view_output_commit; do
  nm -u "$asm_obj" | awk '{print $2}' | grep -Fxq "$sym" || { echo "FAIL: D1 NASM object missing VIEW C import $sym" >&2; exit 1; }
done
for sym in range_end_checked range_overlaps; do
  nm -u "$asm_obj" | awk '{print $2}' | grep -Fxq "$sym" || { echo "FAIL: D1 NASM object missing frozen range import $sym" >&2; exit 1; }
done
readelf -W -S "$asm_obj" | grep -q '\.note\.GNU-stack' || { echo 'FAIL: D1 NASM object lacks GNU-stack note' >&2; exit 1; }

# Current V1N4 native checker, not historical Java/v.Nu, qualifies all generated files.
# Establish the exact pinned Lexbor release source/build before native.c can compile in a fresh clone.
make -s view0-v1n0-lexbor-release >/dev/null
make -s view0-v1n0-tool
checker=build/view0-v1/native/arborcore-view0-html-check
[[ -x "$checker" ]] || { echo 'FAIL: current native V1 checker missing' >&2; exit 1; }
for doc in "${docs[@]}"; do
  out=$($checker --format=tsv "$doc")
  summary=$(printf '%s\n' "$out" | awk -F '\t' '$1=="SUMMARY"{line=$0} END{print line}')
  [[ -n "$summary" ]] || { echo "FAIL: no V1 summary for $doc" >&2; exit 1; }
  [[ "$summary" == *$'\tdiagnostics=0\t'* ]] || { echo "FAIL: V1 diagnostics nonzero for $doc" >&2; printf '%s\n' "$out" >&2; exit 1; }
  [[ "$summary" == *$'\ttokenizer=0\t'* ]] || { echo "FAIL: tokenizer errors for $doc" >&2; exit 1; }
  [[ "$summary" == *$'\ttree=0\t'* ]] || { echo "FAIL: tree errors for $doc" >&2; exit 1; }
  [[ "$summary" == *$'\tparse_clean=yes\t'* ]] || { echo "FAIL: parse not clean for $doc" >&2; exit 1; }
  [[ "$summary" == *$'\tcomplete_conformance=no\t'* ]] || { echo "FAIL: D1 must not promote complete conformance" >&2; exit 1; }
done

# Carry the executable M1 single-Content-Type and C4 real-NASM stability obligations.
bash tools/view0_m1_native_verify.sh >/dev/null
bash tools/view0_c4_abi_verify.sh >/dev/null

echo "VIEW0_D1_DOCUMENT_SHA256=$(sha256sum "${docs[0]}" | awk '{print $1}')"
echo 'VIEW0_D1_RENDERER_MODES=3_OF_3'
echo 'VIEW0_D1_RENDERER_BYTE_EQUIVALENCE=PASS'
echo 'VIEW0_D1_NATIVE_V1N4_DOCUMENTS=3_OF_3_ZERO_DIAGNOSTICS_PARSE_CLEAN'
echo 'VIEW0_D1_M1_CONTENT_TYPE_SINGLETON_REAL_SOCKET_REGRESSION=PASS'
echo 'VIEW0_D1_C4_REAL_NASM_ABI_REGRESSION=PASS'
echo 'PASS: VIEW0 D1 runnable file-based examples and carried M1/C4 obligations'
