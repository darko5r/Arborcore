#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
BENCH="${ARBOR_GEOMETRY_PRODUCTION_BENCH:-$ROOT/build/geometry-production-bench}"
OUT="$ROOT/build/geometry-g2-g4-performance"
RAW="$OUT/raw.tsv"
SUMMARY="$OUT/summary.tsv"
RUNS=31

mkdir -p "$OUT"
echo "benchmark_operand_mode=runtime_nonconstant_shared_inputs"
printf 'run\tcandidate_add\tproduction_add\tcandidate_mul\tproduction_mul\tcandidate_div\tproduction_div\tcandidate_affine\tproduction_affine\n' > "$RAW"

field() {
  local name="$1"
  local text="$2"
  awk -F '\t' -v n="$name" '$1==n {print $2}' <<<"$text"
}

for ((i=1; i<=RUNS; ++i)); do
  echo "G2-G4 measured run $i/$RUNS"
  if (( i % 2 == 0 )); then
    output="$($BENCH production-first)"
  else
    output="$($BENCH candidate-first)"
  fi

  ca="$(field candidate_add_ns "$output")"
  pa="$(field production_add_ns "$output")"
  cm="$(field candidate_mul_ns "$output")"
  pm="$(field production_mul_ns "$output")"
  cd="$(field candidate_div_ns "$output")"
  pd="$(field production_div_ns "$output")"
  cf="$(field candidate_affine_ns "$output")"
  pf="$(field production_affine_ns "$output")"
  for value in "$ca" "$pa" "$cm" "$pm" "$cd" "$pd" "$cf" "$pf"; do
    [[ -n "$value" ]] || { echo "FAIL: malformed geometry benchmark output" >&2; exit 1; }
  done
  printf '%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$i" "$ca" "$pa" "$cm" "$pm" "$cd" "$pd" "$cf" "$pf" >> "$RAW"
done

median_col() {
  local col="$1"
  tail -n +2 "$RAW" | cut -f"$col" | sort -n | sed -n '16p'
}

metric_row() {
  local name="$1" candidate="$2" production="$3"
  local overhead delta result
  overhead="$(awk -v p="$production" -v c="$candidate" 'BEGIN {printf "%.6f", p-c}')"
  delta="$(awk -v p="$production" -v c="$candidate" 'BEGIN {printf "%.4f", ((p-c)/c)*100.0}')"
  result="$(awk -v d="$delta" -v o="$overhead" 'BEGIN {if (d <= 15.0 || o <= 8.0) print "PASS"; else print "REVIEW"}')"
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$name" "$candidate" "$production" "$overhead" "$delta" "$result" >> "$SUMMARY"
}

ca="$(median_col 2)"; pa="$(median_col 3)"
cm="$(median_col 4)"; pm="$(median_col 5)"
cd="$(median_col 6)"; pd="$(median_col 7)"
cf="$(median_col 8)"; pf="$(median_col 9)"
ct="$(awk -v a="$ca" -v m="$cm" -v d="$cd" -v f="$cf" 'BEGIN {printf "%.6f", a+m+d+f}')"
pt="$(awk -v a="$pa" -v m="$pm" -v d="$pd" -v f="$pf" 'BEGIN {printf "%.6f", a+m+d+f}')"

printf 'metric\tcandidate_ns\tproduction_ns\toverhead_ns\tdelta_pct\tresult\n' > "$SUMMARY"
metric_row add "$ca" "$pa"
metric_row mul "$cm" "$pm"
metric_row div "$cd" "$pd"
metric_row affine_fused "$cf" "$pf"
metric_row total "$ct" "$pt"
cat "$SUMMARY"

if awk -F '\t' 'NR>1 && $1!="total" && $6!="PASS" {bad=1} END {exit bad ? 0 : 1}' "$SUMMARY"; then
  echo "G2_G4_GEOMETRY_PERFORMANCE_RESULT=REVIEW_REQUIRED" >&2
  echo "Policy: every equivalent-work component must be <=15% slower OR add <=8ns median absolute overhead." >&2
  exit 1
fi

cat > "$OUT/result.env" <<EVIDENCE
G2_G4_GEOMETRY_PERFORMANCE_RESULT=PASS
PERFORMANCE_POLICY=PER_COMPONENT_REL15_OR_ABS8
BENCHMARK_OPERAND_MODE=RUNTIME_NONCONSTANT_SHARED_INPUTS
CANDIDATE_TOTAL_NS=$ct
PRODUCTION_TOTAL_NS=$pt
EVIDENCE
echo "G2_G4_GEOMETRY_PERFORMANCE_RESULT=PASS"
