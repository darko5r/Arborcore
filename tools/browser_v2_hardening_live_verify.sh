#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-v2-hardening-live"
MODULE="$OUT/arborcore-browser-v2-hardening.wasm"
MODE="${1:-serve}"

CANDIDATE_PATHS=(
  Makefile
  browser/arborcore-browser-v2-hardening-optimization-1.contract
  docs/BROWSER_V2_DIAGNOSTICS.md
  docs/BROWSER_V2_HARDENING_OPTIMIZATION.md
  include/arborcore/browser_hardening_v2.h
  src/c/browser_hardening_v2.c
  tests/browser/browser_v2_hardening_diagnostics.html
  tests/c/browser_hardening_v2_test.c
  tests/c/browser_hardening_v2_wasm_selftest.c
  tests/data/browser_v2_dpr_vectors.json
  tests/data/browser_v2_opt3_performance_evidence.json
  tests/js/browser_v2_hardening_runtime.mjs
  tools/browser_v2_hardening_contract_verify.sh
  tools/browser_v2_hardening_gate.sh
  tools/browser_v2_hardening_inventory.sh
  tools/browser_v2_hardening_js_audit.py
  tools/browser_v2_hardening_live_runner.mjs
  tools/browser_v2_hardening_live_verify.sh
  tools/browser_v2_hardening_native_verify.sh
  tools/browser_v2_hardening_performance_verify.py
  tools/browser_v2_hardening_reproducibility_verify.sh
  tools/browser_v2_hardening_wasm_verify.sh
  tools/browser_v2_opt3_qualification_verify.py
)

candidate_sha() {
  (
    cd "$ROOT"
    for rel in "${CANDIDATE_PATHS[@]}"; do
      [[ -f "$rel" ]] || { echo "FAIL: missing BV2H candidate path $rel" >&2; exit 1; }
      sha256sum "$rel"
    done |
    LC_ALL=C sort -k2 |
    sha256sum |
    awk '{print $1}'
  )
}

mkdir -p "$OUT"

if [[ "$MODE" == evidence ]]; then
  [[ -s "$OUT/result.env" && -s "$OUT/firefox.json" && -s "$OUT/chrome.json" && -s "$MODULE" ]] || {
    echo 'FAIL: BV2H live evidence/module absent; run browser-v2-hardening-live-verify first.' >&2
    exit 1
  }

  cat "$OUT/result.env"
  grep -qx 'BV2H_LIVE_BROWSER_RESULT=PASS' "$OUT/result.env"
  grep -qx 'BV2H_LIVE_BROWSER_COUNT=2' "$OUT/result.env"
  grep -qx 'BV2H_AUTHORITATIVE_JS_LOGIC=ZERO' "$OUT/result.env"
  grep -qx 'BV2H_OPT3_ADMISSION=CANDIDATE_NOT_ADMITTED_REQUIRES_BV2H4_REVIEW' "$OUT/result.env"

  recorded_source="$(awk -F= '$1=="BV2H_LIVE_SOURCE_SHA256"{print $2}' "$OUT/result.env")"
  recorded_wasm="$(awk -F= '$1=="BV2H_LIVE_WASM_SHA256"{print $2}' "$OUT/result.env")"
  [[ "$recorded_source" =~ ^[0-9a-f]{64}$ ]] || { echo 'FAIL: live source SHA missing/invalid' >&2; exit 1; }
  [[ "$recorded_wasm" =~ ^[0-9a-f]{64}$ ]] || { echo 'FAIL: live WASM SHA missing/invalid' >&2; exit 1; }

  current_source="$(candidate_sha)"
  current_wasm="$(sha256sum "$MODULE" | awk '{print $1}')"

  [[ "$current_source" == "$recorded_source" ]] || {
    echo "FAIL: stale BV2H browser evidence: source candidate changed" >&2
    echo "recorded=$recorded_source" >&2
    echo "current=$current_source" >&2
    exit 1
  }
  [[ "$current_wasm" == "$recorded_wasm" ]] || {
    echo "FAIL: stale BV2H browser evidence: live WASM module changed" >&2
    echo "recorded=$recorded_wasm" >&2
    echo "current=$current_wasm" >&2
    exit 1
  }

  echo "BV2H_LIVE_EVIDENCE_SOURCE_SHA256=$current_source"
  echo "BV2H_LIVE_EVIDENCE_WASM_SHA256=$current_wasm"
  echo 'PASS: retained BV2H live evidence is bound to the exact current candidate and live WASM module'
  exit 0
fi

if [[ "$MODE" != serve ]]; then
  echo "FAIL: unknown BV2H live verify mode: $MODE" >&2
  exit 1
fi

command -v clang >/dev/null
command -v node >/dev/null

# A live run must never silently reuse an earlier browser report.
rm -f "$OUT/result.env" "$OUT/firefox.json" "$OUT/chrome.json"

source_sha="$(candidate_sha)"

