#!/usr/bin/env python3
from pathlib import Path
import json

root=Path(__file__).resolve().parents[1]
live=root/'build/browser-v2-hardening-live'
fixture=root/'tests/data/browser_v2_opt3_performance_evidence.json'

reports=[json.loads((live/f'{k}.json').read_text()) for k in ('firefox','chrome')]
webgpu=[r for r in reports if r.get('qualifiedMode')=='webgpu']
if not webgpu:
    raise SystemExit('FAIL: OPT3 requires at least one live WebGPU browser')
for r in webgpu:
    if r.get('opt3Exact') is not True:
        raise SystemExit('FAIL: live WebGPU OPT3 is not byte-exact')
    if r.get('opt3Hash')!='a24be39e38b4fb654e79a2161177cf3670c354695f0b2fa13be677e8d1624dfd':
        raise SystemExit('FAIL: OPT3 hash differs from frozen RGBA8')

e=json.loads(fixture.read_text())
if e.get('schema')!='ARBORCORE_BV2H_OPT3_PERFORMANCE_EVIDENCE_1':
    raise SystemExit('FAIL: OPT3 performance evidence schema mismatch')
if e.get('firefox',{}).get('exactAll') is not True:
    raise SystemExit('FAIL: retained Firefox OPT3 exact evidence missing')
if e.get('firefox',{}).get('performanceSignal')!='STRONG_POSITIVE':
    raise SystemExit('FAIL: retained Firefox OPT3 performance signal mismatch')
if e.get('chrome',{}).get('state')!='QUALIFIED_PLATFORM_FAILURE_EXTERNAL_INSTANCE_LOSS':
    raise SystemExit('FAIL: retained Chrome platform-failure evidence mismatch')
if e.get('productionAdmission')!='DEFERRED':
    raise SystemExit('FAIL: OPT3 production admission must remain deferred')
for size in e['firefox']['sizes']:
    if size['cpuHash'] != size['directHash']:
        raise SystemExit('FAIL: retained OPT3 performance hash mismatch')

print(f'BV2H_OPT3_LIVE_WEBGPU_BROWSER_COUNT={len(webgpu)}')
print('BV2H_OPT3_EXACT_EQUIVALENCE=PASS')
print('BV2H_OPT3_FIREFOX_PERFORMANCE_SIGNAL=STRONG_POSITIVE')
print('BV2H_OPT3_CROSS_BROWSER_PRODUCTION_EVIDENCE=INSUFFICIENT')
print('BV2H_OPT3_STATE=QUALIFIED_TEST_ONLY_PROMISING_FUTURE_CONTRACT_REVISION')
print('BV2H_OPT3_PRODUCTION_ADMISSION=DEFERRED')
print('PASS: OPT3 is exact and strongly promising on Firefox; production admission remains deferred')
