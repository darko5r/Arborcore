#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s build/http1-adapter-bench
out=build/http1-benchmark; rm -rf "$out"; mkdir -p "$out"
: >"$out/rounds.txt"
for round in $(seq 1 9); do
  raw="$out/round-$round.txt"
  ./build/http1-adapter-bench >"$raw"
  iterations=$(awk -F= '$1=="HTTP1_ADAPTER_VALIDATE_ITERATIONS"{print $2}' "$raw")
  checksum=$(awk -F= '$1=="HTTP1_ADAPTER_VALIDATE_CHECKSUM"{print $2}' "$raw")
  total=$(awk -F= '$1=="HTTP1_ADAPTER_VALIDATE_NS_TOTAL"{print $2}' "$raw")
  [[ "$iterations" == 200000 && "$checksum" == 200000 && "$total" =~ ^[0-9]+$ ]]
  awk -v total="$total" 'BEGIN { printf "%.9f\n", total / 200000.0 }' >>"$out/rounds.txt"
done
median=$(LC_ALL=C sort -n "$out/rounds.txt" | awk 'NR==5{printf "%.3f",$1}')
{
  echo "HTTP1_ADAPTER_VALIDATE_MEDIAN_NS_PER_OP=$median"
  echo 'HTTP1_ADAPTER_VALIDATE_ITERATIONS=200000'
  echo 'HTTP1_ADAPTER_VALIDATE_ROUNDS=9'
  echo 'HTTP1_ADAPTER_VALIDATE_CHECKSUM=200000'
  echo 'HTTP1_BENCHMARK_POLICY=DIAGNOSTIC_BASELINE_ONLY'
  echo 'HTTP1_PERFORMANCE_THRESHOLD_INVENTED=NO'
} | tee "$out/result.txt"
echo 'PASS: HTTP1 diagnostic adapter-validation benchmark recorded'
