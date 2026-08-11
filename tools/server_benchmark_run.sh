#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"

RUNS="${ARBORCORE_BENCH_RUNS:-11}"
WARMUPS="${ARBORCORE_BENCH_WARMUPS:-2}"
CPU="$(arborcore_bench_cpu)"
PROFILE="$(arborcore_perf_profile)"
OUT_DIR="${ARBORCORE_BENCH_OUT_DIR:-$ROOT/build/server-benchmark-$PROFILE}"

if ! [[ "$RUNS" =~ ^[1-9][0-9]*$ ]] || ! [[ "$WARMUPS" =~ ^[0-9]+$ ]]; then
    echo "Invalid benchmark run/warmup count" >&2
    exit 2
fi

production_tree_state=clean
if ! git -C "$ROOT" diff --quiet -- src/asm \
   || ! git -C "$ROOT" diff --cached --quiet -- src/asm \
   || [[ -n "$(git -C "$ROOT" ls-files --others --exclude-standard -- src/asm)" ]]
then
    production_tree_state=dirty
fi

if [[ "$production_tree_state" == dirty \
      && "${ARBORCORE_BENCH_ALLOW_DIRTY_PRODUCTION:-0}" != 1 ]]
then
    echo "FAIL: production src/asm has local changes." >&2
    echo "Committed/release verification requires a clean production tree." >&2
    echo "For deliberate pre-commit construction qualification use:" >&2
    echo "  make ARBORCORE_PERF_PROFILE=<profile> verify-server-performance-candidate" >&2
    exit 2
fi

if [[ "$production_tree_state" == dirty ]]; then
    echo "NOTICE: candidate mode allows dirty production; exact source identity will be recorded."
fi

bins=(
    bench-parser
    bench-routing
    bench-response
    bench-lifecycle
    bench-connections
    bench-loopback
)

for bin in "${bins[@]}"; do
    if [[ ! -x "$ROOT/build/$bin" ]]; then
        echo "Missing benchmark executable: build/$bin" >&2
        exit 2
    fi
done

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
RAW="$OUT_DIR/raw.tsv"
SUMMARY="$OUT_DIR/summary.tsv"
ENVFILE="$OUT_DIR/environment.txt"

{
    echo "timestamp=$(date --iso-8601=seconds)"
    echo "git_head=$(git -C "$ROOT" rev-parse HEAD)"
    echo "performance_profile=$PROFILE"
    echo "git_branch=$(git -C "$ROOT" branch --show-current)"
    echo "bench_cpu=$CPU"
    echo "runs=$RUNS"
    echo "warmups=$WARMUPS"
    echo "production_text_bytes=$(arborcore_production_text_bytes "$ROOT")"
    echo "production_tree_state=$production_tree_state"
    echo "production_source_sha256=$(arborcore_production_source_sha256 "$ROOT")"
    echo "kernel=$(uname -srmo)"
    echo "architecture=$(arborcore_architecture)"
    echo "cpu_model=$(arborcore_cpu_model)"
    echo "cpu_scaling_driver=$(arborcore_scaling_driver)"
    echo "nasm=$(nasm -v 2>/dev/null || true)"
    echo "ld=$(ld --version | head -n 1)"
    echo "make=$(make --version | head -n 1)"
    lscpu | grep -E '^(Architecture|CPU\(s\)|Model name|Thread|Core|Socket|CPU max MHz|CPU min MHz):' || true
    for f in /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver; do
        if [[ -r "$f" ]]; then
            printf '%s=' "$f"
            cat "$f"
        fi
    done
} > "$ENVFILE"

printf 'metric\trun\titerations\ttotal_ns\tns_per_op\tops_per_sec\n' > "$RAW"

echo "### Arborcore server benchmark"
echo "profile=$PROFILE CPU=$CPU runs=$RUNS warmups=$WARMUPS"
echo

for bin in "${bins[@]}"; do
    echo "Warmup: $bin"
    for ((w=1; w<=WARMUPS; w++)); do
        arborcore_run_pinned "$CPU" "$ROOT/build/$bin" >/dev/null
    done
done

