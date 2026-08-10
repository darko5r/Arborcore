#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

required=(
  build/buffer.o
  build/arena.o
  build/memory.o
  build/buffer-test
  build/arena-test
  build/polish-gate1-test
)

for path in "${required[@]}"; do
  if [[ ! -e "$path" ]]; then
    echo "FAIL: Polish Gate #1 missing $path" >&2
    exit 1
  fi
done

echo "### POLISH GATE #1: buffer"
./build/buffer-test
echo "PASS: buffer-test"

echo
echo "### POLISH GATE #1: arena / VM"
./build/arena-test
echo "PASS: arena-test"

echo
echo "### POLISH GATE #1: cross-layer integration"
./build/polish-gate1-test
echo "PASS: polish-gate1-test"

echo
echo "### POLISH GATE #1: production dependencies"
buffer_undef="$(nm -u build/buffer.o | awk '{print $NF}' | sort)"
expected_buffer_undef="$(printf '%s\n' memory_copy memory_move | sort)"
if [[ "$buffer_undef" != "$expected_buffer_undef" ]]; then
  echo "FAIL: buffer.o dependency set differs from the qualified memory engine contract" >&2
  nm -u build/buffer.o >&2
  exit 1
fi
if [[ -n "$(nm -u build/arena.o)" ]]; then
  echo "FAIL: arena.o has unexpected undefined symbols" >&2
  nm -u build/arena.o >&2
  exit 1
fi
echo "PASS: buffer.o reuses memory_copy/memory_move; arena.o is self-contained"

echo
echo "### POLISH GATE #1: integration link closure"
if [[ -n "$(nm -u build/polish-gate1-test)" ]]; then
  echo "FAIL: polish-gate1-test has unresolved symbols" >&2
  nm -u build/polish-gate1-test >&2
  exit 1
fi
echo "PASS: integration executable has no unresolved symbols"

echo
echo "### POLISH GATE #1: GNU-stack"
for obj in build/buffer.o build/arena.o build/memory.o build/polish_gate1_test.o; do
  if ! readelf -SW "$obj" | grep -q '\.note.GNU-stack'; then
    echo "FAIL: $obj has no .note.GNU-stack" >&2
    exit 1
  fi
  echo "PASS: $obj"
done

echo
echo "### POLISH GATE #1: code-size baseline"
size build/buffer.o build/arena.o

echo
echo "POLISH GATE #1 PASSED"
