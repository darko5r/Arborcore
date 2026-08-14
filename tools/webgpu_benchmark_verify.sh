#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
RESULT="$ROOT/build/browser-webgpu-live/result.env"
[[ -s "$RESULT" ]] || { echo 'FAIL: run webgpu-live-browser-verify first.' >&2; exit 1; }
webgpu_count="$(awk -F= '$1=="W4_LIVE_WEBGPU_BROWSER_COUNT" {print $2}' "$RESULT")"
[[ "$webgpu_count" -ge 1 ]] || { echo 'W5_WEBGPU_PERFORMANCE_RESULT=REVIEW_NO_LIVE_WEBGPU_BROWSER' >&2; exit 2; }
printf 'browser\tmode\tsubmit_640x360_median_ms\tcomplete_640x360_median_ms\n'
for browser in FIREFOX CHROME; do
  mode="$(awk -F= -v key="W4_LIVE_${browser}_QUALIFIED_MODE" '$1==key {print $2}' "$RESULT")"
  submit="$(awk -F= -v key="W5_LIVE_${browser}_SUBMIT_640X360_MEDIAN_MS" '$1==key {print $2}' "$RESULT")"
  complete="$(awk -F= -v key="W5_LIVE_${browser}_COMPLETE_640X360_MEDIAN_MS" '$1==key {print $2}' "$RESULT")"
  printf '%s\t%s\t%s\t%s\n' "${browser,,}" "$mode" "$submit" "$complete"
done
cat > "$ROOT/build/browser-webgpu-live/performance.env" <<EVIDENCE
W5_WEBGPU_PERFORMANCE_RESULT=PASS_DIAGNOSTIC_BASELINE
W5_PERFORMANCE_POLICY=DIAGNOSTIC_UNTIL_REAL_HOST_BASELINE_ACCEPTED
W5_LIVE_WEBGPU_BROWSER_COUNT=$webgpu_count
EVIDENCE
cat "$ROOT/build/browser-webgpu-live/performance.env"
echo 'PASS: W5 live normal-profile WebGPU performance evidence captured without freezing an unqualified numeric threshold'
