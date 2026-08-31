#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
test -f tools/c/view0_conformance/g12.c
test -f tools/c/view0_conformance/g12.h
test -f tests/c/view0_v1n3_g12_test.c
test -f tests/c/view0_v1n3_g12_adversarial_test.c
test -f tests/c/view0_v1n3_g12_global_failure_atomicity_test.c
test -f tests/data/view0_v1n3_g12_fixture_plan.tsv
test -f tests/data/view0_v1n3_g12_ownership.tsv
echo "VIEW0_V1N3_G12_SOURCE_SCOPE=PASS"

