#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
cc -Iinclude -Itools/include -Itools/c/view0_conformance -std=c17 -Wall -Wextra -Wpedantic -Werror tools/c/view0_conformance/ecma_unicode_gen.c tools/c/view0_conformance/ecma_unicode_tables.c -o build/view0-v1/native/v1n3-ecma-unicode-gen
./build/view0-v1/native/v1n3-ecma-unicode-gen | grep -q '^VIEW0_V1N3_UNICODE_TABLES=17\.0\.0:'
make view0-v1n3-ecma-test
echo "VIEW0_V1N3_UNICODE_GENERATION=PASS"
