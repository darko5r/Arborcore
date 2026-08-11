#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
CPU="$(arborcore_bench_cpu)"
OUT="$ROOT/build/server-benchmark-perf"
mkdir -p "$OUT"

if ! command -v perf >/dev/null 2>&1; then
    echo "SKIP: perf is not installed."
    exit 0
fi
if ! perf stat -e task-clock true >/dev/null 2>&1; then
    echo "SKIP: perf_event access is not available to this process."
    exit 0
fi

events='cycles,instructions,branches,branch-misses,cache-references,cache-misses,context-switches,page-faults'
for bin in bench-lifecycle bench-loopback; do
    echo "### perf: $bin"
    if perf stat -x $'\t' -r 3 -e "$events" taskset -c "$CPU" "$ROOT/build/$bin" \
        >/dev/null 2>"$OUT/$bin.txt"; then
        cat "$OUT/$bin.txt"
    else
        echo "SKIP: requested hardware counters are not permitted for $bin."
    fi
    echo
done
