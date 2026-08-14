import fs from 'node:fs';
import assert from 'node:assert/strict';

const modulePath = process.argv[2];

const vectorPath = new URL(
  '../data/browser_v1_precision_vectors.json',
  import.meta.url,
);

const vectors = JSON.parse(
  fs.readFileSync(vectorPath, 'utf8'),
);

assert.equal(
  vectors.schema,
  'ARBORCORE_BROWSER_V1_PRECISION_VECTORS_1',
);

assert.equal(
  vectors.source.freezeCommit,
  '80395372243fcfb2995ed03beca8a3af3e873964',
);

assert.equal(
  vectors.source.gitBlob,
  'e5595c063ff438a63efc9dfb7c324c60d0099e3f',
);

assert.equal(
  vectors.source.sha256,
  'e6e228daaf3cd220a727e5622fdca5bf24f8ff1fc67816e76adcf35f39ccf13e',
);

const bytes = fs.readFileSync(modulePath);
const moduleObject = new WebAssembly.Module(bytes);

assert.equal(
  WebAssembly.Module.imports(moduleObject).length,
  0,
);

const ex = new WebAssembly.Instance(
  moduleObject,
  {},
).exports;

const decoder = new TextDecoder();

function v2Css(raw) {
  const ptr =
    ex.arbor_browser_host_v2_css_scratch_ptr();

  const cap =
    ex.arbor_browser_host_v2_css_scratch_bytes();

  const status =
    ex.arbor_browser_host_v2_format_q32_css(
      raw,
      ptr,
      cap,
      ex.arbor_browser_host_v2_written_scratch_ptr(),
    );

  assert.equal(status, 0);

  const data = new Uint8Array(
    ex.memory.buffer,
    ptr,
    cap,
  );

  let length = 0;

  while (
    length < data.length &&
    data[length] !== 0
  ) {
    length += 1;
  }

  return decoder.decode(
    data.subarray(0, length),
  );
}

for (const vector of vectors.q32Css) {
  const raw = BigInt(vector.raw);

  assert.equal(
    v2Css(raw),
    vector.css,
    `frozen Browser v1 Q32 ${vector.raw}`,
  );
}

function v2Size(entry, dpr) {
  const device =
    entry.devicePixelContentBoxSize?.[0] ||
    null;

  const content =
    entry.contentBoxSize?.[0] ||
    null;

  const status =
    ex.arbor_browser_host_v2_resolve_device_size(
      device ? 1 : 0,
      device?.inlineSize || 0,
      device?.blockSize || 0,
      content?.inlineSize ||
        entry.contentRect?.width ||
        0,
      content?.blockSize ||
        entry.contentRect?.height ||
        0,
      dpr,
      ex.arbor_browser_host_v2_size_scratch_ptr(),
    );

  assert.equal(status, 0);

  return {
    width:
      ex.arbor_browser_host_v2_size_width(),

    height:
      ex.arbor_browser_host_v2_size_height(),
  };
}

for (const vector of vectors.deviceSize) {
  const result =
    v2Size(vector.entry, vector.dpr);

  assert.equal(
    result.width,
    vector.expectedWidth,
    `${vector.name} width`,
  );

  assert.equal(
    result.height,
    vector.expectedHeight,
    `${vector.name} height`,
  );
}

console.log(
  'PASS: Browser Language Boundary v2 C/WASM numerical behavior matches frozen Browser v1 vector fixture',
);
