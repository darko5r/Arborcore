import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = process.env.ARBORCORE_ROOT || path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const webRoot = path.join(root, 'build', 'browser-webgpu-web');
const outRoot = path.join(root, 'build', 'browser-webgpu-live');
fs.mkdirSync(outRoot, { recursive: true });

const EXPECTED_RGBA8 = 'a24be39e38b4fb654e79a2161177cf3670c354695f0b2fa13be677e8d1624dfd';
const EXPECTED_OPAQUE = 'c21b35e3f28e676cedf24c13575a7346682e101a2d26aad9598d0cdbcee9ee3b';
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
  if (/Chrome\//.test(ua) && !/(Edg|OPR)\//.test(ua)) return 'chrome';
  return '';
}

function sanitize(value) {
  return String(value ?? '').replace(/[\r\n=]/g, '_').replace(/\s+/g, '_');
}

function validateReport(kind, report, qualificationMode) {
  if (report.result !== 'PASS') {
    throw new Error(`${kind} live-browser selftest failed: ${report.error || 'unknown failure'}`);
  }
  if (report.qualificationMode !== qualificationMode) {
    throw new Error(`${kind} qualification mode mismatch: ${report.qualificationMode}`);
  }
  if (report.secureContext !== true) {
    throw new Error(`${kind} live-browser origin is not a secure context`);
  }
  if (report.qualifiedMode === 'webgpu') {
    if (report.uploadReadbackHash !== EXPECTED_RGBA8 ||
        report.opaqueWebGpuHash !== EXPECTED_OPAQUE ||
        report.fallbackAfterLossHash !== EXPECTED_OPAQUE ||
        report.recoveredOpaqueHash !== EXPECTED_OPAQUE ||
        report.deviceLossObserved !== true || report.recoveryResult !== true) {
      throw new Error(`${kind} live WebGPU lifecycle/equivalence evidence incomplete`);
    }
  } else if (report.fallbackAfterLossHash !== EXPECTED_OPAQUE) {
    throw new Error(`${kind} live frozen Canvas2D fallback evidence mismatch`);
  }
}

function resultLines(reports) {
  const entries = REQUIRED_BROWSERS.map(kind => [kind, reports[kind]]);
  const webgpuEntries = entries.filter(([, report]) => report.qualifiedMode === 'webgpu');
  const fallbackEntries = entries.filter(([, report]) => report.qualifiedMode !== 'webgpu');
  return [
    'W4_LIVE_REAL_BROWSER_RESULT=PASS',
    `W4_LIVE_BROWSER_COUNT=${entries.length}`,
    `W4_LIVE_WEBGPU_BROWSER_COUNT=${webgpuEntries.length}`,
    `W4_LIVE_FALLBACK_BROWSER_COUNT=${fallbackEntries.length}`,
    ...entries.flatMap(([kind, report]) => {
      const upper = kind.toUpperCase();
      return [
        `W4_LIVE_${upper}_SECURE_CONTEXT=${report.secureContext}`,
        `W4_LIVE_${upper}_WEBGPU_API=${report.webGpuApiAvailable}`,
        `W4_LIVE_${upper}_QUALIFIED_MODE=${report.qualifiedMode}`,
        `W4_LIVE_${upper}_WEBGPU_RUNTIME_FAILURE_CLASS=${sanitize(report.webGpuRuntimeFailureClass || 'NONE')}`,
        `W4_LIVE_${upper}_WEBGPU_RUNTIME_FAILURE=${sanitize(report.webGpuRuntimeFailure || 'NONE')}`,
        `W4_LIVE_${upper}_CANVAS_FORMAT=${report.capability?.canvasFormat || 'NONE'}`,
        `W4_LIVE_${upper}_ADAPTER_VENDOR=${sanitize(report.capability?.adapterInfo?.vendor || 'UNKNOWN')}`,
        `W4_LIVE_${upper}_ADAPTER_DEVICE=${sanitize(report.capability?.adapterInfo?.device || 'UNKNOWN')}`,
        `W4_LIVE_${upper}_UPLOAD_RGBA8_SHA256=${report.uploadReadbackHash || 'NOT_APPLICABLE'}`,
        `W4_LIVE_${upper}_OPAQUE_WEBGPU_SHA256=${report.opaqueWebGpuHash || 'NOT_APPLICABLE'}`,
        `W4_LIVE_${upper}_FALLBACK_SHA256=${report.fallbackAfterLossHash || 'NOT_APPLICABLE'}`,
        `W4_LIVE_${upper}_DEVICE_LOSS_OBSERVED=${report.deviceLossObserved}`,
        `W4_LIVE_${upper}_RECOVERY_RESULT=${report.recoveryResult}`,
        `W4_LIVE_${upper}_RECOVERED_OPAQUE_SHA256=${report.recoveredOpaqueHash || 'NOT_APPLICABLE'}`,
        `W5_LIVE_${upper}_SUBMIT_640X360_MEDIAN_MS=${report.submit640x360MedianMs ?? 'NOT_APPLICABLE'}`,
        `W5_LIVE_${upper}_COMPLETE_640X360_MEDIAN_MS=${report.complete640x360MedianMs ?? 'NOT_APPLICABLE'}`,
        `W4_LIVE_${upper}_USER_AGENT=${sanitize(report.userAgent)}`
      ];
    })
  ];
}

