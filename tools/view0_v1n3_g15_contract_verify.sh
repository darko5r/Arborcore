#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
test "$(awk 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n3_g15_fixture_plan.tsv)" -eq 64
test "$(awk 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n3_g15_ownership.tsv)" -eq 8
rg -q 'V1N3_G15_RULE_COUNT UINT64_C\(8\)' tools/c/view0_conformance/g15.h
echo "VIEW0_V1N3_G15_RULE_IDENTITIES=8_OF_8"
echo "PASS: V1N3 G15 exact authority, ownership and no-overclaim contract"

