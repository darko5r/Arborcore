#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s build/http0-response-bench
OUT=build/http0-benchmark; rm -rf "$OUT"; mkdir -p "$OUT"
./build/http0-response-bench | tee "$OUT/result.txt"
grep -Eq '^HTTP0_RESPONSE_SERIALIZE_MEDIAN_NS_PER_OP=[0-9]+\.[0-9]+$' "$OUT/result.txt"
grep -Fxq 'HTTP0_RESPONSE_SERIALIZE_ITERATIONS=200000' "$OUT/result.txt"
grep -Fxq 'HTTP0_RESPONSE_SERIALIZE_ROUNDS=9' "$OUT/result.txt"
echo 'HTTP0_BENCHMARK_POLICY=DIAGNOSTIC_BASELINE_ONLY'; echo 'HTTP0_PERFORMANCE_THRESHOLD_INVENTED=NO'
echo 'PASS: HTTP0 diagnostic response-serialization benchmark recorded'
