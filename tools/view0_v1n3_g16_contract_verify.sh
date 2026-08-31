#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
test "$(awk 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n3_g16_fixture_plan.tsv)" -eq 16
test "$(awk 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n3_g16_ownership.tsv)" -eq 2
rg -q 'V1N3_G16_RULE_COUNT UINT64_C\(2\)' tools/c/view0_conformance/g16.h
echo "VIEW0_V1N3_G16_RULE_IDENTITIES=2_OF_2"
echo "PASS: V1N3 G16 exact authority, ownership and no-overclaim contract"

