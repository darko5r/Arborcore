#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
PROFILE="$(arborcore_perf_profile)"
BASELINE="$(arborcore_perf_baseline_path "$ROOT")"

if [[ ! -r "$BASELINE" ]]; then
    echo "No server performance profile found: $BASELINE" >&2
    echo "Create/accept profile: $PROFILE" >&2
    exit 2
fi

# shellcheck disable=SC1090
source "$BASELINE"

if [[ -n "${PERFORMANCE_PROFILE:-}" && "$PERFORMANCE_PROFILE" != "$PROFILE" ]]; then
    echo "FAIL: profile mismatch: file=$PERFORMANCE_PROFILE requested=$PROFILE" >&2
    exit 2
fi

current_arch="$(arborcore_architecture)"
current_model="$(arborcore_cpu_model)"
current_driver="$(arborcore_scaling_driver)"

if [[ -n "${ARCHITECTURE:-}" && "$current_arch" != "$ARCHITECTURE" ]]; then
    echo "FAIL: architecture changed: baseline=$ARCHITECTURE current=$current_arch" >&2
    exit 2
fi
if [[ -n "${CPU_MODEL:-}" && "$current_model" != "$CPU_MODEL" ]]; then
    echo "FAIL: CPU model changed: baseline=$CPU_MODEL current=$current_model" >&2
    exit 2
fi
if [[ -n "${CPU_SCALING_DRIVER:-}" && "$current_driver" != "$CPU_SCALING_DRIVER" ]]; then
    echo "FAIL: CPU scaling driver changed: baseline=$CPU_SCALING_DRIVER current=$current_driver" >&2
    exit 2
fi

export ARBORCORE_BENCH_RUNS="${ARBORCORE_BENCH_RUNS:-31}"
export ARBORCORE_BENCH_WARMUPS="${ARBORCORE_BENCH_WARMUPS:-3}"
export ARBORCORE_BENCH_OUT_DIR="${ARBORCORE_BENCH_OUT_DIR:-$ROOT/build/server-performance-verify-$PROFILE}"

current_cpu="$(arborcore_bench_cpu)"
if [[ -n "${BENCH_CPU:-}" && "$current_cpu" != "$BENCH_CPU" ]]; then
    echo "FAIL: benchmark CPU changed: baseline=$BENCH_CPU current=$current_cpu" >&2
    exit 2
fi

if [[ -n "${CPU_GOVERNOR:-}" && -r /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
    current_governor="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
    if [[ "$current_governor" != "$CPU_GOVERNOR" ]]; then
        echo "FAIL: CPU governor changed: baseline=$CPU_GOVERNOR current=$current_governor" >&2
        exit 2
    fi
fi

bash "$ROOT/tools/server_benchmark_run.sh"
SUMMARY="$ARBORCORE_BENCH_OUT_DIR/summary.tsv"

failed=0
accepted=0

echo
echo "### Performance profile: $PROFILE"
echo "### Baseline comparison (median ns/op)"
printf '%-30s %14s %14s %12s %10s %10s\n' \
    metric baseline_ns current_ns delta_pct threshold result

while IFS=$'\t' read -r \
    metric samples median mad mad_pct p90 p95 p99 min max ops p95slow
do
    key="$(printf '%s' "$metric" | tr '[:lower:]-' '[:upper:]_')"
    base_var="${key}_MEDIAN_NS_PER_OP"
    threshold_var="${key}_REGRESSION_THRESHOLD_PCT"
    baseline="${!base_var:-}"
    threshold="${!threshold_var:-}"

    if [[ -z "$baseline" ]]; then
        printf '%-30s %14s %14s %12s %10s %10s\n' "$metric" MISSING "$median" n/a n/a SKIP
        continue
    fi

    delta="$(awk -v c="$median" -v b="$baseline" 'BEGIN { if (b == 0) print 0; else printf "%.4f", (c-b)*100/b }')"

    if [[ "${POLICY_STATE:-PROPOSED}" != "ACCEPTED" || -z "$threshold" ]]; then
        printf '%-30s %14s %14s %11s%% %10s %10s\n' "$metric" "$baseline" "$median" "$delta" n/a REPORT
        continue
    fi

    accepted=1
    result=PASS
    if awk -v d="$delta" -v t="$threshold" 'BEGIN { exit !(d > t) }'; then
        result=FAIL
        failed=1
    fi
    printf '%-30s %14s %14s %11s%% %9s%% %10s\n' \
        "$metric" "$baseline" "$median" "$delta" "$threshold" "$result"
done < <(tail -n +2 "$SUMMARY")

if [[ "$accepted" -eq 0 ]]; then
    echo
    echo "Report-only mode: baseline policy is not ACCEPTED."
    exit 0
fi

if [[ "$failed" -ne 0 ]]; then
    echo
    echo "FAIL: one or more median regressions exceeded their accepted metric-specific envelopes." >&2
    exit 1
fi

echo
echo "PASS: all median regressions are within accepted metric-specific envelopes."
