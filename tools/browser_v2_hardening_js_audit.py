#!/usr/bin/env python3
from pathlib import Path
import hashlib
import re
import sys

root = Path(__file__).resolve().parents[1]
host = root / 'browser/arborcore_host.js'
expected = 'c7fb40e47ec93796e1a68b44948b983b433ac67b152a628a02678eae297b9d4a'
actual = hashlib.sha256(host.read_bytes()).hexdigest()
if actual != expected:
    raise SystemExit(f'FAIL: frozen production host changed: {actual}')

production = sorted((root / 'browser').glob('*.js'))
if [p.relative_to(root).as_posix() for p in production] != ['browser/arborcore_host.js']:
    raise SystemExit('FAIL: Browser v2 must retain exactly one top-level production JS file')

text = host.read_text()
for marker in [
    "ARBORCORE_HOST_JS_ROLE = 'BROWSER_HOST_SYSCALL_SHIM_ONLY'",
    "AUTHORITATIVE_JS_LOGIC = 'ZERO'",
]:
    if marker not in text:
        raise SystemExit(f'FAIL: missing production authority marker: {marker}')

runtime = (root / 'tests/js/browser_v2_hardening_runtime.mjs').read_text()
if "BV2H_AUTHORITATIVE_JS_LOGIC = 'ZERO'" not in runtime or 'TEST-ONLY' not in runtime:
    raise SystemExit('FAIL: BV2H diagnostic JS is not explicitly test-only/zero-authority')

# Guard against accidentally introducing common semantic-policy implementations into the new diagnostic driver.
forbidden = {
    'pixel transfer implementation': r'linear16_to_srgb8|unpremultiply16|exact_rgba16_to_rgba8',
    'fixed-point geometry implementation': r'Q32_32|q32_mul|q32_div|coverage_raster',
    'fallback policy table': r'fallbackPolicy\s*=|failurePolicy\s*=',
}
for label, pattern in forbidden.items():
    if re.search(pattern, runtime, re.I):
        raise SystemExit(f'FAIL: diagnostic JS contains forbidden {label}')

print('BV2H_PRODUCTION_JS_FILE_COUNT=1')
print(f'BV2H_PRODUCTION_HOST_SHA256={actual}')
print('BV2H_DIAGNOSTIC_JS_ROLE=TEST_ONLY_BROWSER_OBSERVER_AND_HOST_DRIVER')
print('BV2H_AUTHORITATIVE_JS_LOGIC=ZERO')
print('PASS: BV2H diagnostic richness does not reopen production JavaScript authority')
