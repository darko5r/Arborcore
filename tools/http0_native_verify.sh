#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s http0-header-test http0-response-test http0-adversarial-test http0-integration-test
./build/http0-header-test; ./build/http0-response-test; ./build/http0-adversarial-test; ./build/http0-integration-test
echo 'PASS: HTTP0 zero-copy headers, response semantics, adversarial and parser integration qualification'
