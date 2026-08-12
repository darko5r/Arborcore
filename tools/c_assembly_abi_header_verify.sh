#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
HEADER="$ROOT/include/arborcore/assembly_abi.h"
ABI="$ROOT/abi/arborcore-1.symbols"

work="$(mktemp -d /tmp/arborcore-c-abi-header.XXXXXX)"
trap 'rm -rf "$work"' EXIT

grep '^ARBOR_ASM_EXTERN' "$HEADER" \
  | sed -E 's/.*[ *]([A-Za-z_][A-Za-z0-9_]*)\(.*/\1/' \
  | sort -u > "$work/header.symbols"
sort -u "$ABI" > "$work/abi.symbols"

if ! cmp -s "$work/header.symbols" "$work/abi.symbols"; then
    echo "FAIL: C Assembly-ABI header declarations do not match frozen ABI v1." >&2
    diff -u "$work/abi.symbols" "$work/header.symbols" >&2 || true
    exit 1
fi

count="$(wc -l < "$work/header.symbols" | tr -d ' ')"
if [[ "$count" != "94" ]]; then
    echo "FAIL: expected 94 C-visible Assembly ABI declarations, got $count." >&2
    exit 1
fi

cc="${CC:-cc}"
"$cc" -std=c17 -Wall -Wextra -Wpedantic -Werror -I"$ROOT/include" \
  -x c -fsyntax-only - <<'SRC'
#include <stdint.h>
#include <arborcore/assembly_abi.h>

_Static_assert(
    _Generic(&server_create_epoll, int64_t (*)(int64_t): 1, default: 0),
    "server_create_epoll C declaration must preserve listener-fd RDI contract");

int main(void) { return 0; }
SRC

echo "c_assembly_abi_declaration_count=$count"
echo "PASS: C Assembly ABI header mirrors frozen v1 symbols/layouts and server call shape"
