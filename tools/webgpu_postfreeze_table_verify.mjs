import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const root = process.env.ARBORCORE_ROOT || path.resolve(path.dirname(new URL(import.meta.url).pathname), '..');
const tableModule = await import(pathToFileURL(path.join(root, 'browser', 'webgpu_rgba16_exact_tables.js')).href);

function parseBucket(text) {
  const match = text.match(/arbor_browser_linear16_to_srgb8_bucket12\[[^\]]+\]\s*=\s*\{([\s\S]*?)\};/);
  if (!match) throw new Error('frozen bucket12 table not found');
  return Array.from(match[1].matchAll(/\b\d+\b/g), m => Number(m[0]));
}

function parseForward(text) {
  const match = text.match(/arbor_srgb8_to_linear16\[256\]\s*=\s*\{([\s\S]*?)\};/);
  if (!match) throw new Error('frozen sRGB8->linear16 table not found');
  return Array.from(match[1].matchAll(/UINT16_C\((\d+)\)/g), m => Number(m[1]));
}

const bucket = parseBucket(fs.readFileSync(path.join(root, 'browser', 'linear16_srgb8_bucket12.h'), 'utf8'));
const forward = parseForward(fs.readFileSync(path.join(root, 'renderer', 'srgb8_linear16_lut.h'), 'utf8'));
const jsBucket = Array.from(tableModule.FROZEN_LINEAR16_TO_SRGB8_BUCKET12);
const jsForward = Array.from(tableModule.FROZEN_SRGB8_TO_LINEAR16);

if (bucket.length !== 4096 || forward.length !== 256) throw new Error('frozen table lengths are unexpected');
if (JSON.stringify(bucket) !== JSON.stringify(jsBucket)) throw new Error('post-W6 bucket12 table differs from frozen B1');
if (JSON.stringify(forward) !== JSON.stringify(jsForward)) throw new Error('post-W6 forward table differs from frozen renderer');
console.log('POSTFREEZE_BUCKET12_COUNT=4096');
console.log('POSTFREEZE_FORWARD_LUT_COUNT=256');
console.log('PASS: post-W6 GPU experiment tables are exact frozen-table copies');
