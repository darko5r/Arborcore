#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
PROFILE="$(arborcore_perf_profile)"
export ARBORCORE_WRITEV_OUT_DIR="${ARBORCORE_WRITEV_OUT_DIR:-$ROOT/build/writev-experiment-$PROFILE}"
"$ROOT/build/core-writev-experiment-test"
bash "$ROOT/tools/writev_experiment_run.sh"
SUMMARY="$ARBORCORE_WRITEV_OUT_DIR/summary.tsv"
value(){ awk -F '\t' -v m="$1" 'NR>1&&$1==m{print $3;exit}' "$SUMMARY"; }
pct(){ awk -v c="$1" -v b="$2" 'BEGIN{printf "%.4f",100*(c-b)/b}'; }
for suffix in empty body128 body1024; do
  s="$(value serialize_$suffix)"; i="$(value iovec_$suffix)"
  d="$(pct "$i" "$s")"
  printf 'E8 %-8s serialized_write_ns=%s writev_ns=%s emit_delta=%s%%\n' "$suffix" "$s" "$i" "$d"
done
echo "E8_DECISION=RETAIN_SERIALIZED_BUFFER_FOUNDATION"
echo "Reason: writev emission is measured, but production would need persistent per-connection iovec/digit-scratch lifetime state across EAGAIN."
echo "The experimental emitter has resumable mutated-iovec progress semantics; production remains contiguous-buffer based until that state/lifetime cost is qualified in the post-ABI response layer."
