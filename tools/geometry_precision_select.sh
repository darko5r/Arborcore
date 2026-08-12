#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
SUMMARY="${ARBOR_GEOMETRY_BENCH_SUMMARY:-$ROOT/build/geometry-precision-g0-g1/summary.tsv}"
OUT="${ARBOR_GEOMETRY_SELECTION_OUT:-$ROOT/build/geometry-precision-g0-g1/selection.env}"

if [[ ! -s "$SUMMARY" ]]; then
    echo "FAIL: geometry benchmark summary missing: $SUMMARY" >&2
    exit 1
fi

q32_score="$(awk '$1=="Q32.32" {print $6}' "$SUMMARY")"
q24_score="$(awk '$1=="Q24.40" {print $6}' "$SUMMARY")"
if [[ -z "$q32_score" || -z "$q24_score" ]]; then
    echo "FAIL: eligible-candidate scores missing" >&2
    exit 1
fi

q32_vs_q24_pct="$(awk -v q32="$q32_score" -v q24="$q24_score" 'BEGIN{printf "%.4f", ((q32/q24)-1.0)*100.0}')"
q32_within="$(awk -v d="$q32_vs_q24_pct" 'BEGIN{print (d <= 15.0) ? 1 : 0}')"

mkdir -p "$(dirname "$OUT")"
if [[ "$q32_within" == 1 ]]; then
    decision="RECOMMEND_Q32_32_FOR_G2_G4_REVIEW"
    reason="Q32.32 meets the G0 range/precision requirements and stays within 15% of Q24.40 on the same-host median arithmetic score while providing 256x more integer range."
else
    decision="REVIEW_REQUIRED_Q32_32_PERFORMANCE"
    reason="Q32.32 meets the G0 range/precision requirements but exceeds the 15% same-host score review threshold versus Q24.40."
fi

cat > "$OUT" <<EVIDENCE
G0_MIN_INTEGER_RANGE=1048576
G0_MIN_FRACTION_BITS=16
G1_ELIGIBLE_CANDIDATES=Q32.32,Q24.40
Q32_32_SCORE_NS=$q32_score
Q24_40_SCORE_NS=$q24_score
Q32_32_VS_Q24_40_SCORE_DELTA_PCT=$q32_vs_q24_pct
G1_SELECTION_DECISION=$decision
G1_SELECTION_REASON='$reason'
G1_NUMERICAL_CONTRACT_STATE=UNFROZEN_EXPERIMENT
EVIDENCE

cat "$OUT"

if [[ "$q32_within" != 1 ]]; then
    exit 2
fi
