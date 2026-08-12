#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
BENCH="${ARBOR_GEOMETRY_BENCH:-$ROOT/build/geometry-precision-bench}"
OUT_DIR="${ARBOR_GEOMETRY_BENCH_OUT:-$ROOT/build/geometry-precision-g0-g1}"
RUNS="${ARBOR_GEOMETRY_BENCH_RUNS:-31}"

if (( RUNS < 3 || RUNS % 2 == 0 )); then
    echo "FAIL: ARBOR_GEOMETRY_BENCH_RUNS must be an odd integer >= 3" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
raw="$OUT_DIR/raw.tsv"
summary="$OUT_DIR/summary.tsv"
: > "$raw"

for ((run = 1; run <= RUNS; ++run)); do
    echo "G1 measured run $run/$RUNS" >&2
    "$BENCH" \
      | awk -v run="$run" 'BEGIN{OFS="\t"} NR>1 {print run,$1,$2,$3,$4,$5}' \
      >> "$raw"
done

median_for() {
    local candidate="$1"
    local column="$2"
    awk -v c="$candidate" -v col="$column" '$2==c {print $col}' "$raw" \
      | sort -n \
      | awk '{v[NR]=$1} END {if (NR == 0) exit 1; printf "%.6f", v[(NR+1)/2]}'
}

printf 'candidate\tadd_ns\tmul_ns\tdiv_ns\taffine_ns\tscore_ns\n' > "$summary"
for candidate in Q16.16 Q26.6 Q32.32 Q24.40; do
    add="$(median_for "$candidate" 3)"
    mul="$(median_for "$candidate" 4)"
    div="$(median_for "$candidate" 5)"
    affine="$(median_for "$candidate" 6)"
    score="$(awk -v a="$add" -v m="$mul" -v d="$div" -v f="$affine" 'BEGIN{printf "%.6f", a+m+d+f}')"
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$candidate" "$add" "$mul" "$div" "$affine" "$score" >> "$summary"
done

cat "$summary"
echo "geometry_precision_benchmark_raw=$raw"
echo "geometry_precision_benchmark_summary=$summary"
