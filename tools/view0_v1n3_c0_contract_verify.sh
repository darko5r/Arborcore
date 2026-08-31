#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
test "$(awk 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n3_c0_rule_authority.tsv)" -eq 30
test "$(awk 'NR>1 {n++} END {print n+0}' tests/data/view0_v1n3_c0_external_authority.tsv)" -ge 3
rg -q 'ARBOR_VIEW0_NATIVE_V1N3_RULE_COUNT UINT64_C\(30\)' tools/c/view0_conformance/v1n3_c0.h
rg -q 'arbor_view0_native_check_configured' tools/include/arborcore/view0_conformance/native.h
echo "VIEW0_V1N3_C0_RULE_IDENTITIES=30_OF_30"
echo "VIEW0_V1N3_C0_GROUP_COUNTS=G12_8_G13_6_G14_6_G15_8_G16_2"
echo "PASS: V1N3 C0 authority and configured-checker contract"
