#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
test "$(find tools/c/view0_conformance -maxdepth 1 -type f -name 'ecma_*' | wc -l)" -eq 15
test "$(find tests/c -maxdepth 1 -type f -name 'view0_v1n3_ecma_*_test.c' | wc -l)" -eq 4
test "$(find tests/data -maxdepth 1 -type f -name 'view0_v1n3_ecma_*.tsv' | wc -l)" -eq 5
echo "VIEW0_V1N3_ECMA_SOURCE_PATHS=24"
echo "PASS: exact parse-only ECMAScript frontend scope"
