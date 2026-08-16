#!/usr/bin/env python3
from pathlib import Path
import json

root=Path(__file__).resolve().parents[1]
out=root/'build/browser-v2-hardening-live'
reports={k:json.loads((out/f'{k}.json').read_text()) for k in ('firefox','chrome')}
webgpu=[(k,r) for k,r in reports.items() if r.get('qualifiedMode')=='webgpu']
if not webgpu: raise SystemExit('FAIL: no live WebGPU performance evidence')
for kind,r in webgpu:
    t=r['timings']['hostEnqueue']
    m=r['metrics']
    if t['count'] < 32: raise SystemExit(f'FAIL: {kind} has fewer than 32 measured presentations')
    if m['sourceViewReuses'] <= m['sourceViewCreations']: raise SystemExit(f'FAIL: {kind} source-view reuse not dominant')
    if m['textureReuses'] <= m['textureCreations']: raise SystemExit(f'FAIL: {kind} texture reuse not dominant')
    if m['bindGroupReuses'] <= m['bindGroupCreations']: raise SystemExit(f'FAIL: {kind} bind-group reuse not dominant')
    print(f'BV2H_PERF_{kind.upper()}_HOST_ENQUEUE_COUNT={t["count"]}')
    print(f'BV2H_PERF_{kind.upper()}_HOST_ENQUEUE_P50_NS={t["p50Ns"]}')
    print(f'BV2H_PERF_{kind.upper()}_HOST_ENQUEUE_P95_NS={t["p95Ns"]}')
    print(f'BV2H_PERF_{kind.upper()}_HOST_ENQUEUE_P99_NS={t["p99Ns"]}')
    print(f'BV2H_PERF_{kind.upper()}_QUEUE_COMPLETION_P50_NS={r["timings"]["queueCompletionLatency"]["p50Ns"]}')
    print(f'BV2H_PERF_{kind.upper()}_TIMER_RESOLUTION_NS={r.get("timerResolutionNs",0)}')
    print(f'BV2H_PERF_{kind.upper()}_SOURCE_VIEW_CREATIONS={m["sourceViewCreations"]}')
    print(f'BV2H_PERF_{kind.upper()}_SOURCE_VIEW_REUSES={m["sourceViewReuses"]}')
for kind,r in reports.items():
    if r.get('qualifiedMode') == 'fallback' and r.get('webGpuInitialized'):
        if r.get('lifecycleBeforeDestroy',{}).get('name') != 'FALLBACK_READY': raise SystemExit(f'FAIL: {kind} late WebGPU failure did not end in FALLBACK_READY')
        if r.get('metrics',{}).get('fallbacks',0) < 1: raise SystemExit(f'FAIL: {kind} late WebGPU failure did not increment fallback counter')
print('BV2H_PERFORMANCE_POLICY=HOST_ENQUEUE_BASELINE_QUEUE_COMPLETION_DIAGNOSTIC_ONLY')
print('BV2H_OPT2_EVIDENCE=QUALIFIED_FOR_REVIEW')
print('PASS: BV2H2/BV2H3 performance and resource-reuse evidence qualified without freezing machine-specific thresholds')
