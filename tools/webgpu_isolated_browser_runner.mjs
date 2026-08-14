import http from 'node:http';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const root = process.env.ARBORCORE_ROOT || path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const webRoot = path.join(root, 'build', 'browser-webgpu-web');
const outRoot = path.join(root, 'build', 'browser-webgpu-isolated');
fs.mkdirSync(outRoot, { recursive: true });

const EXPECTED_RGBA8 = 'a24be39e38b4fb654e79a2161177cf3670c354695f0b2fa13be677e8d1624dfd';
const EXPECTED_OPAQUE = 'c21b35e3f28e676cedf24c13575a7346682e101a2d26aad9598d0cdbcee9ee3b';
const selection = String(process.env.ARBORCORE_WEBGPU_ISOLATED_BROWSER || 'all').toLowerCase();
const validSelections = new Set(['all', 'firefox', 'chrome']);
if (!validSelections.has(selection)) {
  throw new Error(`invalid ARBORCORE_WEBGPU_ISOLATED_BROWSER=${selection}`);
}

const browserSpecs = [];
function addBrowser(kind, executable) {
  if (selection !== 'all' && selection !== kind) return;
  if (executable && fs.existsSync(executable)) browserSpecs.push({ kind, executable });
}
addBrowser('firefox', process.env.FIREFOX_PATH || '/usr/bin/firefox');
addBrowser('chrome', process.env.GOOGLE_CHROME_STABLE_PATH || '/usr/bin/google-chrome-stable');
if (browserSpecs.length === 0) {
  console.error('W4_ISOLATED_BROWSER_RESULT=REVIEW_NO_SUPPORTED_BROWSER');
  process.exit(2);
}

function contentType(file) {
  if (file.endsWith('.wasm')) return 'application/wasm';
  if (file.endsWith('.js') || file.endsWith('.mjs')) return 'text/javascript; charset=utf-8';
  if (file.endsWith('.html')) return 'text/html; charset=utf-8';
  return 'application/octet-stream';
}

