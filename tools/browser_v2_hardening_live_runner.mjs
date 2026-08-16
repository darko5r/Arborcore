import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';

const root=process.env.ARBORCORE_ROOT||process.cwd();
const timeoutMs=Number(process.env.ARBORCORE_BV2H_LIVE_TIMEOUT_MS||900000);
const out=path.join(root,'build/browser-v2-hardening-live');
const sourceSha=process.env.ARBORCORE_BV2H_SOURCE_SHA256||'';
const wasmSha=process.env.ARBORCORE_BV2H_WASM_SHA256||'';
const sha256=/^[0-9a-f]{64}$/;
if(!sha256.test(sourceSha))throw new Error('missing/invalid ARBORCORE_BV2H_SOURCE_SHA256');
if(!sha256.test(wasmSha))throw new Error('missing/invalid ARBORCORE_BV2H_WASM_SHA256');

const files=new Map([
  ['/browser/arborcore_host.js',['browser/arborcore_host.js','text/javascript; charset=utf-8']],
  ['/browser/shaders/rgba8_present.wgsl',['browser/shaders/rgba8_present.wgsl','text/plain; charset=utf-8']],
  ['/browser/shaders/rgba16_exact_convert.wgsl',['browser/shaders/rgba16_exact_convert.wgsl','text/plain; charset=utf-8']],
  ['/tests/js/browser_language_boundary_v2_experiment.mjs',['tests/js/browser_language_boundary_v2_experiment.mjs','text/javascript; charset=utf-8']],
  ['/tests/js/browser_v2_hardening_runtime.mjs',['tests/js/browser_v2_hardening_runtime.mjs','text/javascript; charset=utf-8']],
  ['/tests/data/browser_v2_dpr_vectors.json',['tests/data/browser_v2_dpr_vectors.json','application/json; charset=utf-8']],
  ['/arborcore-browser-v2-hardening.wasm',['build/browser-v2-hardening-live/arborcore-browser-v2-hardening.wasm','application/wasm']],
  ['/browser_v2_hardening_diagnostics.html',['tests/browser/browser_v2_hardening_diagnostics.html','text/html; charset=utf-8']]
]);
const reports=new Map();
function browserKind(ua){if(/Firefox\//.test(ua))return'firefox';if(/Chrome\//.test(ua)&&!/Edg\//.test(ua))return'chrome';return'other';}
function validateReport(kind,r){
  if(r.result!=='PASS')throw new Error(`${kind}: report failed`);
  if(r.authoritativeJsLogic!=='ZERO')throw new Error(`${kind}: authoritative JS boundary violated`);
  if(r.wasm?.importCount!==0)throw new Error(`${kind}: WASM imports not zero`);
  if(r.dpr?.passCount!==r.dpr?.totalCount||!r.dpr?.totalCount)throw new Error(`${kind}: DPR vectors incomplete`);
  if(r.opt3ContractAdmission!=='CANDIDATE_NOT_ADMITTED_REQUIRES_BV2H4_REVIEW')throw new Error(`${kind}: OPT3 admission marker invalid`);
  if(r.broadBrowserReleaseClaim!=='NOT_ADMITTED_WEBKIT_SAFARI_NOT_FORMALLY_QUALIFIED')throw new Error(`${kind}: broad release claim invalid`);
  if(r.qualifiedMode==='webgpu'&&!r.opt3Exact)throw new Error(`${kind}: WebGPU without exact OPT3 evidence`);
}
function finish(server){
  if(!reports.has('firefox')||!reports.has('chrome'))return;
  fs.mkdirSync(out,{recursive:true});
  const f=reports.get('firefox'),c=reports.get('chrome'); validateReport('firefox',f);validateReport('chrome',c);
  const webgpuCount=[f,c].filter(r=>r.qualifiedMode==='webgpu').length;if(webgpuCount<1)throw new Error('BV2H requires at least one live WebGPU browser');
  const lines=[
    'BV2H_LIVE_BROWSER_RESULT=PASS','BV2H_LIVE_BROWSER_COUNT=2',`BV2H_LIVE_WEBGPU_BROWSER_COUNT=${webgpuCount}`,
    `BV2H_LIVE_SOURCE_SHA256=${sourceSha}`,`BV2H_LIVE_WASM_SHA256=${wasmSha}`,
    `BV2H_LIVE_FIREFOX_MODE=${f.qualifiedMode}`,`BV2H_LIVE_FIREFOX_FAILURE_CLASS=${f.webGpuFailureClass}`,`BV2H_LIVE_FIREFOX_OPT3_EXACT=${f.opt3Exact}`,`BV2H_LIVE_FIREFOX_RECOVERY=${f.lifecycleRecovery?.result||'N/A'}`,
    `BV2H_LIVE_CHROME_MODE=${c.qualifiedMode}`,`BV2H_LIVE_CHROME_FAILURE_CLASS=${c.webGpuFailureClass}`,`BV2H_LIVE_CHROME_OPT3_EXACT=${c.opt3Exact}`,`BV2H_LIVE_CHROME_RECOVERY=${c.lifecycleRecovery?.result||'N/A'}`,
    'BV2H_AUTHORITATIVE_JS_LOGIC=ZERO','BV2H_OPT3_ADMISSION=CANDIDATE_NOT_ADMITTED_REQUIRES_BV2H4_REVIEW','BV2H_WEBKIT_SAFARI=NOT_FORMALLY_QUALIFIED','BV2H_LIVE_STATE=QUALIFIED_CANDIDATE_NOT_FROZEN'
  ];
  fs.writeFileSync(path.join(out,'result.env'),lines.join('\n')+'\n');
  fs.writeFileSync(path.join(out,'firefox.json'),JSON.stringify(f,null,2)+'\n');
  fs.writeFileSync(path.join(out,'chrome.json'),JSON.stringify(c,null,2)+'\n');
  console.log(lines.join('\n'));console.log('PASS: BV2H8 normal Firefox/Chrome live qualification matrix bound to exact candidate/module');server.close(()=>process.exit(0));
}
const server=http.createServer((req,res)=>{
  const url=new URL(req.url,'http://127.0.0.1');
  if(req.method==='POST'&&url.pathname==='/report'){
    let body='';req.setEncoding('utf8');req.on('data',c=>body+=c);req.on('end',()=>{try{const report=JSON.parse(body),kind=browserKind(report.userAgent||'');if(kind==='other')throw new Error('unsupported live browser');if(!reports.has(kind)){reports.set(kind,report);console.log(`BV2H_LIVE_REPORT_RECEIVED=${kind}`);}res.writeHead(204);res.end();finish(server);}catch(e){res.writeHead(400);res.end(String(e));}});return;
  }
  const item=files.get(url.pathname==='/'?'/browser_v2_hardening_diagnostics.html':url.pathname);if(!item){res.writeHead(404);res.end('not found');return;}
  try{const [rel,type]=item,data=fs.readFileSync(path.join(root,rel));res.writeHead(200,{'content-type':type,'cache-control':'no-store','cross-origin-opener-policy':'same-origin'});res.end(data);}catch(e){res.writeHead(500);res.end(String(e));}
});
server.listen(0,'127.0.0.1',()=>{const{port}=server.address();console.log(`BV2H_LIVE_URL=http://127.0.0.1:${port}/browser_v2_hardening_diagnostics.html?qualification=live`);console.log('BV2H_LIVE_ACTION=OPEN_URL_IN_NORMAL_FIREFOX_AND_NORMAL_CHROME');console.log('BV2H_LIVE_WAITING_FOR=firefox,chrome');});
setTimeout(()=>{console.error('FAIL: BV2H live browser timeout');server.close(()=>process.exit(2));},timeoutMs).unref();
