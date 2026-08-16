#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-v2-hardening-wasm"
MODULE="$OUT/browser-v2-hardening.wasm"
mkdir -p "$OUT"
command -v clang >/dev/null
command -v node >/dev/null
clang --target=wasm32 -I"$ROOT/include" -std=c17 -O2 -ffreestanding -fno-builtin -nostdlib \
  "$ROOT/src/c/browser_hardening_v2.c" \
  "$ROOT/tests/c/browser_hardening_v2_wasm_selftest.c" \
  -Wl,--no-entry \
  -Wl,--export=browser_hardening_v2_selftest \
  -Wl,--export=arbor_browser_hardening_v2_version \
  -Wl,--export=arbor_browser_hardening_v2_reset \
  -Wl,--export=arbor_browser_hardening_v2_lifecycle_state_value \
  -Wl,--export=arbor_browser_hardening_v2_lifecycle_generation \
  -Wl,--export=arbor_browser_hardening_v2_transition \
  -Wl,--export=arbor_browser_hardening_v2_metric_increment \
  -Wl,--export=arbor_browser_hardening_v2_metric_value \
  -Wl,--export=arbor_browser_hardening_v2_timing_record \
  -Wl,--export=arbor_browser_hardening_v2_timing_count \
  -Wl,--export=arbor_browser_hardening_v2_timing_min \
  -Wl,--export=arbor_browser_hardening_v2_timing_max \
  -Wl,--export=arbor_browser_hardening_v2_timing_mean \
  -Wl,--export=arbor_browser_hardening_v2_timing_percentile_permille \
  -Wl,--export-memory -Wl,--strip-all -o "$MODULE"
node --input-type=module - "$MODULE" <<'NODE'
import fs from 'node:fs';
const file=process.argv[2]; const bytes=fs.readFileSync(file); const module=await WebAssembly.compile(bytes);
const imports=WebAssembly.Module.imports(module); if(imports.length!==0) throw new Error(`imports=${imports.length}`);
const instance=await WebAssembly.instantiate(module,{}); if(instance.exports.browser_hardening_v2_selftest()!==0) throw new Error('selftest failed');
console.log('BV2H_WASM_IMPORT_COUNT=0');
console.log(`BV2H_WASM_MODULE_SHA256_PENDING_RUNTIME_IDENTITY=DIAGNOSTIC`);
console.log('PASS: BV2H zero-import C/WASM lifecycle, counters and timing authority');
NODE
printf 'BV2H_WASM_SHA256=%s\n' "$(sha256sum "$MODULE" | awk '{print $1}')"
