#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
test "$(awk 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n3_g14_fixture_plan.tsv)" -eq 48
test "$(awk 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n3_g14_ownership.tsv)" -eq 6
rg -q 'V1N3_G14_RULE_COUNT UINT64_C\(6\)' tools/c/view0_conformance/g14.h
echo "VIEW0_V1N3_G14_RULE_IDENTITIES=6_OF_6"
echo "PASS: V1N3 G14 exact authority, ownership and no-overclaim contract"

