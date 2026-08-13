#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BENCH="${ARBOR_BROWSER_BENCH:-$ROOT/build/browser-surface-bench}"
OUT="$ROOT/build/browser-b6-performance"
RUNS=31
SMALL_LIMIT_NS=16666667
mkdir -p "$OUT"
raw="$OUT/raw.tsv"
summary="$OUT/summary.tsv"
: > "$raw"
printf 'run\texport_mixed_640x360_ns\texport_opaque_1280x720_ns\n' >> "$raw"
for ((i=1; i<=RUNS; ++i)); do
  echo "B6 measured run $i/$RUNS"
  output="$($BENCH)"
  small="$(awk -F= '$1=="export_mixed_640x360_ns" {print $2}' <<<"$output")"
  large="$(awk -F= '$1=="export_opaque_1280x720_ns" {print $2}' <<<"$output")"
  [[ "$small" =~ ^[0-9]+$ && "$large" =~ ^[0-9]+$ ]]
  printf '%d\t%s\t%s\n' "$i" "$small" "$large" >> "$raw"
done
median_col() {
  local col="$1"
  tail -n +2 "$raw" | cut -f"$col" | sort -n | awk 'NR==16 {print; exit}'
}
small_median="$(median_col 2)"
large_median="$(median_col 3)"
printf 'metric\tmedian_ns_per_frame\tresult\n' > "$summary"
if (( small_median <= SMALL_LIMIT_NS )); then small_result=PASS; else small_result=REVIEW; fi
printf 'export_mixed_640x360\t%s\t%s\n' "$small_median" "$small_result" >> "$summary"
printf 'export_opaque_1280x720\t%s\tDIAGNOSTIC\n' "$large_median" >> "$summary"
cat "$summary"
if [[ "$small_result" != PASS ]]; then
  cat > "$OUT/result.env" <<EVIDENCE
B6_BROWSER_EXPORT_PERFORMANCE_RESULT=REVIEW_REQUIRED
EXPORT_IMPLEMENTATION=EXACT_BUCKET12_FROZEN_RENDERER_EQUIVALENT
EXPORT_640X360_60HZ_LIMIT_NS=$SMALL_LIMIT_NS
EXPORT_MIXED_640X360_MEDIAN_NS=$small_median
EXPORT_OPAQUE_1280X720_MEDIAN_NS=$large_median
EVIDENCE
  cat "$OUT/result.env"
  exit 1
fi
cat > "$OUT/result.env" <<EVIDENCE
B6_BROWSER_EXPORT_PERFORMANCE_RESULT=PASS
EXPORT_IMPLEMENTATION=EXACT_BUCKET12_FROZEN_RENDERER_EQUIVALENT
PERFORMANCE_POLICY=MIXED_640X360_WITHIN_60HZ_FRAME_BUDGET
EXPORT_640X360_60HZ_LIMIT_NS=$SMALL_LIMIT_NS
EXPORT_MIXED_640X360_MEDIAN_NS=$small_median
EXPORT_OPAQUE_1280X720_MEDIAN_NS=$large_median
LARGE_EXPORT_STATE=DIAGNOSTIC_BASELINE_ONLY
EVIDENCE
cat "$OUT/result.env"