echo
for ((run=1; run<=RUNS; run++)); do
    echo "Measured run $run/$RUNS"
    for bin in "${bins[@]}"; do
        while IFS=$'\t' read -r metric iterations total_ns extra; do
            [[ -z "$metric" ]] && continue
            if [[ -n "${extra:-}" ]] || ! [[ "$iterations" =~ ^[0-9]+$ ]] || ! [[ "$total_ns" =~ ^[0-9]+$ ]]; then
                echo "Malformed result from $bin: $metric $iterations $total_ns ${extra:-}" >&2
                exit 3
            fi
            ns_per_op="$(awk -v n="$total_ns" -v i="$iterations" 'BEGIN { printf "%.6f", n / i }')"
            ops_per_sec="$(awk -v x="$ns_per_op" 'BEGIN { if (x == 0) print 0; else printf "%.3f", 1000000000 / x }')"
            printf '%s\t%d\t%s\t%s\t%s\t%s\n' \
                "$metric" "$run" "$iterations" "$total_ns" "$ns_per_op" "$ops_per_sec" >> "$RAW"
        done < <(arborcore_run_pinned "$CPU" "$ROOT/build/$bin")
    done
done

# These are percentiles of repeated whole-benchmark RUNS, not per-request latency.
# Median and MAD are the qualification statistics; run-p90/p95/p99 remain diagnostics.
printf 'metric\tsamples\tmedian_ns_per_op\tmad_ns_per_op\tmad_pct\trun_p90_ns_per_op\trun_p95_ns_per_op\trun_p99_ns_per_op\tmin_ns_per_op\tmax_ns_per_op\tmedian_ops_per_sec\trun_p95_slow_pct\n' > "$SUMMARY"

median_of_sorted_file() {
    awk '
        { a[NR] = $1 }
        END {
            n = NR
            if (n == 0) exit 1
            if (n % 2) printf "%.6f", a[(n + 1) / 2]
            else printf "%.6f", (a[n / 2] + a[n / 2 + 1]) / 2
        }
    ' "$1"
}

rank_value() {
    local file="$1"
    local pct="$2"
    awk -v p="$pct" '
        { a[NR] = $1 }
        END {
            n = NR
            if (n == 0) exit 1
            idx = int(p * n)
            if (idx < p * n) idx++
            if (idx < 1) idx = 1
            if (idx > n) idx = n
            printf "%.6f", a[idx]
        }
    ' "$file"
}

while IFS= read -r metric; do
    values="$OUT_DIR/.values.$$"
    devs="$OUT_DIR/.devs.$$"

    awk -F '\t' -v m="$metric" 'NR > 1 && $1 == m { print $5 }' "$RAW" | sort -n > "$values"

    median="$(median_of_sorted_file "$values")"
    awk -v med="$median" '{ d=$1-med; if (d<0) d=-d; printf "%.6f\n", d }' "$values" | sort -n > "$devs"
    mad="$(median_of_sorted_file "$devs")"

    samples="$(wc -l < "$values" | tr -d ' ')"
    p90="$(rank_value "$values" 0.90)"
    p95="$(rank_value "$values" 0.95)"
    p99="$(rank_value "$values" 0.99)"
    min="$(head -n 1 "$values")"
    max="$(tail -n 1 "$values")"
    mad_pct="$(awk -v m="$median" -v d="$mad" 'BEGIN { if (m == 0) print 0; else printf "%.4f", 100*d/m }')"
    ops="$(awk -v m="$median" 'BEGIN { if (m == 0) print 0; else printf "%.3f", 1000000000/m }')"
    p95_slow="$(awk -v m="$median" -v p="$p95" 'BEGIN { if (m == 0) print 0; else printf "%.4f", 100*(p-m)/m }')"

    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$metric" "$samples" "$median" "$mad" "$mad_pct" \
        "$p90" "$p95" "$p99" "$min" "$max" "$ops" "$p95_slow" >> "$SUMMARY"

    rm -f "$values" "$devs"
done < <(awk -F '\t' 'NR > 1 { print $1 }' "$RAW" | sort -u)

echo
echo "### Summary"
column -t -s $'\t' "$SUMMARY" 2>/dev/null || cat "$SUMMARY"
echo
echo "Raw:     $RAW"
echo "Summary: $SUMMARY"
echo "Env:     $ENVFILE"
