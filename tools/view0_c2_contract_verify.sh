#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
contract=view/arborcore-view-core-1.contract
header=include/arborcore/view.h
source=src/c/view.c
doc=docs/VIEW_CORE_VIEW0.md
for item in \
 'VIEW0_C2_CONTRACT_REVISION=0.1-VIEW0-C2-R1' \
 'VIEW0_C2_SCOPE=HTML_NORMAL_DATA_TEXT_ESCAPING' \
 'VIEW0_C2_PUBLIC_FUNCTION_COUNT=2' \
 'VIEW0_C2_DYNAMIC_CONTEXT=HTML_NORMAL_DATA_TEXT_ONLY' \
 'VIEW0_C2_AMPERSAND_SERIALIZATION=&amp;' \
 'VIEW0_C2_LESS_THAN_SERIALIZATION=&lt;' \
 'VIEW0_C2_GREATER_THAN_SERIALIZATION=&gt;' \
 'VIEW0_C2_DOUBLE_QUOTE_SERIALIZATION=LITERAL' \
 'VIEW0_C2_APOSTROPHE_SERIALIZATION=LITERAL' \
 'VIEW0_C2_HTML_ATTRIBUTE_CONTEXT=NOT_ADMITTED' \
 'VIEW0_C2_URL_CONTEXT=NOT_ADMITTED' \
 'VIEW0_C2_CSS_CONTEXT=NOT_ADMITTED' \
 'VIEW0_C2_JAVASCRIPT_CONTEXT=NOT_ADMITTED' \
 'VIEW0_C2_XML_CONTEXT=NOT_ADMITTED' \
 'VIEW0_C2_GENERIC_ESCAPE_EVERY_CONTEXT_API=FORBIDDEN' \
 'VIEW0_C2_MEASUREMENT=EXACT_FAILURE_ATOMIC' \
 'VIEW0_C2_MEASURE_TEXT_ALIAS=REJECT' \
 'VIEW0_C2_CROSS_PASS_SOURCE_SNAPSHOT=NO' \
 'VIEW0_C2_CROSS_PASS_SOURCE_STABILITY=CALLER_REQUIRED' \
 'VIEW0_C2_WORST_CASE_EXPANSION_FACTOR=5' \
 'VIEW0_C2_SOURCE_ACTIVE_OUTPUT_BODY_ALIAS=REJECT' \
 'VIEW0_C2_SOURCE_PRIOR_SAME_ARENA_NONOVERLAP=ALLOW' \
 'VIEW0_C2_NON_ESCAPE_BYTE_POLICY=PRESERVE_EXACTLY_IN_SERIALIZED_STREAM' \
 'VIEW0_C2_ARBITRARY_BYTE_DOM_ROUNDTRIP=NOT_CLAIMED' \
 'VIEW0_C2_UTF8_UNICODE_VALIDATION=OUT_OF_SCOPE' \
 'VIEW0_C2_HTML_INTEGRATION_ENCODING_REQUIREMENT=UTF8_VALIDITY_BEFORE_USER_FACING_ADMISSION' \
 'VIEW0_C2_HIDDEN_HEAP=ZERO' \
 'VIEW0_C2_DATABASE_DEPENDENCY=NONE'; do
  grep -Fxq "$item" "$contract" || { echo "FAIL: missing C2 contract marker: $item" >&2; exit 1; }
done
count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' "$header")
[[ "$count" -ge 7 ]] || { echo "FAIL: cumulative VIEW public surface lost C1/C2 functions; got $count" >&2; exit 1; }
grep -Fq 'arbor_view_html_text_measure' "$header"
grep -Fq 'arbor_view_html_text_append' "$header"
grep -Fq 'html_text_measure_additional' "$source"
grep -Fq 'html_text_source_aliases_active_output' "$source"
grep -Fq 'entity = "&amp;"' "$source"
grep -Fq 'entity = "&lt;"' "$source"
grep -Fq 'entity = "&gt;"' "$source"
! grep -Eq '\b(malloc|calloc|realloc|free|pthread_|mtx_|atomic_)\b' "$source"
grep -Fq '`escape_everything()` API.' "$doc"
grep -Fq 'C2 is byte-oriented and preserves every non-escape byte exactly in the' "$doc"
grep -Fq 'Measurement is not a snapshot.' "$doc"
echo 'VIEW0_C2_PUBLIC_FUNCTION_COUNT=2'
echo "VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=$count"
echo 'VIEW0_C2_HIDDEN_HEAP_COUNT=0'
echo 'VIEW0_C2_INTERNAL_LOCK_ATOMIC_COUNT=0'
echo 'PASS: VIEW0 C2 HTML text context, escaping, resource and separation contract'
