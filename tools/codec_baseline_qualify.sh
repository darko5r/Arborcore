#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"

ROUNDS="${ARBORCORE_CODEC_BASELINE_ROUNDS:-3}"
PROFILE="$(arborcore_perf_profile)"
BASELINE="$ROOT/generated/performance/codec-$PROFILE.env"

if ! [[ "$ROUNDS" =~ ^[0-9]+$ ]] || (( ROUNDS < 3 )); then
    echo "ARBORCORE_CODEC_BASELINE_ROUNDS must be an integer >= 3" >&2
    exit 2
fi

# Establish a clean, repeatable reference only.
if ! git -C "$ROOT" diff --quiet -- src/asm \
   || ! git -C "$ROOT" diff --cached --quiet -- src/asm \
   || [[ -n "$(git -C "$ROOT" ls-files --others --exclude-standard -- src/asm)" ]]
then
    echo "FAIL: codec reference baseline requires clean production src/asm." >&2
    exit 2
fi

export ARBORCORE_CODEC_RUNS="${ARBORCORE_CODEC_RUNS:-31}"
export ARBORCORE_CODEC_WARMUPS="${ARBORCORE_CODEC_WARMUPS:-3}"

summaries=()
envs=()
for ((r=1; r<=ROUNDS; r++)); do
    export ARBORCORE_CODEC_OUT_DIR="$ROOT/build/codec-repeatability-$PROFILE-$r"
    echo "### Codec reference round $r/$ROUNDS"
    bash "$ROOT/tools/codec_benchmark_run.sh"
    summaries+=("$ARBORCORE_CODEC_OUT_DIR/summary.tsv")
    envs+=("$ARBORCORE_CODEC_OUT_DIR/environment.txt")
done

first_source="$(awk -F= '$1=="production_source_sha256"{print $2}' "${envs[0]}")"
first_text="$(awk -F= '$1=="production_text_bytes"{print $2}' "${envs[0]}")"
first_percent_text="$(awk -F= '$1=="percent_codec_text_bytes"{print $2}' "${envs[0]}")"
first_cpu="$(awk -F= '$1=="bench_cpu"{print $2}' "${envs[0]}")"
first_model="$(awk -F= '$1=="cpu_model"{print substr($0,index($0,"=")+1)}' "${envs[0]}")"
first_driver="$(awk -F= '$1=="cpu_scaling_driver"{print $2}' "${envs[0]}")"
first_governor="$(awk -F= '$1=="cpu_governor"{print $2}' "${envs[0]}" || true)"

for e in "${envs[@]}"; do
    [[ "$(awk -F= '$1=="production_source_sha256"{print $2}' "$e")" == "$first_source" ]] || { echo "Mismatched source hash: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="production_text_bytes"{print $2}' "$e")" == "$first_text" ]] || { echo "Mismatched production text: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="percent_codec_text_bytes"{print $2}' "$e")" == "$first_percent_text" ]] || { echo "Mismatched percent codec text: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="bench_cpu"{print $2}' "$e")" == "$first_cpu" ]] || { echo "Mismatched benchmark CPU: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="cpu_model"{print substr($0,index($0,"=")+1)}' "$e")" == "$first_model" ]] || { echo "Mismatched CPU model: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="cpu_scaling_driver"{print $2}' "$e")" == "$first_driver" ]] || { echo "Mismatched scaling driver: $e" >&2; exit 3; }
    [[ "$(awk -F= '$1=="cpu_governor"{print $2}' "$e" || true)" == "$first_governor" ]] || { echo "Mismatched governor: $e" >&2; exit 3; }
done

metric_list="$(awk -F '\t' 'NR>1{print $1}' "${summaries[0]}" | sort)"
for s in "${summaries[@]:1}"; do
    [[ "$(awk -F '\t' 'NR>1{print $1}' "$s" | sort)" == "$metric_list" ]] || { echo "Mismatched metric set: $s" >&2; exit 3; }
done

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$ROOT/generated/performance"

{
    echo '# Arborcore machine-specific percent-codec reference baseline.'
    echo '# Three independent repeated-run rounds; comparison statistic is median ns/op.'
    echo 'BASELINE_VERSION=1'
    printf 'PERFORMANCE_PROFILE=%s\n' "$PROFILE"
    echo 'POLICY_STATE=ACCEPTED'
    printf 'PRODUCTION_COMMIT=%s\n' "$(git -C "$ROOT" rev-parse HEAD)"
    printf 'PRODUCTION_SOURCE_SHA256=%s\n' "$first_source"
    printf 'PRODUCTION_TEXT_BYTES=%s\n' "$first_text"
    printf 'PERCENT_CODEC_TEXT_BYTES=%s\n' "$first_percent_text"
    printf 'BENCH_CPU=%s\n' "$first_cpu"
    printf 'BENCH_RUNS=%s\n' "$ARBORCORE_CODEC_RUNS"
    printf 'BENCH_WARMUPS=%s\n' "$ARBORCORE_CODEC_WARMUPS"
    printf 'CPU_MODEL=%q\n' "$first_model"
    printf 'CPU_SCALING_DRIVER=%s\n' "$first_driver"
    printf 'CPU_GOVERNOR=%s\n' "$first_governor"
    printf 'SOURCE_REPEATABILITY_ROUNDS=%s\n' "$ROUNDS"
    echo 'BASELINE_AGGREGATION=median_of_round_medians'
    echo 'THRESHOLD_RULE=max_5pct__ceil_1.5x_cross_round_full_median_spread_pct'

    while IFS= read -r metric; do
        vals="$tmp/$metric.values"
        : > "$vals"
        for s in "${summaries[@]}"; do
            awk -F '\t' -v m="$metric" 'NR>1 && $1==m{print $3}' "$s" >> "$vals"
        done
        sort -n -o "$vals" "$vals"
        baseline="$(awk '{a[NR]=$1} END{n=NR;if(n%2)printf "%.6f",a[(n+1)/2];else printf "%.6f",(a[n/2]+a[n/2+1])/2}' "$vals")"
        lo="$(head -n1 "$vals")"
        hi="$(tail -n1 "$vals")"
        spread="$(awk -v lo="$lo" -v hi="$hi" -v b="$baseline" 'BEGIN{if(b==0)print 0;else printf "%.4f",100*(hi-lo)/b}')"
        threshold="$(awk -v s="$spread" 'BEGIN{x=1.5*s;t=int(x);if(t<x)t++;if(t<5)t=5;print t}')"
        key="$(printf '%s' "$metric" | tr '[:lower:]-' '[:upper:]_')"
        printf '%s_MEDIAN_NS_PER_OP=%s\n' "$key" "$baseline"
        printf '%s_CROSS_ROUND_MIN_MEDIAN_NS_PER_OP=%s\n' "$key" "$lo"
        printf '%s_CROSS_ROUND_MAX_MEDIAN_NS_PER_OP=%s\n' "$key" "$hi"
        printf '%s_CROSS_ROUND_FULL_SPREAD_PCT=%s\n' "$key" "$spread"
        printf '%s_REGRESSION_THRESHOLD_PCT=%s\n' "$key" "$threshold"
    done <<< "$metric_list"
} > "$BASELINE"

echo
echo "### ACCEPTED CODEC REFERENCE BASELINE"
cat "$BASELINE"
echo
echo "Baseline written to: $BASELINE"
