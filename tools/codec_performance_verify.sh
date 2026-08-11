#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
PROFILE="$(arborcore_perf_profile)"
BASELINE="$ROOT/generated/performance/codec-$PROFILE.env"

[[ -r "$BASELINE" ]] || { echo "No codec performance baseline found: $BASELINE" >&2; echo "Run: make ARBORCORE_PERF_PROFILE=$PROFILE qualify-codec-baseline" >&2; exit 2; }
# shellcheck disable=SC1090
source "$BASELINE"

[[ "${PERFORMANCE_PROFILE:-}" == "$PROFILE" ]] || { echo "FAIL: codec profile mismatch" >&2; exit 2; }
current_model="$(arborcore_cpu_model)"
current_driver="$(arborcore_scaling_driver)"
[[ -z "${CPU_MODEL:-}" || "$current_model" == "$CPU_MODEL" ]] || { echo "FAIL: CPU model changed" >&2; exit 2; }
[[ -z "${CPU_SCALING_DRIVER:-}" || "$current_driver" == "$CPU_SCALING_DRIVER" ]] || { echo "FAIL: CPU scaling driver changed" >&2; exit 2; }
current_cpu="$(arborcore_bench_cpu)"
[[ -z "${BENCH_CPU:-}" || "$current_cpu" == "$BENCH_CPU" ]] || { echo "FAIL: benchmark CPU changed" >&2; exit 2; }
if [[ -n "${CPU_GOVERNOR:-}" && -r /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
    [[ "$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)" == "$CPU_GOVERNOR" ]] || { echo "FAIL: CPU governor changed" >&2; exit 2; }
fi

export ARBORCORE_CODEC_RUNS="${ARBORCORE_CODEC_RUNS:-31}"
export ARBORCORE_CODEC_WARMUPS="${ARBORCORE_CODEC_WARMUPS:-3}"
export ARBORCORE_CODEC_OUT_DIR="${ARBORCORE_CODEC_OUT_DIR:-$ROOT/build/codec-performance-verify-$PROFILE}"

bash "$ROOT/tools/codec_benchmark_run.sh"
SUMMARY="$ARBORCORE_CODEC_OUT_DIR/summary.tsv"
current_percent_text="$(size "$ROOT/build/percent_codec.o" | awk 'NR==2{print $1}')"
current_prod_text="$(arborcore_production_text_bytes "$ROOT")"

percent_delta=$(( current_percent_text - PERCENT_CODEC_TEXT_BYTES ))
prod_delta=$(( current_prod_text - PRODUCTION_TEXT_BYTES ))

echo
echo "### Codec quality-vector size evidence"
printf 'percent_codec_text_bytes baseline=%s current=%s delta=%+d\n' "$PERCENT_CODEC_TEXT_BYTES" "$current_percent_text" "$percent_delta"
printf 'production_text_bytes    baseline=%s current=%s delta=%+d\n' "$PRODUCTION_TEXT_BYTES" "$current_prod_text" "$prod_delta"

echo
echo "### Codec reference comparison (median ns/op)"
printf '%-30s %14s %14s %12s %10s %12s\n' metric baseline_ns current_ns delta_pct threshold result

review=0
while IFS=$'\t' read -r metric samples median mad mad_pct p90 p95 p99 min max ops p95slow; do
    key="$(printf '%s' "$metric" | tr '[:lower:]-' '[:upper:]_')"
    base_var="${key}_MEDIAN_NS_PER_OP"
    threshold_var="${key}_REGRESSION_THRESHOLD_PCT"
    baseline="${!base_var:-}"
    threshold="${!threshold_var:-}"
    [[ -n "$baseline" && -n "$threshold" ]] || { printf '%-30s %14s %14s %12s %10s %12s\n' "$metric" MISSING "$median" n/a n/a SKIP; continue; }
    delta="$(awk -v c="$median" -v b="$baseline" 'BEGIN{if(b==0)print 0;else printf "%.4f",100*(c-b)/b}')"
    result=PASS
    if awk -v d="$delta" -v t="$threshold" 'BEGIN{exit !(d>t)}'; then
        result=REVIEW
        review=1
    fi
    printf '%-30s %14s %14s %11s%% %9s%% %12s\n' "$metric" "$baseline" "$median" "$delta" "$threshold" "$result"
done < <(tail -n +2 "$SUMMARY")

if [[ "$review" -ne 0 ]]; then
    echo
    echo "REVIEW REQUIRED: one or more codec medians exceeded the reference envelope." >&2
    echo "This is quality-tradeoff evidence, not an automatic architectural rejection." >&2
    exit 1
fi

echo
echo "PASS: codec medians are within reference envelopes; review size/quality evidence separately."
