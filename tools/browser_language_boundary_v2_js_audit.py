#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
host = root / "browser" / "arborcore_host.js"
text = host.read_text()

required = [
    "BROWSER_HOST_SYSCALL_SHIM_ONLY",
    "AUTHORITATIVE_JS_LOGIC",
    "navigator.gpu",
    "getContext('2d'",
    "getContext('webgpu'",
]
for token in required:
    if token not in text:
        raise SystemExit(f"FAIL: host shim missing required boundary marker/API: {token}")

prohibited = {
    "Q32 denominator": r"4294967296n|Q32_DENOMINATOR",
    "CSS decimal arithmetic": r"CSS_DECIMAL_SCALE|roundNearestEven",
    "pixel transfer function": r"linear16_to_srgb8|unpremultiply16|alpha16_to_alpha8|exact_rgba16_to_rgba8",
    "embedded WGSL": r"@vertex|@fragment|@compute|texture_2d<",
    "authoritative table literal": r"FROZEN_LINEAR16_TO_SRGB8_BUCKET12|FROZEN_SRGB8_TO_LINEAR16",
}
for label, pattern in prohibited.items():
    if re.search(pattern, text):
        raise SystemExit(f"FAIL: authoritative logic leaked into JS host shim: {label}")

if "./precision_surface.js" in text or "./webgpu_accelerator.js" in text:
    raise SystemExit("FAIL: v2 host shim depends on frozen v1 JS implementation")

production_js = [host]
line_count = sum(len(p.read_text().splitlines()) for p in production_js)
print(f"LBV2_PRODUCTION_JS_FILE_COUNT={len(production_js)}")
print(f"LBV2_PRODUCTION_JS_LINE_COUNT={line_count}")
print("LBV2_AUTHORITATIVE_JS_LOGIC=ZERO")
print("PASS: Browser Host Boundary v2 production JS is host-shim-only")
