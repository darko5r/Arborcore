#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
PROFILE="$(arborcore_perf_profile)"
export ARBORCORE_ROUTE_INDEX_OUT_DIR="${ARBORCORE_ROUTE_INDEX_OUT_DIR:-$ROOT/build/route-index-experiment-$PROFILE}"

"$ROOT/build/core-route-index-experiment-test"
bash "$ROOT/tools/route_index_experiment_run.sh"

SUMMARY="$ARBORCORE_ROUTE_INDEX_OUT_DIR/summary.tsv"
ENVFILE="$ARBORCORE_ROUTE_INDEX_OUT_DIR/environment.txt"

metric_value() {
    awk -F '\t' -v m="$1" 'NR>1 && $1==m{print $3;exit}' "$SUMMARY"
}

linear_first="$(metric_value linear_first)"
index_first="$(metric_value index_first)"
linear_last="$(metric_value linear_last)"
index_last="$(metric_value index_last)"
linear_miss="$(metric_value linear_miss)"
index_miss="$(metric_value index_miss)"

pct() {
    awk -v c="$1" -v b="$2" 'BEGIN{if(b==0)print 0;else printf "%.4f",100*(c-b)/b}'
}

d_first="$(pct "$index_first" "$linear_first")"
d_last="$(pct "$index_last" "$linear_last")"
d_miss="$(pct "$index_miss" "$linear_miss")"

candidate_text="$(awk -F= '$1=="route_index_candidate_text_bytes"{print $2}' "$ENVFILE")"
router_text="$(awk -F= '$1=="router_text_bytes"{print $2}' "$ENVFILE")"

echo
echo "### D4 prepared/static route-index decision"
printf '%-12s %14s %14s %12s\n' case linear_ns index_ns delta_pct
printf '%-12s %14s %14s %11s%%\n' first "$linear_first" "$index_first" "$d_first"
printf '%-12s %14s %14s %11s%%\n' last  "$linear_last"  "$index_last"  "$d_last"
printf '%-12s %14s %14s %11s%%\n' miss  "$linear_miss"  "$index_miss"  "$d_miss"
printf 'router_text_bytes=%s\n' "$router_text"
printf 'route_index_candidate_text_bytes=%s\n' "$candidate_text"

# Promotion is intentionally strict. The prepared candidate must not make any
# measured lookup case more than 3%% slower, and it must improve last or miss
# by at least 5%%. Otherwise the ordered linear router remains authoritative.
promote=1
for d in "$d_first" "$d_last" "$d_miss"; do
    if awk -v x="$d" 'BEGIN{exit !(x>3.0)}'; then promote=0; fi
done
if ! awk -v a="$d_last" -v b="$d_miss" 'BEGIN{exit !((a<=-5.0)||(b<=-5.0))}'; then
    promote=0
fi

if [[ "$promote" -eq 1 ]]; then
    echo "D4_DECISION=ADMIT_PREPARED_INDEX_CANDIDATE"
    echo "NOTICE: candidate earned promotion evidence; do not silently replace the production router."
    echo "A source-level promotion must preserve this exact equivalence test and be reviewed before commit."
else
    echo "D4_DECISION=RETAIN_ORDERED_LINEAR_REFERENCE"
    echo "PASS: experiment completed; production router remains unchanged."
fi

# Either evidence-based outcome completes the experiment successfully.
exit 0
