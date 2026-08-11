#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"

ROUNDS="${ARBORCORE_BASELINE_ROUNDS:-3}"
PROFILE="$(arborcore_perf_profile)"
BASELINE="$(arborcore_perf_baseline_path "$ROOT")"

if ! [[ "$ROUNDS" =~ ^[0-9]+$ ]] || (( ROUNDS < 3 )); then
    echo "ARBORCORE_BASELINE_ROUNDS must be an integer >= 3" >&2
    exit 2
fi

if [[ -n "${ARBORCORE_BASELINE_REPEATABILITY_PREFIX:-}" ]]; then
    PREFIX="$ARBORCORE_BASELINE_REPEATABILITY_PREFIX"
else
    profile_prefix="$ROOT/build/server-repeatability-$PROFILE"
    legacy_prefix="$ROOT/build/server-repeatability"
    if [[ -r "${profile_prefix}-1/summary.tsv" ]]; then
        PREFIX="$profile_prefix"
    elif [[ -r "${legacy_prefix}-1/summary.tsv" ]]; then
        # Compatibility with the three rounds collected before profile support.
        PREFIX="$legacy_prefix"
    else
        PREFIX="$profile_prefix"
    fi
fi

summaries=()
envs=()
for ((r=1; r<=ROUNDS; r++)); do
    s="${PREFIX}-${r}/summary.tsv"
    e="${PREFIX}-${r}/environment.txt"
    if [[ ! -r "$s" || ! -r "$e" ]]; then
        echo "Missing repeatability round $r: $s or $e" >&2
        exit 2
    fi
    summaries+=("$s")
    envs+=("$e")
done

# Require identical benchmark identity/environment across accepted rounds.
first_head="$(awk -F= '$1=="git_head"{print substr($0,index($0,"=")+1)}' "${envs[0]}")"
first_cpu="$(awk -F= '$1=="bench_cpu"{print $2}' "${envs[0]}")"
first_runs="$(awk -F= '$1=="runs"{print $2}' "${envs[0]}")"
first_warmups="$(awk -F= '$1=="warmups"{print $2}' "${envs[0]}")"
first_text="$(awk -F= '$1=="production_text_bytes"{print $2}' "${envs[0]}")"
first_kernel="$(awk -F= '$1=="kernel"{print substr($0,index($0,"=")+1)}' "${envs[0]}")"
first_arch="$(awk -F= '$1=="architecture"{print substr($0,index($0,"=")+1)}' "${envs[0]}" || true)"
first_model="$(awk -F= '$1=="cpu_model"{print substr($0,index($0,"=")+1)}' "${envs[0]}" || true)"
first_driver="$(awk -F= '$1=="cpu_scaling_driver"{print substr($0,index($0,"=")+1)}' "${envs[0]}" || true)"
# Compatibility with the already-collected v2 repeatability rounds.
[[ -n "$first_arch" ]] || first_arch="$(uname -m)"
[[ -n "$first_model" ]] || first_model="$(arborcore_cpu_model)"
[[ -n "$first_driver" ]] || first_driver="$(arborcore_scaling_driver)"
first_governor="$(awk -F= '$1 ~ /scaling_governor$/{print $2}' "${envs[0]}")"

for e in "${envs[@]}"; do
    [[ "$(awk -F= '$1=="git_head"{print substr($0,index($0,"=")+1)}' "$e")" == "$first_head" ]] || { echo "Mismatched git_head: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="bench_cpu"{print $2}' "$e")" == "$first_cpu" ]] || { echo "Mismatched bench_cpu: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="runs"{print $2}' "$e")" == "$first_runs" ]] || { echo "Mismatched runs: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="warmups"{print $2}' "$e")" == "$first_warmups" ]] || { echo "Mismatched warmups: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="production_text_bytes"{print $2}' "$e")" == "$first_text" ]] || { echo "Mismatched production text size: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="kernel"{print substr($0,index($0,"=")+1)}' "$e")" == "$first_kernel" ]] || { echo "Mismatched kernel: $e" >&2; exit 3; }
    round_arch="$(awk -F= '$1=="architecture"{print substr($0,index($0,"=")+1)}' "$e" || true)"
    round_model="$(awk -F= '$1=="cpu_model"{print substr($0,index($0,"=")+1)}' "$e" || true)"
    round_driver="$(awk -F= '$1=="cpu_scaling_driver"{print substr($0,index($0,"=")+1)}' "$e" || true)"
    [[ -z "$round_arch" || "$round_arch" == "$first_arch" ]] || { echo "Mismatched architecture: $e" >&2; exit 3; }
    [[ -z "$round_model" || "$round_model" == "$first_model" ]] || { echo "Mismatched CPU model: $e" >&2; exit 3; }
    [[ -z "$round_driver" || "$round_driver" == "$first_driver" ]] || { echo "Mismatched scaling driver: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1 ~ /scaling_governor$/{print $2}' "$e")" == "$first_governor" ]] || { echo "Mismatched CPU governor: $e" >&2; exit 3; }
