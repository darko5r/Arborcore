#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
doc=docs/VIEW_CORE_VIEW0.md
readme=examples/view0/README.md
render=examples/view0/render.c
asm=examples/view0/nasm_view.asm
makefile=Makefile

for path in "$doc" "$readme" "$render" "$asm" "$makefile" examples/view0/page.html; do
  [[ -s "$path" ]] || { echo "FAIL: missing/empty D1 artifact: $path" >&2; exit 1; }
done

grep -Fq '### D1: canonical manuals, file-based runnable examples, and consistency gate' "$doc"
grep -Fq 'make view0-d1-example-documents' "$doc"
grep -Fq 'make view0-d1-gate' "$doc"
grep -Fq 'build/view0-d1/documents/template.html' "$doc"
grep -Fq 'build/view0-d1/documents/native-c.html' "$doc"
grep -Fq 'build/view0-d1/documents/nasm.html' "$doc"
grep -Fq 'Complete HTML and complete ECMAScript conformance are not' "$doc"

grep -Fq 'make view0-d1-example-documents' "$readme"
grep -Fq 'T1 prepared template loaded from `page.html` at startup' "$readme"
grep -Fq 'C3-style typed native C compiled view' "$readme"
grep -Fq 'C4-style hand-written NASM consumer' "$readme"
grep -Fq 'Content-Type: text/html; charset=utf-8' "$readme"
grep -Fq 'controller or middleware' "$readme"
grep -Fq 'does not reopen Assembly ABI v1' "$readme"

grep -Fq 'Content-Type: text/html; charset=utf-8' "$render"
grep -Fq 'middleware that has already added Content-Type violates' "$render"
grep -Fq 'Content-Type: text/html; charset=utf-8' "$asm"
grep -Fq 'Controllers/middleware must' "$asm"
grep -Fq 'introduces no new Arborcore Assembly ABI symbol' "$asm"

grep -Fq 'view0-d1-example-documents:' "$makefile"
grep -Fq 'view0-d1-gate:' "$makefile"

grep -Fq 'VIEW0_M1_COMMON_HTML_PUBLICATION_SEQUENCE=RENDER_VALIDATE_UTF8_MAKE_PLAN_APPEND_CONTENT_TYPE_PUBLISH_PLAN' view/arborcore-view-core-1.contract
grep -Fq 'VIEW0_M1_CONTENT_TYPE_NAME=Content-Type' view/arborcore-view-core-1.contract
grep -Fq 'VIEW0_M1_CONTENT_TYPE_VALUE=text/html; charset=utf-8' view/arborcore-view-core-1.contract
grep -Fq 'static const uint8_t name[] = "Content-Type";' tests/c/view0_m1_integration_test.c
grep -Fq 'static const uint8_t value[] = "text/html; charset=utf-8";' tests/c/view0_m1_integration_test.c

echo 'VIEW0_D1_MANUAL_SURFACES=2_OF_2'
echo 'VIEW0_D1_CONTENT_TYPE_SINGLETON_DOCUMENTATION=CONSISTENT'
echo 'VIEW0_D1_C4_ASSEMBLY_STABILITY_DOCUMENTATION=CONSISTENT'
echo 'PASS: VIEW0 D1 documentation and runnable-example instructions are mutually consistent'