async function runOne(spec) {
  let reportResolve;
  let reportReject;
  let reportSettled = false;
  let intentionalStop = false;
  let pageRequestSeen = false;
  let requestCount = 0;
  let childExitCode = null;
  let childExitSignal = null;

  const reportPromise = new Promise((resolve, reject) => {
    reportResolve = value => {
      if (reportSettled) return;
      reportSettled = true;
      resolve(value);
    };
    reportReject = error => {
      if (reportSettled) return;
      reportSettled = true;
      reject(error);
    };
  });

  const server = http.createServer((req, res) => {
    requestCount += 1;
    if (req.method === 'POST' && req.url === '/report') {
      let data = '';
      req.setEncoding('utf8');
      req.on('data', chunk => { data += chunk; });
      req.on('end', () => {
        try {
          const parsed = JSON.parse(data);
          res.writeHead(204);
          res.end();
          reportResolve(parsed);
        } catch (error) {
          res.writeHead(400);
          res.end();
          reportReject(error);
        }
      });
      return;
    }

    if (req.url && req.url.startsWith('/webgpu_accelerator_browser_test.html')) {
      pageRequestSeen = true;
    }
    const pathname = req.url === '/' ? '/webgpu_accelerator_browser_test.html' : String(req.url || '').split('?', 1)[0];
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
    server.listen(0, '127.0.0.1', resolve);
  });
  const port = server.address().port;
  const url = `http://127.0.0.1:${port}/webgpu_accelerator_browser_test.html?qualification=isolated`;
  const profile = fs.mkdtempSync(path.join(os.tmpdir(), `arborcore-webgpu-${spec.kind}-`));

  const firefoxHeadless = process.env.ARBORCORE_WEBGPU_ISOLATED_FIREFOX_HEADLESS === '1';
  let args;
  let launchMode;
  if (spec.kind === 'chrome') {
    launchMode = 'headless-clean-profile';
    args = [
      '--headless=new', '--no-sandbox', '--disable-dev-shm-usage',
      '--enable-logging=stderr', `--user-data-dir=${profile}`, url
    ];
  } else if (firefoxHeadless) {
    launchMode = 'headless-clean-profile-diagnostic';
    args = [
      '--headless', '--no-remote', '--profile', profile, url
    ];
  } else {
    launchMode = 'headed-clean-profile';
    args = [
      '--no-remote', '--new-instance', '--profile', profile, url
    ];
  }

  console.log(`browser=${spec.kind} launch_mode=${launchMode}`);
  const childEnv = { ...process.env };
  if (spec.kind === 'firefox' && firefoxHeadless) childEnv.MOZ_HEADLESS = '1';
  else delete childEnv.MOZ_HEADLESS;

  const child = spawn(spec.executable, args, {
    stdio: ['ignore', 'pipe', 'pipe'],
    env: childEnv
  });
  let stderr = '';
  let stdout = '';
  child.stderr.on('data', chunk => { stderr += chunk.toString(); });
  child.stdout.on('data', chunk => { stdout += chunk.toString(); });
  child.once('error', error => reportReject(error));
  child.once('exit', (code, signal) => {
    childExitCode = code;
    childExitSignal = signal;
    if (!intentionalStop && !reportSettled) {
      reportReject(new Error(
        `${spec.kind} exited before report (code=${code ?? 'null'}, signal=${signal ?? 'null'}, ` +
        `launch_mode=${launchMode}, page_request_seen=${pageRequestSeen}, request_count=${requestCount})`));
    }
  });

  const timeoutMs = Number(process.env.ARBORCORE_WEBGPU_ISOLATED_TIMEOUT_MS || '45000');
  const timeout = setTimeout(() => reportReject(new Error(
    `${spec.kind} WebGPU browser report timeout after ${timeoutMs} ms ` +
    `(launch_mode=${launchMode}, page_request_seen=${pageRequestSeen}, request_count=${requestCount}, ` +
    `exit_code=${childExitCode ?? 'running'}, exit_signal=${childExitSignal ?? 'none'})`)), timeoutMs);

  try {
    const result = await reportPromise;
    clearTimeout(timeout);
    fs.writeFileSync(path.join(outRoot, `${spec.kind}.json`), JSON.stringify(result, null, 2) + '\n');
    if (result.result !== 'PASS' || result.qualificationMode !== 'isolated' || result.secureContext !== true) {
      throw new Error(`${spec.kind} WebGPU/adaptive selftest failed: ${result.error || 'unknown failure'}`);
    }
    console.log(`browser=${spec.kind} page_request_seen=${pageRequestSeen}`);
    console.log(`browser=${spec.kind} request_count=${requestCount}`);
    return result;
  } finally {
    clearTimeout(timeout);
    intentionalStop = true;
    if (child.exitCode === null && child.signalCode === null) child.kill('SIGTERM');
    server.close();
    try { fs.rmSync(profile, { recursive: true, force: true }); } catch (_) {}
    if (stderr) fs.writeFileSync(path.join(outRoot, `${spec.kind}.stderr.txt`), stderr);
    if (stdout) fs.writeFileSync(path.join(outRoot, `${spec.kind}.stdout.txt`), stdout);
  }
}

const reports = {};
for (const spec of browserSpecs) {
  reports[spec.kind] = await runOne(spec);
  const report = reports[spec.kind];
  console.log(`browser=${spec.kind} result=${report.result}`);
  console.log(`browser_${spec.kind}_webgpu_api=${report.webGpuApiAvailable}`);
  console.log(`browser_${spec.kind}_qualified_mode=${report.qualifiedMode}`);
  console.log(`browser_${spec.kind}_canvas_format=${report.capability?.canvasFormat || ''}`);
  console.log(`browser_${spec.kind}_adapter_vendor=${report.capability?.adapterInfo?.vendor || ''}`);
  console.log(`browser_${spec.kind}_adapter_device=${report.capability?.adapterInfo?.device || ''}`);
  console.log(`browser_${spec.kind}_upload_rgba8_sha256=${report.uploadReadbackHash || ''}`);
  console.log(`browser_${spec.kind}_opaque_webgpu_sha256=${report.opaqueWebGpuHash || ''}`);
  console.log(`browser_${spec.kind}_fallback_sha256=${report.fallbackAfterLossHash || ''}`);
}

