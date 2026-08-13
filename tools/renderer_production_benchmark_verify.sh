#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BENCH="${ARBOR_RENDERER_PRODUCTION_BENCH:-$ROOT/build/renderer-production-bench}"
OUT="$ROOT/build/renderer-r9-performance"
RAW="$OUT/raw.tsv"
SUMMARY="$OUT/summary.tsv"
RESULT="$OUT/result.env"
RUNS=31
mkdir -p "$OUT"
printf 'run\tcandidate_clear\tproduction_clear\tcandidate_rect\tproduction_rect\tline\tpath\n' > "$RAW"

for ((run=1; run<=RUNS; ++run)); do
  echo "R9 measured run $run/$RUNS"
  output="$($BENCH)"
  candidate_clear=""; production_clear=""; candidate_rect=""; production_rect=""; line=""; path=""; operand_mode=""
  while IFS='=' read -r key value; do
    case "$key" in
      benchmark_operand_mode) operand_mode="$value" ;;
      candidate_clear_ns) candidate_clear="$value" ;;
      production_clear_ns) production_clear="$value" ;;
      candidate_rect_ns) candidate_rect="$value" ;;
      production_rect_ns) production_rect="$value" ;;
      production_line_ns) line="$value" ;;
      production_path_ns) path="$value" ;;
    esac
  done <<< "$output"
  [[ "$operand_mode" == "runtime_nonconstant_shared_surface_rect_color" ]]
  [[ -n "$candidate_clear" && -n "$production_clear" && -n "$candidate_rect" && -n "$production_rect" && -n "$line" && -n "$path" ]]
  printf '%d\t%s\t%s\t%s\t%s\t%s\t%s\n' "$run" "$candidate_clear" "$production_clear" "$candidate_rect" "$production_rect" "$line" "$path" >> "$RAW"
done

median_column() {
  local column="$1"
  tail -n +2 "$RAW" | cut -f"$column" | sort -n | awk 'NR==16 {print; exit}'
}

cclear="$(median_column 2)"
pclear="$(median_column 3)"
crect="$(median_column 4)"
prect="$(median_column 5)"
line="$(median_column 6)"
path="$(median_column 7)"

printf 'metric\tcandidate_ns\tproduction_ns\toverhead_ns\tdelta_pct\tresult\n' > "$SUMMARY"
check_metric() {
  local name="$1" candidate="$2" production="$3" abs_limit="$4"
  awk -v n="$name" -v c="$candidate" -v p="$production" -v a="$abs_limit" 'BEGIN {
    o=p-c; d=(c==0 ? 0 : (o/c)*100.0); pass=(p <= c*1.50 || o <= a);
    printf "%s\t%.6f\t%.6f\t%.6f\t%.4f\t%s\n", n,c,p,o,d,(pass?"PASS":"REVIEW");
    exit(pass?0:1)
  }' >> "$SUMMARY"
}

status=0
check_metric clear "$cclear" "$pclear" 500.0 || status=1
check_metric fractional_rect "$crect" "$prect" 4000.0 || status=1
printf 'line_diagnostic\t-\t%s\t-\t-\tDIAGNOSTIC\n' "$line" >> "$SUMMARY"
printf 'path_diagnostic\t-\t%s\t-\t-\tDIAGNOSTIC\n' "$path" >> "$SUMMARY"
cat "$SUMMARY"

if (( status != 0 )); then
  cat > "$RESULT" <<EVIDENCE
R9_RENDERER_PERFORMANCE_RESULT=REVIEW_REQUIRED
PERFORMANCE_POLICY=CLEAR_RECT_REL50_OR_ABS_BUDGET
BENCHMARK_OPERAND_MODE=RUNTIME_NONCONSTANT_SHARED_SURFACE_RECT_COLOR
CLEAR_ABS_LIMIT_NS=500
RECT_ABS_LIMIT_NS=4000
LINE_PATH_STATE=DIAGNOSTIC_BASELINE_ONLY
EVIDENCE
  cat "$RESULT"
  exit 1
fi

cat > "$RESULT" <<EVIDENCE
R9_RENDERER_PERFORMANCE_RESULT=PASS
PERFORMANCE_POLICY=CLEAR_RECT_REL50_OR_ABS_BUDGET
BENCHMARK_OPERAND_MODE=RUNTIME_NONCONSTANT_SHARED_SURFACE_RECT_COLOR
CLEAR_ABS_LIMIT_NS=500
RECT_ABS_LIMIT_NS=4000
LINE_PATH_STATE=DIAGNOSTIC_BASELINE_ESTABLISHED
LINE_MEDIAN_NS=$line
PATH_MEDIAN_NS=$path
EVIDENCE
cat "$RESULT"
