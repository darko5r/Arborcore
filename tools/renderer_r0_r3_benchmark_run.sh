#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BENCH="${ARBOR_RENDERER_FOUNDATION_BENCH:-$ROOT/build/renderer-foundation-bench}"
OUT="$ROOT/build/renderer-r0-r3"
RAW="$OUT/raw.tsv"
SUMMARY="$OUT/summary.tsv"
RUNS=31

mkdir -p "$OUT"
: > "$RAW"

for ((i=1; i<=RUNS; ++i)); do
    echo "R0-R3 measured run $i/$RUNS" >&2
    "$BENCH" | tail -n +2 | while IFS=$'\t' read -r metric value; do
        printf '%s\t%s\n' "$metric" "$value" >> "$RAW"
    done
done

python3 - "$RAW" "$SUMMARY" <<'PY'
import statistics, sys
raw, summary = sys.argv[1:]
values = {}
with open(raw, encoding='utf-8') as f:
    for line in f:
        metric, value = line.rstrip('\n').split('\t')
        values.setdefault(metric, []).append(float(value))
order = ['coverage_q24','coverage_q32','rgba8_fill','rgba16_fill','rgba32_fill']
with open(summary, 'w', encoding='utf-8') as f:
    f.write('metric\tmedian_ns_per_op\n')
    for metric in order:
        f.write(f'{metric}\t{statistics.median(values[metric]):.6f}\n')
PY

cat "$SUMMARY"
echo "renderer_foundation_benchmark_raw=$RAW"
echo "renderer_foundation_benchmark_summary=$SUMMARY"
