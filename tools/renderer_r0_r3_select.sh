#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
SUMMARY="$ROOT/build/renderer-r0-r3/summary.tsv"
OUT="$ROOT/build/renderer-r0-r3/selection.env"

value() { awk -F '\t' -v k="$1" '$1==k {print $2}' "$SUMMARY"; }
q24="$(value coverage_q24)"
q32="$(value coverage_q32)"
rgba8="$(value rgba8_fill)"
rgba16="$(value rgba16_fill)"
rgba32="$(value rgba32_fill)"

python3 - "$q24" "$q32" "$rgba8" "$rgba16" "$rgba32" "$OUT" <<'PY'
import sys
q24,q32,r8,r16,r32 = map(float, sys.argv[1:6])
out=sys.argv[6]
coverage_delta=(q32-q24)/q24*100.0 if q24 else float('inf')
coverage_abs=q32-q24
color_ratio=r16/r8 if r8 else float('inf')
coverage_pass=(coverage_delta <= 15.0) or (coverage_abs <= 2.0)
color_pass=color_ratio <= 2.5
if coverage_pass and color_pass:
    decision='RECOMMEND_RGBA16_Q0_32_FOR_R4_R9_REVIEW'
else:
    decision='REVIEW_REQUIRED'
with open(out,'w',encoding='utf-8') as f:
    f.write('R0_MIN_COLOR_CHANNEL_BITS=16\n')
    f.write('R0_MIN_COVERAGE_FRACTION_BITS=24\n')
    f.write('R0_INTERNAL_COLOR_RECOMMENDATION=RGBA16_UNORM_PREMULTIPLIED_CANDIDATE\n')
    f.write('R0_RGBA8_ROLE=DISPLAY_EXPORT_REFERENCE\n')
    f.write('R0_RGBA32_ROLE=HIGH_PRECISION_REFERENCE\n')
    f.write('R3_COVERAGE_RECOMMENDATION=Q0.32\n')
    f.write(f'Q0_32_VS_Q0_24_DELTA_PCT={coverage_delta:.4f}\n')
    f.write(f'Q0_32_VS_Q0_24_OVERHEAD_NS={coverage_abs:.6f}\n')
    f.write(f'RGBA16_VS_RGBA8_FILL_RATIO={color_ratio:.4f}\n')
    f.write('R0_COLOR_POLICY=RGBA16_MUST_BE_LE_2_5X_RGBA8_FILL_COST\n')
    f.write('R3_COVERAGE_POLICY=Q0_32_REL15_OR_ABS2NS_VS_Q0_24\n')
    f.write(f'R0_R3_SELECTION_DECISION={decision}\n')
    f.write('R0_R3_FOUNDATION_STATE=UNFROZEN_QUALIFIED_RECOMMENDATION\n')
print(f'Q0_32_VS_Q0_24_DELTA_PCT={coverage_delta:.4f}')
print(f'Q0_32_VS_Q0_24_OVERHEAD_NS={coverage_abs:.6f}')
print(f'RGBA16_VS_RGBA8_FILL_RATIO={color_ratio:.4f}')
print(f'R0_R3_SELECTION_DECISION={decision}')
print('R0_R3_FOUNDATION_STATE=UNFROZEN_QUALIFIED_RECOMMENDATION')
if not (coverage_pass and color_pass):
    raise SystemExit(2)
PY
