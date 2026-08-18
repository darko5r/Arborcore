#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s http1-core-test http1-adversarial-test http1-integration-test http1-socket-test
./build/http1-core-test
./build/http1-adversarial-test
./build/http1-integration-test
./build/http1-socket-test
echo 'PASS: HTTP1 AF1 retrofit, MVC0 reuse, dynamic HTTP fields and real-socket qualification'