clang --target=wasm32 -I"$ROOT/include" -I"$ROOT/tests/c" -std=c17 -O2 -ffreestanding -fno-builtin -nostdlib \
  "$ROOT/src/c/geometry.c" "$ROOT/src/c/renderer.c" "$ROOT/src/c/browser_surface.c" "$ROOT/src/c/browser_host_v2.c" "$ROOT/src/c/browser_hardening_v2.c" \
  "$ROOT/src/wasm/geometry_int128_builtins.c" "$ROOT/src/wasm/renderer_memory_builtins.c" \
  "$ROOT/tests/c/browser_wasm_selftest.c" "$ROOT/tests/c/browser_host_v2_wasm_selftest.c" "$ROOT/tests/c/browser_hardening_v2_wasm_selftest.c" \
  -Wl,--no-entry \
  -Wl,--export=browser_wasm_prepare -Wl,--export=browser_wasm_rgba16_ptr -Wl,--export=browser_wasm_rgba16_size -Wl,--export=browser_wasm_rgba8_ptr -Wl,--export=browser_wasm_rgba8_size \
  -Wl,--export=browser_wasm_opaque_rgba8_ptr -Wl,--export=browser_wasm_opaque_rgba8_size -Wl,--export=browser_wasm_width -Wl,--export=browser_wasm_height -Wl,--export=browser_wasm_rgba8_stride \
  -Wl,--export=browser_wasm_opaque_width -Wl,--export=browser_wasm_opaque_height -Wl,--export=browser_wasm_opaque_rgba8_stride \
  -Wl,--export=browser_host_v2_selftest -Wl,--export=browser_hardening_v2_selftest \
  -Wl,--export=arbor_browser_host_v2_version -Wl,--export=arbor_browser_host_v2_format_q32_css -Wl,--export=arbor_browser_host_v2_resolve_device_size \
  -Wl,--export=arbor_browser_host_v2_validate_rgba8 -Wl,--export=arbor_browser_host_v2_validate_rgba16 -Wl,--export=arbor_browser_host_v2_classify_gpu_failure \
  -Wl,--export=arbor_browser_host_v2_state_init -Wl,--export=arbor_browser_host_v2_state_webgpu_ready -Wl,--export=arbor_browser_host_v2_state_failure -Wl,--export=arbor_browser_host_v2_state_destroy \
  -Wl,--export=arbor_browser_host_v2_prepare_gpu_tables -Wl,--export=arbor_browser_host_v2_bucket12_ptr -Wl,--export=arbor_browser_host_v2_bucket12_bytes -Wl,--export=arbor_browser_host_v2_forward_lut_ptr -Wl,--export=arbor_browser_host_v2_forward_lut_bytes \
  -Wl,--export=arbor_browser_host_v2_size_width -Wl,--export=arbor_browser_host_v2_size_height -Wl,--export=arbor_browser_host_v2_resolved_size_mode -Wl,--export=arbor_browser_host_v2_layout_byte_length \
  -Wl,--export=arbor_browser_host_v2_rgba8_output_bytes -Wl,--export=arbor_browser_host_v2_dispatch_x -Wl,--export=arbor_browser_host_v2_dispatch_y \
  -Wl,--export=arbor_browser_host_v2_state_present_mode -Wl,--export=arbor_browser_host_v2_state_failure_class -Wl,--export=arbor_browser_host_v2_state_generation \
  -Wl,--export=arbor_browser_host_v2_css_scratch_ptr -Wl,--export=arbor_browser_host_v2_css_scratch_bytes -Wl,--export=arbor_browser_host_v2_written_scratch_ptr -Wl,--export=arbor_browser_host_v2_written_value \
  -Wl,--export=arbor_browser_host_v2_failure_scratch_ptr -Wl,--export=arbor_browser_host_v2_failure_scratch_bytes -Wl,--export=arbor_browser_host_v2_size_scratch_ptr -Wl,--export=arbor_browser_host_v2_layout_scratch_ptr -Wl,--export=arbor_browser_host_v2_state_scratch_ptr \
  -Wl,--export=arbor_browser_hardening_v2_version -Wl,--export=arbor_browser_hardening_v2_reset -Wl,--export=arbor_browser_hardening_v2_lifecycle_state_value -Wl,--export=arbor_browser_hardening_v2_lifecycle_generation -Wl,--export=arbor_browser_hardening_v2_transition \
  -Wl,--export=arbor_browser_hardening_v2_metric_increment -Wl,--export=arbor_browser_hardening_v2_metric_value \
  -Wl,--export=arbor_browser_hardening_v2_timing_record -Wl,--export=arbor_browser_hardening_v2_timing_count -Wl,--export=arbor_browser_hardening_v2_timing_min -Wl,--export=arbor_browser_hardening_v2_timing_max -Wl,--export=arbor_browser_hardening_v2_timing_mean -Wl,--export=arbor_browser_hardening_v2_timing_percentile_permille \
  -Wl,--export-memory -Wl,--strip-all -o "$MODULE"

wasm_sha="$(sha256sum "$MODULE" | awk '{print $1}')"

printf 'BV2H_LIVE_SOURCE_SHA256=%s\n' "$source_sha"
printf 'BV2H_LIVE_WASM_SHA256=%s\n' "$wasm_sha"

ARBORCORE_BV2H_SOURCE_SHA256="$source_sha" \
ARBORCORE_BV2H_WASM_SHA256="$wasm_sha" \
node "$ROOT/tools/browser_v2_hardening_live_runner.mjs"