const entries = Object.entries(reports);
const webgpuEntries = entries.filter(([, report]) => report.qualifiedMode === 'webgpu');
const fallbackEntries = entries.filter(([, report]) => report.qualifiedMode !== 'webgpu');
for (const [kind, report] of webgpuEntries) {
  if (report.uploadReadbackHash !== EXPECTED_RGBA8) {
    throw new Error(`${kind} WebGPU source upload/readback did not match frozen B1`);
  }
  if (report.opaqueWebGpuHash !== EXPECTED_OPAQUE ||
      report.fallbackAfterLossHash !== EXPECTED_OPAQUE ||
      report.recoveredOpaqueHash !== EXPECTED_OPAQUE ||
      report.deviceLossObserved !== true || report.recoveryResult !== true) {
    throw new Error(`${kind} WebGPU lifecycle/equivalence evidence incomplete`);
  }
}
for (const [kind, report] of fallbackEntries) {
  if (report.fallbackAfterLossHash !== EXPECTED_OPAQUE) {
    throw new Error(`${kind} frozen Canvas2D fallback evidence mismatch`);
  }
}

const lines = [
  'W4_ISOLATED_BROWSER_RESULT=PASS',
  `W4_ISOLATED_BROWSER_COUNT=${entries.length}`,
  `W4_ISOLATED_WEBGPU_BROWSER_COUNT=${webgpuEntries.length}`,
  `W4_ISOLATED_FALLBACK_BROWSER_COUNT=${fallbackEntries.length}`,
  ...entries.flatMap(([kind, report]) => {
    const upper = kind.toUpperCase();
    return [
      `W4_ISOLATED_${upper}_WEBGPU_API=${report.webGpuApiAvailable}`,
      `W4_ISOLATED_${upper}_QUALIFIED_MODE=${report.qualifiedMode}`,
      `W4_ISOLATED_${upper}_CANVAS_FORMAT=${report.capability?.canvasFormat || 'NONE'}`,
      `W4_ISOLATED_${upper}_ADAPTER_VENDOR=${String(report.capability?.adapterInfo?.vendor || 'UNKNOWN').replace(/\s+/g, '_')}`,
      `W4_ISOLATED_${upper}_ADAPTER_DEVICE=${String(report.capability?.adapterInfo?.device || 'UNKNOWN').replace(/\s+/g, '_')}`,
      `W4_ISOLATED_${upper}_UPLOAD_RGBA8_SHA256=${report.uploadReadbackHash || 'NOT_APPLICABLE'}`,
      `W4_ISOLATED_${upper}_OPAQUE_WEBGPU_SHA256=${report.opaqueWebGpuHash || 'NOT_APPLICABLE'}`,
      `W4_ISOLATED_${upper}_FALLBACK_SHA256=${report.fallbackAfterLossHash || 'NOT_APPLICABLE'}`,
      `W4_ISOLATED_${upper}_DEVICE_LOSS_OBSERVED=${report.deviceLossObserved}`,
      `W4_ISOLATED_${upper}_RECOVERY_RESULT=${report.recoveryResult}`,
      `W4_ISOLATED_${upper}_RECOVERED_OPAQUE_SHA256=${report.recoveredOpaqueHash || 'NOT_APPLICABLE'}`,
      `W5_ISOLATED_${upper}_SUBMIT_640X360_MEDIAN_MS=${report.submit640x360MedianMs ?? 'NOT_APPLICABLE'}`,
      `W5_ISOLATED_${upper}_COMPLETE_640X360_MEDIAN_MS=${report.complete640x360MedianMs ?? 'NOT_APPLICABLE'}`
    ];
  })
];
fs.writeFileSync(path.join(outRoot, 'result.env'), lines.join('\n') + '\n');
console.log(`W4_ISOLATED_WEBGPU_BROWSER_COUNT=${webgpuEntries.length}`);
console.log('W4_ISOLATED_BROWSER_RESULT=PASS');