const reports = {};
let finishResolve;
let finishReject;
const finished = new Promise((resolve, reject) => {
  finishResolve = resolve;
  finishReject = reject;
});

const qualificationMode = 'live';
const server = http.createServer((req, res) => {
  if (req.method === 'POST' && req.url === '/report') {
    let data = '';
    req.setEncoding('utf8');
    req.on('data', chunk => { data += chunk; });
    req.on('end', () => {
      let parsed;
      try {
        parsed = JSON.parse(data);
      } catch (error) {
        console.error(`W4_LIVE_REPORT_REJECTED=malformed_json:${sanitize(error)}`);
        res.writeHead(400);
        res.end('invalid report');
        return;
      }

      const kind = browserKind(parsed.userAgent);
      if (!kind) {
        console.error(`W4_LIVE_REPORT_REJECTED=unsupported_browser:${sanitize(parsed.userAgent || '')}`);
        res.writeHead(400);
        res.end('unsupported browser');
        return;
      }

      if (parsed.qualificationMode !== qualificationMode) {
        console.error(`W4_LIVE_REPORT_REJECTED=${kind}:qualification_mode:${sanitize(parsed.qualificationMode)}`);
        res.writeHead(409);
        res.end('qualification mode mismatch');
        return;
      }

      try {
        validateReport(kind, parsed, qualificationMode);
        if (reports[kind]) {
          console.log(`W4_LIVE_DUPLICATE_REPORT_IGNORED=${kind}`);
          res.writeHead(204);
          res.end();
          return;
        }
        reports[kind] = parsed;
        fs.writeFileSync(path.join(outRoot, `${kind}.json`), JSON.stringify(parsed, null, 2) + '\n');
        console.log(`W4_LIVE_REPORT_RECEIVED=${kind}`);
        res.writeHead(204);
        res.end();
        if (REQUIRED_BROWSERS.every(name => reports[name])) finishResolve();
      } catch (error) {
        res.writeHead(400);
        res.end('invalid report');
        finishReject(error);
      }
    });
    return;
  }

  const rawUrl = new URL(req.url || '/', 'http://127.0.0.1');
  const isQualificationPage = rawUrl.pathname === '/' || rawUrl.pathname === '/webgpu_accelerator_browser_test.html';
  if (req.method === 'GET' && isQualificationPage && rawUrl.searchParams.get('qualification') !== qualificationMode) {
    res.writeHead(302, {
      location: `/webgpu_accelerator_browser_test.html?qualification=${qualificationMode}`,
      'cache-control': 'no-store'
    });
    res.end();
    return;
  }
  const pathname = rawUrl.pathname === '/' ? '/webgpu_accelerator_browser_test.html' : rawUrl.pathname;
  const safe = path.normalize(pathname).replace(/^[/\\]+/, '').replace(/^(\.\.(\/|\\|$))+/, '');
  const file = path.join(webRoot, safe);
  if (!file.startsWith(webRoot) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) {
    res.writeHead(404);
    res.end('not found');
    return;
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
  const requestedPort = Number(process.env.ARBORCORE_WEBGPU_LIVE_PORT || 0);
  server.listen(requestedPort, '127.0.0.1', resolve);
});

const port = server.address().port;
const url = `http://127.0.0.1:${port}/webgpu_accelerator_browser_test.html?qualification=${qualificationMode}`;
console.log(`WEBGPU_LIVE_URL=${url}`);
console.log('WEBGPU_LIVE_ACTION=OPEN_THE_URL_IN_YOUR_NORMAL_FIREFOX_AND_NORMAL_CHROME_PROFILES');
console.log('WEBGPU_LIVE_BROWSER_LAUNCH_POLICY=MANUAL_NO_PROFILE_INJECTION');
console.log('WEBGPU_LIVE_WAITING_FOR=firefox,chrome');

const timeoutMs = Number(process.env.ARBORCORE_WEBGPU_LIVE_TIMEOUT_MS || 300000);
const timeout = setTimeout(() => {
  finishReject(new Error(`live-browser qualification timeout after ${timeoutMs} ms`));
}, timeoutMs);

let exitCode = 0;
try {
  await finished;
  const lines = resultLines(reports);
  fs.writeFileSync(path.join(outRoot, 'result.env'), lines.join('\n') + '\n');
  for (const line of lines) console.log(line);
  const webgpuCount = REQUIRED_BROWSERS.filter(kind => reports[kind].qualifiedMode === 'webgpu').length;
  if (webgpuCount < 1) {
    console.error('W4_LIVE_WEBGPU_ADMISSION=REVIEW_NO_LIVE_WEBGPU_BROWSER');
    exitCode = 2;
  } else {
    console.log('W4_LIVE_WEBGPU_ADMISSION=PASS');
  }
} finally {
  clearTimeout(timeout);
  await new Promise(resolve => server.close(resolve));
}
process.exitCode = exitCode;
