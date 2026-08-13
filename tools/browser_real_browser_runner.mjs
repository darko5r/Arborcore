import http from 'node:http';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const root = process.env.ARBORCORE_ROOT || path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const webRoot = path.join(root, 'build', 'browser-b0-b6-web');
const outRoot = path.join(root, 'build', 'browser-b0-b6-browser');
fs.mkdirSync(outRoot, { recursive: true });

const browserSpecs = [];
function addBrowser(kind, executable) {
  if (executable && fs.existsSync(executable)) browserSpecs.push({ kind, executable });
}

addBrowser('firefox', process.env.FIREFOX_PATH || '/usr/bin/firefox');
addBrowser('chrome', process.env.GOOGLE_CHROME_STABLE_PATH || '/usr/bin/google-chrome-stable');
if (browserSpecs.length === 0) {
  console.error('B5_BROWSER_RESULT=REVIEW_NO_SUPPORTED_BROWSER');
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
  const reportPromise = new Promise((resolve, reject) => {
    reportResolve = resolve;
    reportReject = reject;
  });

  const server = http.createServer((req, res) => {
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

    const pathname = req.url === '/' ? '/precision_surface_browser_test.html' : req.url;
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
  const url = `http://127.0.0.1:${port}/precision_surface_browser_test.html`;
  const profile = fs.mkdtempSync(path.join(os.tmpdir(), `arborcore-${spec.kind}-`));
  let args;
  if (spec.kind === 'chrome') {
    args = [
      '--headless=new', '--disable-gpu', '--no-sandbox', '--disable-dev-shm-usage',
      `--user-data-dir=${profile}`, url
    ];
  } else {
    args = ['--headless', '--no-remote', '--profile', profile, url];
  }

  const child = spawn(spec.executable, args, {
    stdio: ['ignore', 'pipe', 'pipe'],
    env: { ...process.env, MOZ_HEADLESS: '1' }
  });
  let stderr = '';
  child.stderr.on('data', chunk => { stderr += chunk.toString(); });

  const timeout = setTimeout(() => reportReject(new Error(`${spec.kind} browser report timeout`)), 20000);
  try {
    const result = await reportPromise;
    clearTimeout(timeout);
    fs.writeFileSync(path.join(outRoot, `${spec.kind}.json`), JSON.stringify(result, null, 2) + '\n');
    if (result.result !== 'PASS') {
      throw new Error(`${spec.kind} browser selftest failed: ${result.error || 'unknown failure'}`);
    }
    return result;
  } finally {
    clearTimeout(timeout);
    child.kill('SIGTERM');
    server.close();
    try { fs.rmSync(profile, { recursive: true, force: true }); } catch (_) {}
    if (stderr) fs.writeFileSync(path.join(outRoot, `${spec.kind}.stderr.txt`), stderr);
  }
}

const reports = {};
for (const spec of browserSpecs) {
  reports[spec.kind] = await runOne(spec);
  console.log(`browser=${spec.kind} result=${reports[spec.kind].result}`);
  console.log(`browser_${spec.kind}_rgba8_sha256=${reports[spec.kind].rgba8Hash}`);
  console.log(`browser_${spec.kind}_opaque_canvas_sha256=${reports[spec.kind].opaqueCanvasHash}`);
  console.log(`browser_${spec.kind}_float16_imagedata=${reports[spec.kind].float16ImageData}`);
  console.log(`browser_${spec.kind}_device_pixel_content_box=${reports[spec.kind].resizeObserverDevicePixelBox}`);
  console.log(`browser_${spec.kind}_presentation_640x360_median_ms=${reports[spec.kind].presentation640x360MedianMs}`);
}

fs.writeFileSync(path.join(outRoot, 'result.env'), [
  'B5_REAL_BROWSER_RESULT=PASS',
  `B5_BROWSER_COUNT=${Object.keys(reports).length}`,
  ...Object.entries(reports).flatMap(([kind, report]) => [
    `B5_${kind.toUpperCase()}_RGBA16_SHA256=${report.rgba16Hash}`,
    `B5_${kind.toUpperCase()}_RGBA8_SHA256=${report.rgba8Hash}`,
    `B5_${kind.toUpperCase()}_OPAQUE_CANVAS_SHA256=${report.opaqueCanvasHash}`,
    `B5_${kind.toUpperCase()}_FLOAT16_IMAGEDATA=${report.float16ImageData}`,
    `B5_${kind.toUpperCase()}_DEVICE_PIXEL_CONTENT_BOX=${report.resizeObserverDevicePixelBox}`,
    `B6_${kind.toUpperCase()}_PRESENTATION_640X360_MEDIAN_MS=${report.presentation640x360MedianMs}`
  ])
].join('\n') + '\n');
console.log('B5_REAL_BROWSER_RESULT=PASS');
