#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
PROFILE="${ARBORCORE_PERF_PROFILE:-local}"
CPU="${ARBORCORE_BENCH_CPU:-0}"
RUNS="${ARBORCORE_C_BRIDGE_RUNS:-31}"
WARMUPS="${ARBORCORE_C_BRIDGE_WARMUPS:-3}"
EXE="$ROOT/build/bench-c-runtime-bridge"
OUT="$ROOT/build/c-runtime-bridge-performance-$PROFILE"
RAW="$OUT/raw.tsv"
SUMMARY="$OUT/summary.tsv"

mkdir -p "$OUT"
printf 'run\tmetric\tns_per_op\n' > "$RAW"

run_once() {
    if command -v taskset >/dev/null 2>&1; then
        taskset -c "$CPU" "$EXE"
    else
        "$EXE"
    fi
}

for ((i=1; i<=WARMUPS; ++i)); do
    run_once >/dev/null
done

for ((i=1; i<=RUNS; ++i)); do
    echo "CR8 measured run $i/$RUNS"
    while IFS=$'\t' read -r metric value; do
        printf '%s\t%s\t%s\n' "$i" "$metric" "$value" >> "$RAW"
    done < <(run_once)
done

median_for() {
    local metric="$1"
    awk -F '\t' -v m="$metric" 'NR>1 && $2==m {print $3}' "$RAW" \
      | sort -n \
      | awk '{a[NR]=$1} END {if (NR==0) exit 1; if (NR%2) printf "%.6f", a[(NR+1)/2]; else printf "%.6f", (a[NR/2]+a[NR/2+1])/2}'
}

metrics=(raw_request wrapper_request raw_response wrapper_response raw_route wrapper_route)
printf 'metric\tmedian_ns_per_op\n' > "$SUMMARY"
for metric in "${metrics[@]}"; do
    printf '%s\t%s\n' "$metric" "$(median_for "$metric")" >> "$SUMMARY"
done

cat "$SUMMARY"

value() { awk -F '\t' -v m="$1" '$1==m {print $2}' "$SUMMARY"; }

compare_pair() {
    local label="$1" raw_metric="$2" wrapper_metric="$3" relative_limit="$4" absolute_limit="$5"
    local raw wrapper
    raw="$(value "$raw_metric")"
    wrapper="$(value "$wrapper_metric")"
    awk -v label="$label" -v raw="$raw" -v wrapper="$wrapper" \
        -v rel="$relative_limit" -v abs="$absolute_limit" 'BEGIN {
        overhead = wrapper - raw
        delta = (raw > 0.0) ? (overhead / raw * 100.0) : 999999.0
        pass = (overhead <= abs || delta <= rel)
        printf "%s raw_ns=%.6f wrapper_ns=%.6f overhead_ns=%.6f delta_pct=%.4f%% rel_limit=%.1f%% abs_limit=%.1fns %s\n", label, raw, wrapper, overhead, delta, rel, abs, (pass ? "PASS" : "FAIL")
        exit(pass ? 0 : 1)
    }'
}

failed=0
compare_pair request raw_request wrapper_request 12.0 8.0 || failed=1
compare_pair response raw_response wrapper_response 12.0 8.0 || failed=1
compare_pair route raw_route wrapper_route 15.0 8.0 || failed=1

if (( failed != 0 )); then
    echo "CR8_BRIDGE_PERFORMANCE_RESULT=REVIEW_REQUIRED"
    exit 1
fi

echo "CR8_BRIDGE_PERFORMANCE_RESULT=PASS"
echo "Bridge policy: pass each metric when median relative overhead is within its initial CR envelope OR absolute wrapper overhead is <=8ns."
