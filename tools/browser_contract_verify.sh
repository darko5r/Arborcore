#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
LIB="$ROOT/build/libarborcore_browser_surface.a"
CONTRACT="$ROOT/browser/arborcore-browser-surface-1.contract"
ACCEL="$ROOT/browser/linear16_srgb8_bucket12.h"
EXPECTED_GOLDEN="a24be39e38b4fb654e79a2161177cf3670c354695f0b2fa13be677e8d1624dfd"
EXPECTED_OPAQUE="c21b35e3f28e676cedf24c13575a7346682e101a2d26aad9598d0cdbcee9ee3b"
EXPECTED_ACCEL="a7e7d15e382f7c3ed91ffcf0b8a3567e9994d6a01afd0b329343775f4d3426ba"
cd "$ROOT"

source_hash="$({ printf '%s\0' \
  include/arborcore/browser_surface.h \
  src/c/browser_surface.c \
  browser/precision_surface.js \
  browser/linear16_srgb8_bucket12.h \
  browser/arborcore-browser-surface-1.contract \
  | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
contract_hash="$(sha256sum browser/arborcore-browser-surface-1.contract | awk '{print $1}')"
accel_hash="$(sha256sum browser/linear16_srgb8_bucket12.h | awk '{print $1}')"
public_symbols="$(nm -g --defined-only "$LIB" | awk 'NF >= 3 {print $3}' | grep '^arbor_browser_' | sort -u || true)"
public_count="$(printf '%s\n' "$public_symbols" | sed '/^$/d' | wc -l)"
undefined="$(nm -u "$LIB" | awk '$NF !~ /:$/ && NF >= 1 {print $NF}' | sort -u)"
semantic="$(printf '%s\n' "$undefined" | grep '^arbor_' || true)"
hardening="$(printf '%s\n' "$undefined" | grep -E '^__stack_chk_fail(_local)?$' || true)"
other="$(printf '%s\n' "$undefined" | grep -Ev '^(__stack_chk_fail(_local)?)$' | sed '/^$/d' || true)"

printf 'browser_source_sha256=%s\n' "$source_hash"
printf 'browser_contract_sha256=%s\n' "$contract_hash"
printf 'browser_export_accelerator_sha256=%s\n' "$accel_hash"
printf 'browser_c_surface_symbol_count=%s\n' "$public_count"
printf 'browser_renderer_symbol_dependency_count=%s\n' "$(printf '%s\n' "$semantic" | sed '/^$/d' | wc -l)"
printf 'browser_native_hardening_dependency_count=%s\n' "$(printf '%s\n' "$hardening" | sed '/^$/d' | wc -l)"
if [[ -n "$hardening" ]]; then
  printf 'browser_native_hardening_dependencies=%s\n' "$(printf '%s\n' "$hardening" | tr '\n' ' ' | sed 's/ $//')"
fi

[[ "$public_count" == "2" ]]
[[ -z "$semantic" ]]
[[ -z "$other" ]]
[[ "$accel_hash" == "$EXPECTED_ACCEL" ]]
grep -qx 'BROWSER_CONTRACT_VERSION=1.0' "$CONTRACT"
grep -qx 'BROWSER_CONTRACT_STATE=FROZEN' "$CONTRACT"
grep -qx 'BROWSER_C_SURFACE_STATE=UNFROZEN_CONSTRUCTION' "$CONTRACT"
grep -qx 'BROWSER_JS_SURFACE_STATE=UNFROZEN_CONSTRUCTION' "$CONTRACT"
grep -qx 'HTML_CSS_POLICY=PARALLEL_INDEPENDENT_RENDERING_PATH' "$CONTRACT"
grep -qx 'WEBGPU_ROLE=FUTURE_ACCELERATOR_OUT_OF_SCOPE' "$CONTRACT"
grep -qx 'AUTHORITATIVE_PIXEL_FORMAT=RGBA16_UNORM_LE_PREMULTIPLIED_LINEAR_LIGHT' "$CONTRACT"
grep -qx "PRESENTATION_EXPORT_RGBA8_GOLDEN_SHA256=$EXPECTED_GOLDEN" "$CONTRACT"
grep -qx "OPAQUE_CANVAS_PROBE_RGBA8_SHA256=$EXPECTED_OPAQUE" "$CONTRACT"
grep -qx 'PRESENTATION_EXPORT_TRANSFER_SOURCE=FROZEN_RENDERER_SRGB8_LINEAR16_TABLE' "$CONTRACT"
grep -qx 'PRESENTATION_EXPORT_ACCELERATOR=LINEAR16_BUCKET12_EXACT_MIDPOINT_NEAREST_EVEN' "$CONTRACT"
grep -qx 'PRESENTATION_EXPORT_RENDERER_SYMBOL_DEPENDENCY=NONE' "$CONTRACT"
grep -qx 'PRESENTATION_EXPORT_EQUIVALENCE=EXHAUSTIVE_OPAQUE_LINEAR16_PLUS_ALL_ALPHA_STRUCTURED' "$CONTRACT"
grep -qx 'WASM_MEMORY_VIEW_POLICY=REACQUIRE_AFTER_MEMORY_BUFFER_IDENTITY_CHANGE' "$CONTRACT"
grep -qx 'HYBRID_COORDINATE_EXPORT=Q32_32_BIGINT_TO_CSS_DECIMAL_9_NEAREST_EVEN' "$CONTRACT"
grep -q 'typeof rawValue.*bigint\|typeof rawValue !== .bigint.' "$ROOT/browser/precision_surface.js"
if grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' "$ROOT/src/c/browser_surface.c"; then
  echo 'FAIL: browser bridge must not allocate.' >&2
  exit 1
fi
if grep -Eq '\b(float|double)\b' "$ROOT/src/c/browser_surface.c"; then
  echo 'FAIL: browser C bridge authoritative conversion must not use floating point.' >&2
  exit 1
fi
if nm -u "$LIB" | grep -q 'arbor_rgba16_to_srgb8'; then
  echo 'FAIL: B1 hot export path regained an interposable renderer conversion call.' >&2
  exit 1
fi

echo 'PASS: B1 export accelerator has no runtime renderer symbol dependency'
echo 'PASS: Browser Precision Surface v1 frozen contract/source invariants'
