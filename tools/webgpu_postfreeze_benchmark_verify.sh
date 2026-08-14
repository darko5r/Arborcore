#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
LIVE="$ROOT/build/browser-webgpu-postfreeze-live/result.env"
OUT="$ROOT/build/browser-webgpu-postfreeze-performance"
[[ -s "$LIVE" ]] || { echo 'FAIL: run webgpu-postfreeze-live-browser-verify first.' >&2; exit 1; }
mkdir -p "$OUT"
webgpu_count="$(awk -F= '$1=="OPT4_LIVE_WEBGPU_BROWSER_COUNT" {print $2}' "$LIVE")"
[[ "$webgpu_count" -ge 1 ]] || { echo 'OPT1_MEASUREMENT_RESULT=REVIEW_NO_LIVE_WEBGPU_BROWSER' >&2; exit 2; }

printf 'browser\tb1_export_ms\tbaseline_rgba8_submit_ms\toptimized_rgba8_submit_ms\tdirect_rgba16_submit_ms\tcompletion_callback_ms\topt2_result\n' > "$OUT/summary.tsv"
opt2_all=PASS
for browser in FIREFOX CHROME; do
  mode="$(awk -F= -v k="OPT4_LIVE_${browser}_QUALIFIED_MODE" '$1==k {print $2}' "$LIVE")"
  [[ "$mode" == webgpu ]] || continue
  export_ms="$(awk -F= -v k="OPT1_LIVE_${browser}_B1_EXPORT_640X360_MEDIAN_MS" '$1==k {print $2}' "$LIVE")"
  baseline_ms="$(awk -F= -v k="OPT1_LIVE_${browser}_BASELINE_RGBA8_SUBMIT_640X360_MEDIAN_MS" '$1==k {print $2}' "$LIVE")"
  optimized_ms="$(awk -F= -v k="OPT2_LIVE_${browser}_OPTIMIZED_RGBA8_SUBMIT_640X360_MEDIAN_MS" '$1==k {print $2}' "$LIVE")"
  direct_ms="$(awk -F= -v k="OPT3_LIVE_${browser}_DIRECT_RGBA16_SUBMIT_640X360_MEDIAN_MS" '$1==k {print $2}' "$LIVE")"
  completion_ms="$(awk -F= -v k="OPT1_LIVE_${browser}_OPTIMIZED_COMPLETE_CALLBACK_MEDIAN_MS" '$1==k {print $2}' "$LIVE")"
  creations="$(awk -F= -v k="OPT2_LIVE_${browser}_SOURCE_VIEW_CREATIONS" '$1==k {print $2}' "$LIVE")"
  reuses="$(awk -F= -v k="OPT2_LIVE_${browser}_SOURCE_VIEW_REUSES" '$1==k {print $2}' "$LIVE")"
  exact="$(awk -F= -v k="OPT2_LIVE_${browser}_EXACT_FROZEN_RGBA8" '$1==k {print $2}' "$LIVE")"
  opt3_exact="$(awk -F= -v k="OPT3_LIVE_${browser}_EXACT_EQUIVALENCE" '$1==k {print $2}' "$LIVE")"
  [[ "$exact" == true && "$opt3_exact" == true ]]
  result="$(node - "$baseline_ms" "$optimized_ms" "$creations" "$reuses" <<'NODE'
const baseline = Number(process.argv[2]);
const optimized = Number(process.argv[3]);
const creations = Number(process.argv[4]);
const reuses = Number(process.argv[5]);
if (![baseline, optimized, creations, reuses].every(Number.isFinite) || baseline <= 0) process.exit(2);
const within = optimized <= baseline * 1.05;
const reuse = creations <= 3 && reuses >= 100;
process.stdout.write(within && reuse ? 'PASS' : 'FAIL');
NODE
)"
  [[ "$result" == PASS ]] || opt2_all=FAIL
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "${browser,,}" "$export_ms" "$baseline_ms" "$optimized_ms" "$direct_ms" "$completion_ms" "$result" >> "$OUT/summary.tsv"
done
cat "$OUT/summary.tsv"
[[ "$opt2_all" == PASS ]] || { echo 'OPT2_ADMISSION_RESULT=REVIEW_MATERIAL_REGRESSION_OR_NO_VIEW_REUSE' >&2; exit 3; }

cat > "$OUT/result.env" <<EVIDENCE
OPT1_MEASUREMENT_RESULT=PASS
OPT1_MEASUREMENT_POLICY=SPLIT_B1_EXPORT_VIEW_WRITETEXTURE_ENCODE_SUBMIT_BURST_AND_COMPLETION_CALLBACK
OPT1_COMPLETION_CALLBACK_POLICY=DIAGNOSTIC_NOT_GPU_EXECUTION_TIME
OPT2_SEMANTIC_RESULT=PASS_BYTE_EXACT_FROZEN_B1_RGBA8
OPT2_PERFORMANCE_RESULT=PASS_NO_MATERIAL_SUBMIT_REGRESSION
OPT2_ALLOCATION_RESULT=PASS_STABLE_WASM_VIEW_REUSE
OPT2_ADMISSION_RESULT=PASS_OPTIONAL_V1_COMPATIBLE_IMPLEMENTATION
OPT2_SUBMIT_REGRESSION_LIMIT_PERCENT=5
OPT3_EQUIVALENCE_RESULT=PASS_FROZEN_C_B1_ORACLE
OPT3_PERFORMANCE_RESULT=DIAGNOSTIC_ONLY
OPT3_CONTRACT_ADMISSION=NONE_REQUIRES_FUTURE_CONTRACT_REVISION
OPT4_BROAD_BROWSER_RELEASE_CLAIM=NOT_ADMITTED_WEBKIT_SAFARI_NOT_FORMALLY_QUALIFIED
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: OPT1 measurement and OPT2/OPT3 post-W6 performance/equivalence policy'
