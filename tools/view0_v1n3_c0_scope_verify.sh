#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
actual=$(mktemp)
expected=$(mktemp)
trap 'rm -f "$actual" "$expected"' EXIT
{ git diff --name-only 4071c52433cbb24e33e5f007fcbf321c01316449 --; git ls-files --others --exclude-standard; } | sort -u >"$actual"
printf '%s\n' Makefile docs/VIEW_CORE_VIEW0.md tests/c/view0_v1n3_*.c tests/data/view0_v1n3_*.tsv tools/c/view0_conformance/ecma_*.c tools/c/view0_conformance/ecma_*.h tools/c/view0_conformance/g1[2-6].c tools/c/view0_conformance/g1[2-6].h tools/c/view0_conformance/native.c tools/c/view0_conformance/v1n3_c0.c tools/c/view0_conformance/v1n3_c0.h tools/include/arborcore/view0_conformance/native.h tools/view0_v1n3_*.sh view/arborcore-view-core-1.contract | sort -u >"$expected"
test "$(wc -l <"$actual")" -eq 98
cmp "$actual" "$expected"
! grep -Eq 'view0_v1n4|/g1[7-9]|/g[2-9][0-9]' "$actual"
echo "VIEW0_V1N3_C0_TRANSACTION_PATHS=98"
echo "VIEW0_V1N3_C0_LATER_GROUP_SOURCE_PATHS=ZERO"
echo "PASS: exact V1N3 C1 isolated source scope"
