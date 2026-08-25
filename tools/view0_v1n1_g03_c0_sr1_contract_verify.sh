#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
need_hash(){ [[ "$(sha256sum "$1" | awk '{print $1}')" == "$2" ]] || fail "hash drift: $1"; }
contract=view/arborcore-view-core-1.contract
header=tools/include/arborcore/view0_conformance/native.h
adapter=tools/c/view0_conformance/lexbor_adapter.c
prov=tools/c/view0_conformance/g03_provenance.c
internal=tools/c/view0_conformance/g03_provenance_internal.h
need_hash tests/data/view0_v1n1_g03_c0_sr1_insertion_mode_enum.tsv 2a86df935aa198c7a5b53f34a60cfec74bc19a6fc42152acd6ce9865f3a3a9db
need_hash tests/data/view0_v1n1_g03_c0_sr1_record_layout.tsv 4faf4b4b4f890d2c5770ce43b10aba06d9ef2b58e38558c267f8bdab16255364
need_hash tests/data/view0_v1n1_g03_c0_sr1_delivery_semantics.tsv 67b080eaf5292ab8a24bb62051072d2cd14ee84976e9b7a4a8e805a9d3863aae
need_hash tests/data/view0_v1n1_g03_c0_sr1_qualification_controls.tsv b74075a5bef7863db014be7978714147e520a5023244e65845ddd30dd7e4b753
for item in \
 'VIEW0_V1N1_G03_C0_SR1_SOURCE_REPAIR_CONTEXT=QUALIFIED' \
 'VIEW0_V1N1_G03_C0_SR1_STABLE_INSERTION_MODE_COUNT=23' \
 'VIEW0_V1N1_G03_C0_SR1_RECORD_FIELD_COUNT=13' \
 'VIEW0_V1N1_G03_C0_SR1_RECORD_SIZE_X86_64=104' \
 'VIEW0_V1N1_G03_C0_SR1_PARSER_PASS_COUNT=ONE_PINNED_LEXBOR_PARSE_ONLY' \
 'VIEW0_V1N1_G03_C0_SR1_INSERTION_CAPTURE=STATIC_LINK_WRAP_LXB_HTML_TREE_INSERT_FOREIGN_ELEMENT' \
 'VIEW0_V1N1_G03_C0_SR1_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G03_C0_SR1_PRODUCTION_VIEW_API_GROWTH=NO' \
 'VIEW0_V1N1_G03_C0_SR1_ACTIVE_TOP_CONTRACT=0.1-VIEW0-V1N1-G03-R4A' \
 'VIEW0_V1N1_G03_C0_SR1_R5_RULE_IMPLEMENTATION=NO'; do
 grep -Fxq "$item" "$contract" || fail "contract marker missing: $item"
done
[[ "$(grep -Ec '^    ARBOR_VIEW0_NATIVE_INSERTION_MODE_[A-Z0-9_]+ = ([1-9][0-9]*),?$' "$header")" -eq 23 ]] || fail 'stable insertion-mode enum count drift'
grep -Fq 'typedef struct arbor_view0_native_source_repair_context {' "$header" || fail 'SR1 record missing'
grep -Fq 'arbor_view0_native_source_repair_observer_f source_repair;' "$header" || fail 'SR1 observer delivery missing'
grep -Fq '_Static_assert(sizeof(arbor_view0_native_source_repair_context) == 104u' "$adapter" || fail 'SR1 layout static assert missing'
grep -Fq '__wrap_lxb_html_tree_insert_foreign_element' "$prov" || fail 'SR1 insertion wrapper missing'
grep -Fq '__real_lxb_html_tree_insert_foreign_element' "$prov" || fail 'SR1 insertion wrapper does not delegate'
grep -Fq 'context->source_repair_token == token' "$prov" || fail 'exact source-token pointer correlation missing'
grep -Fq 'begin != context->source_repair_record.source_offset' "$prov" || fail 'source range correlation missing'
grep -Fq 'tree->foster_parenting' "$prov" || fail 'neutral foster state capture missing'
grep -Fq 'capture.downstream_context != (void *)capture.tree' "$adapter" || fail 'downstream tree identity check missing'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked through Arborcore private neutral header'
! grep -ERq 'ARBOR_VIEW_V1_G03_|0x000000003003' "$adapter" "$prov" "$internal" || fail 'C0-SR1 mechanism acquired G03 rule semantics'
if grep -ERq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' "$adapter" "$prov"; then fail 'C0-SR1 direct Arborcore heap allocation'; fi
count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "production VIEW API count changed: $count"
! grep -Rqs 'ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE\|0x0000000030030005' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R5 rule implemented during C0-SR1'
echo 'VIEW0_V1N1_G03_C0_SR1_STABLE_INSERTION_MODE_COUNT=23'
echo 'VIEW0_V1N1_G03_C0_SR1_RECORD_FIELD_COUNT=13'
echo 'VIEW0_V1N1_G03_C0_SR1_RECORD_SIZE_X86_64=104'
echo 'VIEW0_V1N1_G03_C0_SR1_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'VIEW0_V1N1_G03_C0_SR1_R5_RULE_IMPLEMENTATION=NO'
echo 'PASS: C0-SR1 neutral single-parser contract, layout, ownership and no-growth boundary'
