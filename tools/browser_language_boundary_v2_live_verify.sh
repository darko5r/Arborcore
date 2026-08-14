#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-language-boundary-v2-live"
MODULE="$OUT/arborcore-browser-language-boundary-v2.wasm"
MODE="${1:-serve}"
mkdir -p "$OUT"
if [[ "$MODE" == "evidence" ]]; then
  [[ -s "$OUT/result.env" ]] || { echo 'FAIL: LBv2 live evidence absent; run live verification first.' >&2; exit 1; }
  cat "$OUT/result.env"
  grep -qx 'LBV2_LIVE_BROWSER_RESULT=PASS' "$OUT/result.env"
  grep -qx 'LBV2_LIVE_BROWSER_COUNT=2' "$OUT/result.env"
  grep -qx 'LBV2_AUTHORITATIVE_JS_LOGIC=ZERO' "$OUT/result.env"
  echo 'PASS: retained Browser Language Boundary v2 live evidence'
  exit 0
fi
command -v clang >/dev/null 2>&1
clang --target=wasm32 \
  -I"$ROOT/include" -I"$ROOT/tests/c" \
  -std=c17 -O2 -ffreestanding -fno-builtin -nostdlib \
  "$ROOT/src/c/geometry.c" \
  "$ROOT/src/c/renderer.c" \
  "$ROOT/src/c/browser_surface.c" \
  "$ROOT/src/c/browser_host_v2.c" \
  "$ROOT/src/wasm/geometry_int128_builtins.c" \
  "$ROOT/src/wasm/renderer_memory_builtins.c" \
  "$ROOT/tests/c/browser_wasm_selftest.c" \
  "$ROOT/tests/c/browser_host_v2_wasm_selftest.c" \
  -Wl,--no-entry \
  -Wl,--export=browser_wasm_prepare \
  -Wl,--export=browser_wasm_rgba16_ptr -Wl,--export=browser_wasm_rgba16_size \
  -Wl,--export=browser_wasm_rgba8_ptr -Wl,--export=browser_wasm_rgba8_size \
  -Wl,--export=browser_wasm_opaque_rgba8_ptr -Wl,--export=browser_wasm_opaque_rgba8_size \
  -Wl,--export=browser_wasm_width -Wl,--export=browser_wasm_height \
  -Wl,--export=browser_wasm_rgba8_stride \
  -Wl,--export=browser_wasm_opaque_width -Wl,--export=browser_wasm_opaque_height \
  -Wl,--export=browser_wasm_opaque_rgba8_stride \
  -Wl,--export=browser_host_v2_selftest \
  -Wl,--export=arbor_browser_host_v2_version \
  -Wl,--export=arbor_browser_host_v2_format_q32_css \
  -Wl,--export=arbor_browser_host_v2_resolve_device_size \
  -Wl,--export=arbor_browser_host_v2_validate_rgba8 -Wl,--export=arbor_browser_host_v2_validate_rgba16 \
  -Wl,--export=arbor_browser_host_v2_classify_gpu_failure \
  -Wl,--export=arbor_browser_host_v2_state_init -Wl,--export=arbor_browser_host_v2_state_webgpu_ready \
  -Wl,--export=arbor_browser_host_v2_state_failure -Wl,--export=arbor_browser_host_v2_state_destroy \
  -Wl,--export=arbor_browser_host_v2_prepare_gpu_tables \
  -Wl,--export=arbor_browser_host_v2_bucket12_ptr -Wl,--export=arbor_browser_host_v2_bucket12_bytes \
  -Wl,--export=arbor_browser_host_v2_forward_lut_ptr -Wl,--export=arbor_browser_host_v2_forward_lut_bytes \
  -Wl,--export=arbor_browser_host_v2_size_width -Wl,--export=arbor_browser_host_v2_size_height \
  -Wl,--export=arbor_browser_host_v2_resolved_size_mode -Wl,--export=arbor_browser_host_v2_layout_byte_length \
  -Wl,--export=arbor_browser_host_v2_rgba8_output_bytes \
  -Wl,--export=arbor_browser_host_v2_dispatch_x -Wl,--export=arbor_browser_host_v2_dispatch_y \
  -Wl,--export=arbor_browser_host_v2_state_present_mode -Wl,--export=arbor_browser_host_v2_state_failure_class \
  -Wl,--export=arbor_browser_host_v2_state_generation \
  -Wl,--export=arbor_browser_host_v2_css_scratch_ptr -Wl,--export=arbor_browser_host_v2_css_scratch_bytes \
  -Wl,--export=arbor_browser_host_v2_written_scratch_ptr -Wl,--export=arbor_browser_host_v2_written_value \
  -Wl,--export=arbor_browser_host_v2_failure_scratch_ptr -Wl,--export=arbor_browser_host_v2_failure_scratch_bytes \
  -Wl,--export=arbor_browser_host_v2_size_scratch_ptr -Wl,--export=arbor_browser_host_v2_layout_scratch_ptr \
  -Wl,--export=arbor_browser_host_v2_state_scratch_ptr \
  -Wl,--export-memory -Wl,--strip-all \
  -o "$MODULE"
node "$ROOT/tools/browser_language_boundary_v2_live_runner.mjs"
