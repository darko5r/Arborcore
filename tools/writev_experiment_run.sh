#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
RUNS="${ARBORCORE_WRITEV_RUNS:-31}"
WARMUPS="${ARBORCORE_WRITEV_WARMUPS:-3}"
CPU="$(arborcore_bench_cpu)"
PROFILE="$(arborcore_perf_profile)"
OUT_DIR="${ARBORCORE_WRITEV_OUT_DIR:-$ROOT/build/writev-experiment-$PROFILE}"
BIN="$ROOT/build/bench-writev-experiment"
rm -rf "$OUT_DIR"; mkdir -p "$OUT_DIR"
RAW="$OUT_DIR/raw.tsv"; SUMMARY="$OUT_DIR/summary.tsv"; ENVFILE="$OUT_DIR/environment.txt"
{
  echo "timestamp=$(date --iso-8601=seconds)"
  echo "performance_profile=$PROFILE"
  echo "bench_cpu=$CPU"
  echo "runs=$RUNS"
  echo "warmups=$WARMUPS"
  echo "candidate_text_bytes=$(size "$ROOT/build/response_iovec_candidate.o" | awk 'NR==2{print $1}')"
} > "$ENVFILE"
printf 'metric\trun\titerations\ttotal_ns\tns_per_op\n' > "$RAW"
for ((w=1;w<=WARMUPS;w++)); do arborcore_run_pinned "$CPU" "$BIN" >/dev/null; done
for ((run=1;run<=RUNS;run++)); do
  echo "E8 measured run $run/$RUNS"
  while IFS=$'\t' read -r metric iterations total_ns extra; do
    [[ -z "$metric" ]] && continue
    [[ -z "${extra:-}" ]] || { echo "Malformed E8 result" >&2; exit 3; }
    ns="$(awk -v n="$total_ns" -v i="$iterations" 'BEGIN{printf "%.6f",n/i}')"
    printf '%s\t%d\t%s\t%s\t%s\n' "$metric" "$run" "$iterations" "$total_ns" "$ns" >> "$RAW"
  done < <(arborcore_run_pinned "$CPU" "$BIN")
done
printf 'metric\tsamples\tmedian_ns_per_op\tmad_ns_per_op\tmad_pct\n' > "$SUMMARY"
median_sorted(){ awk '{a[NR]=$1} END{n=NR;if(!n)exit 1;if(n%2)printf "%.6f",a[(n+1)/2];else printf "%.6f",(a[n/2]+a[n/2+1])/2}' "$1"; }
while IFS= read -r metric; do
  vals="$OUT_DIR/.v.$$"; devs="$OUT_DIR/.d.$$"
  awk -F '\t' -v m="$metric" 'NR>1&&$1==m{print $5}' "$RAW" | sort -n > "$vals"
  med="$(median_sorted "$vals")"
  awk -v m="$med" '{d=$1-m;if(d<0)d=-d;printf "%.6f\n",d}' "$vals" | sort -n > "$devs"
  mad="$(median_sorted "$devs")"; samples="$(wc -l < "$vals" | tr -d ' ')"
  mad_pct="$(awk -v m="$med" -v d="$mad" 'BEGIN{if(m==0)print 0;else printf "%.4f",100*d/m}')"
  printf '%s\t%s\t%s\t%s\t%s\n' "$metric" "$samples" "$med" "$mad" "$mad_pct" >> "$SUMMARY"
  rm -f "$vals" "$devs"
done < <(awk -F '\t' 'NR>1{print $1}' "$RAW" | sort -u)
echo; echo "### E8 serialized-write vs writev summary"
column -t -s $'\t' "$SUMMARY" 2>/dev/null || cat "$SUMMARY"
echo "Summary: $SUMMARY"
