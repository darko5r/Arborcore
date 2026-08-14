import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';

const root = process.env.ARBORCORE_ROOT || process.cwd();
const timeoutMs = Number(process.env.ARBORCORE_LBV2_LIVE_TIMEOUT_MS || 900000);
const files = new Map([
  ['/browser/arborcore_host.js', ['browser/arborcore_host.js', 'text/javascript; charset=utf-8']],
  ['/browser/shaders/rgba8_present.wgsl', ['browser/shaders/rgba8_present.wgsl', 'text/plain; charset=utf-8']],
  ['/browser/shaders/rgba16_exact_convert.wgsl', ['browser/shaders/rgba16_exact_convert.wgsl', 'text/plain; charset=utf-8']],
  ['/tests/js/browser_language_boundary_v2_experiment.mjs', ['tests/js/browser_language_boundary_v2_experiment.mjs', 'text/javascript; charset=utf-8']],
  ['/arborcore-browser-language-boundary-v2.wasm', ['build/browser-language-boundary-v2-live/arborcore-browser-language-boundary-v2.wasm', 'application/wasm']],
  ['/browser_language_boundary_v2_test.html', ['tests/browser/browser_language_boundary_v2_test.html', 'text/html; charset=utf-8']]
]);
const reports = new Map();
const outDir = path.join(root, 'build/browser-language-boundary-v2-live');
fs.mkdirSync(outDir, { recursive: true });
for (const name of ['result.env', 'firefox.json', 'chrome.json', 'failure.txt']) {
  fs.rmSync(path.join(outDir, name), { force: true });
}
let finishing = false;
function browserKind(ua) {
  if (/Firefox\//.test(ua)) return 'firefox';
  if (/Chrome\//.test(ua) && !/Edg\//.test(ua)) return 'chrome';
  return 'other';
}
function finish(server) {
  if (finishing || !reports.has('firefox') || !reports.has('chrome')) return;
  finishing = true;
  const f = reports.get('firefox'), c = reports.get('chrome');
  fs.writeFileSync(path.join(outDir, 'firefox.json'), JSON.stringify(f, null, 2) + '\n');
  fs.writeFileSync(path.join(outDir, 'chrome.json'), JSON.stringify(c, null, 2) + '\n');
  try {
    if (f.result !== 'PASS' || c.result !== 'PASS') throw new Error('LBv2 live-browser report failed');
    const webgpuCount = [f, c].filter(r => r.qualifiedMode === 'webgpu').length;
    if (webgpuCount < 1) throw new Error('LBv2 requires at least one live WebGPU browser');
    if (f.authoritativeJsLogic !== 'ZERO' || c.authoritativeJsLogic !== 'ZERO') throw new Error('authoritative JS boundary violated');
    const lines = [
      'LBV2_LIVE_BROWSER_RESULT=PASS',
      'LBV2_LIVE_BROWSER_COUNT=2',
      `LBV2_LIVE_WEBGPU_BROWSER_COUNT=${webgpuCount}`,
      `LBV2_LIVE_FIREFOX_MODE=${f.qualifiedMode}`,
      `LBV2_LIVE_FIREFOX_OPT3_EXACT=${f.opt3Exact}`,
      `LBV2_LIVE_CHROME_MODE=${c.qualifiedMode}`,
      `LBV2_LIVE_CHROME_FAILURE_CLASS=${c.webGpuFailureClass}`,
      'LBV2_AUTHORITATIVE_JS_LOGIC=ZERO',
      'LBV2_BROAD_BROWSER_RELEASE_CLAIM=NOT_ADMITTED_WEBKIT_SAFARI_NOT_FORMALLY_QUALIFIED'
    ];
    fs.writeFileSync(path.join(outDir, 'result.env'), lines.join('\n') + '\n');
    console.log(lines.join('\n'));
    console.log('PASS: Browser Language Boundary v2 live Firefox/Chrome qualification');
    server.close(() => process.exit(0));
  } catch (error) {
    const message = String(error?.stack || error);
    fs.writeFileSync(path.join(outDir, 'failure.txt'), message + '\n');
    console.error(`FAIL: ${message}`);
    server.close(() => process.exit(1));
  }
}
const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://127.0.0.1');
  if (req.method === 'POST' && url.pathname === '/report') {
    let body=''; req.setEncoding('utf8');
    req.on('data', c => body += c);
    req.on('end', () => {
      try {
        const report = JSON.parse(body); const kind = browserKind(report.userAgent || '');
        if (kind === 'other') throw new Error('unsupported live browser');
        if (!reports.has(kind)) { reports.set(kind, report); console.log(`LBV2_LIVE_REPORT_RECEIVED=${kind}`); }
        res.writeHead(204); res.end();
      } catch (error) {
        if (!res.headersSent) res.writeHead(400);
        if (!res.writableEnded) res.end(String(error));
        return;
      }
      finish(server);
    }); return;
  }
  const item = files.get(url.pathname === '/' ? '/browser_language_boundary_v2_test.html' : url.pathname);
  if (!item) { res.writeHead(404); res.end('not found'); return; }
  const [rel, type] = item; const data = fs.readFileSync(path.join(root, rel));
  res.writeHead(200, { 'content-type': type, 'cache-control': 'no-store' }); res.end(data);
});
server.listen(0, '127.0.0.1', () => {
  const { port } = server.address();
  console.log(`LBV2_LIVE_URL=http://127.0.0.1:${port}/browser_language_boundary_v2_test.html?qualification=live`);
  console.log('LBV2_LIVE_ACTION=OPEN_IN_NORMAL_FIREFOX_AND_NORMAL_CHROME');
  console.log('LBV2_LIVE_WAITING_FOR=firefox,chrome');
});
setTimeout(() => { console.error('FAIL: LBv2 live browser timeout'); server.close(() => process.exit(2)); }, timeoutMs).unref();
