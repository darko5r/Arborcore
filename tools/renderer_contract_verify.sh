#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

EXPECTED_SOURCE="fe943ed7c52b75f8dd5eecd41f290ac9e06c5201ffeee59a00f713c267418783"
EXPECTED_CONTRACT="1db52f652d50943753bc18a3cf487741e7ad5a494614d64fac32a2f693a5aab7"
EXPECTED_LUT="8572ef0dc49b8c2344482e2cbc352d21caad25ac71adbbc5461a0c4f49055f3b"
EXPECTED_GOLDEN="fda03aa982372e8bb181ecf4e65910478bd8ad66ae08cf12cc1a5f89288673ba"

source_sha="$({ printf '%s\0' include/arborcore/renderer.h src/c/renderer.c renderer/srgb8_linear16_lut.h src/wasm/renderer_memory_builtins.c renderer/arborcore-renderer-1.contract | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
contract_sha="$(sha256sum renderer/arborcore-renderer-1.contract | awk '{print $1}')"
lut_sha="$(sha256sum renderer/srgb8_linear16_lut.h | awk '{print $1}')"

printf 'renderer_source_sha256=%s\n' "$source_sha"
printf 'renderer_contract_sha256=%s\n' "$contract_sha"
printf 'renderer_lut_sha256=%s\n' "$lut_sha"
[[ "$source_sha" == "$EXPECTED_SOURCE" ]]
[[ "$contract_sha" == "$EXPECTED_CONTRACT" ]]
[[ "$lut_sha" == "$EXPECTED_LUT" ]]

required_contract_lines=(
  'PIXEL_FORMAT=RGBA16_UNORM_LE_PREMULTIPLIED'
  'INTERNAL_COLOR_SPACE=LINEAR_LIGHT_SRGB_PRIMARIES'
  'COVERAGE_FORMAT=Q0.32_UINT64_0_TO_2P32'
  'COMPOSITING=PORTER_DUFF_SOURCE_OVER_PREMULTIPLIED'
  'PIXEL_CELL=HALF_OPEN_[x,x+1)x[y,y+1)'
  'RECT_COVERAGE=ANALYTICAL_AREA_NO_SUPERSAMPLING'
  'LINE_AA=FIXED_POINT_WU_MIDPOINT_Q0.32'
  'CURVE_TOLERANCE=1_OVER_256_DEVICE_PIXEL'
  'WASM_REFERENCE=ZERO_IMPORT_SAME_RENDERER_SOURCE'
  "GOLDEN_RGBA16_SHA256=$EXPECTED_GOLDEN"
)
for line in "${required_contract_lines[@]}"; do
  grep -Fqx -- "$line" renderer/arborcore-renderer-1.contract || {
    echo "FAIL: missing renderer contract line: $line" >&2
    exit 1
  }
done

if grep -Eq '\b(malloc|calloc|realloc|free)[[:space:]]*\(' src/c/renderer.c; then
  echo "FAIL: renderer reference layer introduced hidden allocation." >&2
  exit 1
fi
if grep -Eq '\b(float|double)\b' src/c/renderer.c; then
  echo "FAIL: authoritative renderer source contains floating-point types." >&2
  exit 1
fi

mapfile -t symbols < <(nm -g --defined-only build/renderer/renderer.o | awk 'NF >= 3 {print $3}' | grep '^arbor_' | sort -u)
printf 'renderer_c_surface_symbol_count=%d\n' "${#symbols[@]}"
[[ "${#symbols[@]}" -eq 15 ]]

mapfile -t undefined < <(nm -u build/renderer/renderer.o | awk '{print $NF}' | sort -u)
semantic_dependencies=()
hardening_dependencies=()
unexpected_dependencies=()
for symbol in "${undefined[@]}"; do
  case "$symbol" in
    arbor_coord_abs|arbor_coord_add|arbor_coord_div|arbor_coord_mul|arbor_coord_sub)
      semantic_dependencies+=("$symbol")
      ;;
    __stack_chk_fail|__stack_chk_fail_local)
      hardening_dependencies+=("$symbol")
      ;;
    *)
      unexpected_dependencies+=("$symbol")
      ;;
  esac
done
expected_semantic=(arbor_coord_abs arbor_coord_add arbor_coord_div arbor_coord_mul arbor_coord_sub)
if [[ "${semantic_dependencies[*]}" != "${expected_semantic[*]}" ]]; then
  echo "FAIL: renderer Geometry dependency surface changed." >&2
  printf 'actual_semantic=%s\n' "${semantic_dependencies[*]}" >&2
  printf 'expected_semantic=%s\n' "${expected_semantic[*]}" >&2
  exit 1
fi
if (( ${#unexpected_dependencies[@]} != 0 )); then
  echo "FAIL: renderer introduced an unexpected external dependency." >&2
  printf 'unexpected=%s\n' "${unexpected_dependencies[*]}" >&2
  exit 1
fi
printf 'renderer_geometry_dependency_count=%d\n' "${#semantic_dependencies[@]}"
printf 'renderer_native_hardening_dependency_count=%d\n' "${#hardening_dependencies[@]}"
if (( ${#hardening_dependencies[@]} != 0 )); then
  printf 'renderer_native_hardening_dependencies=%s\n' "${hardening_dependencies[*]}"
fi

fill_disassembly="$(objdump -dr -Mintel build/renderer/renderer.o | sed -n '/<arbor_renderer_fill_rect>:/,/^$/p')"
if grep -Eq 'arbor_rgba16_apply_coverage|arbor_rgba16_source_over|arbor_renderer_rect_coverage' <<< "$fill_disassembly"; then
  echo "FAIL: arbor_renderer_fill_rect reintroduced an interposable public hot-loop call." >&2
  exit 1
fi
echo "PASS: arbor_renderer_fill_rect hot loop avoids interposable public coverage/compositing calls"

echo "PASS: Precision Renderer Reference Contract v1 source/shape invariants"
