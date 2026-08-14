import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = process.env.ARBORCORE_ROOT || path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const webRoot = path.join(root, 'build', 'browser-webgpu-postfreeze-web');
const outRoot = path.join(root, 'build', 'browser-webgpu-postfreeze-live');
fs.mkdirSync(outRoot, { recursive: true });
const REQUIRED_BROWSERS = ['firefox', 'chrome'];

function contentType(file) {
  if (file.endsWith('.wasm')) return 'application/wasm';
  if (file.endsWith('.js') || file.endsWith('.mjs')) return 'text/javascript; charset=utf-8';
  if (file.endsWith('.html')) return 'text/html; charset=utf-8';
  return 'application/octet-stream';
}

function browserKind(userAgent) {
  const ua = String(userAgent || '');
  if (/Firefox\//.test(ua)) return 'firefox';
  if (/Edg\//.test(ua)) return 'edge';
  if (/Chrome\//.test(ua) && !/OPR\//.test(ua)) return 'chrome';
  if (/Safari\//.test(ua) && !/Chrome\//.test(ua)) return 'safari';
  return '';
}

function sanitize(value) {
  return String(value ?? '').replace(/[\r\n=]/g, '_').replace(/\s+/g, '_');
}

function metric(value) {
  return typeof value === 'number' && Number.isFinite(value) ? String(value) : 'NOT_APPLICABLE';
}

function validateReport(kind, report) {
  if (report.result !== 'PASS') throw new Error(`${kind} OPT browser test failed: ${report.error || 'unknown failure'}`);
  if (report.qualificationMode !== 'live') throw new Error(`${kind} OPT qualification mode mismatch`);
  if (report.secureContext !== true) throw new Error(`${kind} OPT origin is not secure`);
  if (report.qualifiedMode === 'webgpu') {
    if (!report.opt1 || !report.opt2 || !report.opt3) throw new Error(`${kind} WebGPU OPT evidence incomplete`);
    if (report.opt2.exactFrozenRgba8 !== true) throw new Error(`${kind} OPT2 frozen-RGBA8 equivalence failed`);
    if (report.opt3.exactEquivalence !== true) throw new Error(`${kind} OPT3 exact-conversion equivalence failed`);
    if (report.opt3.contractAdmission !== 'NONE_REQUIRES_FUTURE_CONTRACT_REVISION') {
      throw new Error(`${kind} OPT3 attempted to enter frozen WebGPU v1 contract`);
    }
  } else {
    const allowed = new Set([
      'DEVICE_LOSS_OR_PLATFORM_ALLOCATION_FAILURE',
      'WEBGPU_UNAVAILABLE_FROZEN_FALLBACK'
    ]);
    if (!allowed.has(report.webGpuRuntimeFailureClass)) {
      throw new Error(`${kind} fallback has inadmissible failure class ${report.webGpuRuntimeFailureClass}`);
    }
  }
}

function resultLines(reports) {
  const entries = REQUIRED_BROWSERS.map(kind => [kind, reports[kind]]);
  const webgpuEntries = entries.filter(([, report]) => report.qualifiedMode === 'webgpu');
  const fallbackEntries = entries.filter(([, report]) => report.qualifiedMode !== 'webgpu');
  const lines = [
    'OPT4_LIVE_BROWSER_RESULT=PASS',
    `OPT4_LIVE_BROWSER_COUNT=${entries.length}`,
    `OPT4_LIVE_WEBGPU_BROWSER_COUNT=${webgpuEntries.length}`,
    `OPT4_LIVE_FALLBACK_BROWSER_COUNT=${fallbackEntries.length}`,
    'OPT4_REQUIRED_CURRENT_HOST_BROWSERS=FIREFOX_CHROME',
    'OPT4_BROAD_BROWSER_RELEASE_CLAIM=NOT_ADMITTED_WEBKIT_SAFARI_NOT_FORMALLY_QUALIFIED'
  ];
  for (const [kind, report] of entries) {
    const upper = kind.toUpperCase();
    lines.push(
      `OPT4_LIVE_${upper}_QUALIFIED_MODE=${report.qualifiedMode}`,
      `OPT4_LIVE_${upper}_WEBGPU_RUNTIME_FAILURE_CLASS=${sanitize(report.webGpuRuntimeFailureClass || 'NONE')}`,
      `OPT4_LIVE_${upper}_USER_AGENT=${sanitize(report.userAgent)}`
    );
    if (report.qualifiedMode === 'webgpu') {
      lines.push(
        `OPT1_LIVE_${upper}_B1_EXPORT_640X360_MEDIAN_MS=${metric(report.opt1.wasmB1Export640x360MedianMs)}`,
        `OPT1_LIVE_${upper}_BASELINE_RGBA8_SUBMIT_640X360_MEDIAN_MS=${metric(report.opt1.baselineRgba8Submit640x360MedianMs)}`,
        `OPT1_LIVE_${upper}_OPTIMIZED_VIEW_MEDIAN_MS=${metric(report.opt1.optimizedProfileMedianMs?.viewAcquireMs)}`,
        `OPT1_LIVE_${upper}_OPTIMIZED_WRITETEXTURE_MEDIAN_MS=${metric(report.opt1.optimizedProfileMedianMs?.writeTextureCallMs)}`,
        `OPT1_LIVE_${upper}_OPTIMIZED_ENCODE_MEDIAN_MS=${metric(report.opt1.optimizedProfileMedianMs?.encodeMs)}`,
        `OPT1_LIVE_${upper}_OPTIMIZED_SUBMIT_CALL_MEDIAN_MS=${metric(report.opt1.optimizedProfileMedianMs?.submitCallMs)}`,
        `OPT1_LIVE_${upper}_OPTIMIZED_COMPLETE_CALLBACK_MEDIAN_MS=${metric(report.opt1.optimizedCompletionCallback640x360MedianMs)}`,
        `OPT1_LIVE_${upper}_OPTIMIZED_BURST120_SUBMIT_TOTAL_MS=${metric(report.opt1.optimizedBurst120SubmitTotalMs)}`,
        `OPT1_LIVE_${upper}_OPTIMIZED_BURST120_COMPLETE_TOTAL_MS=${metric(report.opt1.optimizedBurst120CompleteTotalMs)}`,
        `OPT2_LIVE_${upper}_OPTIMIZED_RGBA8_SUBMIT_640X360_MEDIAN_MS=${metric(report.opt2.optimizedRgba8Submit640x360MedianMs)}`,
        `OPT2_LIVE_${upper}_SOURCE_VIEW_CREATIONS=${report.opt2.stats?.sourceViewCreations ?? 'NOT_APPLICABLE'}`,
        `OPT2_LIVE_${upper}_SOURCE_VIEW_REUSES=${report.opt2.stats?.sourceViewReuses ?? 'NOT_APPLICABLE'}`,
        `OPT2_LIVE_${upper}_EXACT_FROZEN_RGBA8=${report.opt2.exactFrozenRgba8}`,
        `OPT3_LIVE_${upper}_EXACT_EQUIVALENCE=${report.opt3.exactEquivalence}`,
        `OPT3_LIVE_${upper}_GPU_VECTOR_RGBA8_SHA256=${report.opt3.gpuVectorRgba8Hash}`,
        `OPT3_LIVE_${upper}_EXPECTED_VECTOR_RGBA8_SHA256=${report.opt3.expectedVectorRgba8Hash}`,
        `OPT3_LIVE_${upper}_DIRECT_RGBA16_SUBMIT_640X360_MEDIAN_MS=${metric(report.opt3.directRgba16Submit640x360MedianMs)}`,
        `OPT3_LIVE_${upper}_DIRECT_WRITETEXTURE_MEDIAN_MS=${metric(report.opt3.directProfileMedianMs?.writeTextureCallMs)}`,
        `OPT3_LIVE_${upper}_DIRECT_ENCODE_MEDIAN_MS=${metric(report.opt3.directProfileMedianMs?.encodeMs)}`,
        'OPT3_CONTRACT_ADMISSION=NONE_REQUIRES_FUTURE_CONTRACT_REVISION'
      );
    }
  }
  return lines;
}

const reports = {};
let finishResolve;
let finishReject;
const finished = new Promise((resolve, reject) => { finishResolve = resolve; finishReject = reject; });

const server = http.createServer((req, res) => {
  if (req.method === 'POST' && req.url === '/report') {
    let data = '';
    req.setEncoding('utf8');
    req.on('data', chunk => { data += chunk; });
    req.on('end', () => {
      try {
        const parsed = JSON.parse(data);
        const kind = browserKind(parsed.userAgent);
        if (!kind) throw new Error('unsupported browser');
        validateReport(kind, parsed);
        if (!reports[kind]) {
          reports[kind] = parsed;
          fs.writeFileSync(path.join(outRoot, `${kind}.json`), JSON.stringify(parsed, null, 2) + '\n');
          console.log(`OPT4_LIVE_REPORT_RECEIVED=${kind}`);
        } else {
          console.log(`OPT4_LIVE_DUPLICATE_REPORT_IGNORED=${kind}`);
        }
        res.writeHead(204); res.end();
        if (REQUIRED_BROWSERS.every(name => reports[name])) finishResolve();
      } catch (error) {
        res.writeHead(400); res.end('invalid report'); finishReject(error);
      }
    });
    return;
  }
  const rawUrl = new URL(req.url || '/', 'http://127.0.0.1');
  const page = rawUrl.pathname === '/' || rawUrl.pathname === '/webgpu_postfreeze_browser_test.html';
  if (req.method === 'GET' && page && rawUrl.searchParams.get('qualification') !== 'live') {
    res.writeHead(302, { location: '/webgpu_postfreeze_browser_test.html?qualification=live', 'cache-control': 'no-store' });
    res.end(); return;
  }
  const pathname = rawUrl.pathname === '/' ? '/webgpu_postfreeze_browser_test.html' : rawUrl.pathname;
  const safe = path.normalize(pathname).replace(/^[/\\]+/, '').replace(/^(\.\.(\/|\\|$))+/, '');
  const file = path.join(webRoot, safe);
  if (!file.startsWith(webRoot) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) {
    res.writeHead(404); res.end('not found'); return;
  }
  res.writeHead(200, {
    'content-type': contentType(file),
    'cache-control': 'no-store',
    'cross-origin-opener-policy': 'same-origin',
    'cross-origin-resource-policy': 'same-origin'
  });
  fs.createReadStream(file).pipe(res);
});

await new Promise((resolve, reject) => {
  server.once('error', reject);
  server.listen(Number(process.env.ARBORCORE_WEBGPU_POSTFREEZE_LIVE_PORT || 0), '127.0.0.1', resolve);
});
const port = server.address().port;
console.log(`WEBGPU_POSTFREEZE_LIVE_URL=http://127.0.0.1:${port}/webgpu_postfreeze_browser_test.html?qualification=live`);
console.log('WEBGPU_POSTFREEZE_LIVE_ACTION=OPEN_IN_NORMAL_FIREFOX_AND_NORMAL_CHROME');
console.log('WEBGPU_POSTFREEZE_LIVE_WAITING_FOR=firefox,chrome');
const timeoutMs = Number(process.env.ARBORCORE_WEBGPU_POSTFREEZE_LIVE_TIMEOUT_MS || 900000);
const timeout = setTimeout(() => finishReject(new Error(`OPT live-browser timeout after ${timeoutMs} ms`)), timeoutMs);
let exitCode = 0;
try {
  await finished;
  const lines = resultLines(reports);
  fs.writeFileSync(path.join(outRoot, 'result.env'), lines.join('\n') + '\n');
  for (const line of lines) console.log(line);
  const count = REQUIRED_BROWSERS.filter(kind => reports[kind].qualifiedMode === 'webgpu').length;
  if (count < 1) {
    console.error('OPT4_WEBGPU_ADMISSION=REVIEW_NO_LIVE_WEBGPU_BROWSER');
    exitCode = 2;
  } else {
    console.log('OPT4_WEBGPU_ADMISSION=PASS');
  }
} finally {
  clearTimeout(timeout);
  await new Promise(resolve => server.close(resolve));
}
process.exitCode = exitCode;