done

metric_list="$(awk -F '\t' 'NR>1{print $1}' "${summaries[0]}" | sort)"
for s in "${summaries[@]:1}"; do
    other="$(awk -F '\t' 'NR>1{print $1}' "$s" | sort)"
    [[ "$other" == "$metric_list" ]] || { echo "Mismatched metric set: $s" >&2; exit 3; }
done

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

mkdir -p "$ROOT/generated/performance"

{
    echo '# Arborcore machine-specific accepted server performance baseline.'
    echo '# Accepted from independent repeatability rounds.'
    echo '# Regression statistic: repeated-run MEDIAN ns/op.'
    echo '# Tail percentiles are diagnostic and are not pass/fail criteria.'
    echo 'BASELINE_VERSION=3'
    printf 'PERFORMANCE_PROFILE=%s\n' "$PROFILE"
    echo 'POLICY_STATE=ACCEPTED'
    printf 'PRODUCTION_COMMIT=%s\n' "$first_head"
    printf 'PRODUCTION_TEXT_BYTES=%s\n' "$first_text"
    printf 'BENCH_CPU=%s\n' "$first_cpu"
    printf 'BENCH_RUNS=%s\n' "$first_runs"
    printf 'BENCH_WARMUPS=%s\n' "$first_warmups"
    printf 'KERNEL=%q\n' "$first_kernel"
    printf 'ARCHITECTURE=%s\n' "$first_arch"
    printf 'CPU_MODEL=%q\n' "$first_model"
    printf 'CPU_SCALING_DRIVER=%s\n' "$first_driver"
    printf 'CPU_GOVERNOR=%s\n' "$first_governor"
    printf 'SOURCE_REPEATABILITY_ROUNDS=%s\n' "$ROUNDS"
    printf 'SOURCE_REPEATABILITY_PREFIX=%q\n' "$PREFIX"
    echo 'BASELINE_AGGREGATION=median_of_round_medians'
    echo 'THRESHOLD_RULE=max_5pct__ceil_1.5x_cross_round_full_median_spread_pct'

    while IFS= read -r metric; do
        vals="$tmp/$metric.values"
        : > "$vals"
        for s in "${summaries[@]}"; do
            # Median is column 3 in both v1 and v2 summary formats.
            awk -F '\t' -v m="$metric" 'NR>1 && $1==m{print $3}' "$s" >> "$vals"
        done
        sort -n -o "$vals" "$vals"

        baseline="$(awk '
            {a[NR]=$1}
            END {
                n=NR
                if (n%2) printf "%.6f", a[(n+1)/2]
                else printf "%.6f", (a[n/2]+a[n/2+1])/2
            }' "$vals")"
        lo="$(head -n1 "$vals")"
        hi="$(tail -n1 "$vals")"
        spread="$(awk -v lo="$lo" -v hi="$hi" -v b="$baseline" 'BEGIN{ if(b==0)print 0; else printf "%.4f",100*(hi-lo)/b }')"
        threshold="$(awk -v s="$spread" 'BEGIN{
            x=1.5*s
            t=int(x)
            if (t < x) t++
            if (t < 5) t=5
            print t
        }')"

        key="$(printf '%s' "$metric" | tr '[:lower:]-' '[:upper:]_')"
        printf '%s_MEDIAN_NS_PER_OP=%s\n' "$key" "$baseline"
        printf '%s_CROSS_ROUND_MIN_MEDIAN_NS_PER_OP=%s\n' "$key" "$lo"
        printf '%s_CROSS_ROUND_MAX_MEDIAN_NS_PER_OP=%s\n' "$key" "$hi"
        printf '%s_CROSS_ROUND_FULL_SPREAD_PCT=%s\n' "$key" "$spread"
        printf '%s_REGRESSION_THRESHOLD_PCT=%s\n' "$key" "$threshold"
    done <<< "$metric_list"
} > "$BASELINE"

echo "### ACCEPTED SERVER BASELINE"
cat "$BASELINE"
echo
echo "Baseline written to: $BASELINE"
