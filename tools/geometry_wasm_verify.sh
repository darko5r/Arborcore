#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
OUT="$ROOT/build/geometry-g4-wasm"
MODULE="$OUT/arborcore-geometry-v1.wasm"

command -v clang >/dev/null 2>&1 || {
  echo "G4_WASM_RUNTIME_RESULT=REVIEW_NO_CLANG" >&2
  exit 1
}
command -v node >/dev/null 2>&1 || {
  echo "G4_WASM_RUNTIME_RESULT=REVIEW_NO_NODE_RUNTIME" >&2
  exit 1
}

mkdir -p "$OUT"
clang --target=wasm32 \
  -I"$ROOT/include" \
  -std=c17 -O2 -ffreestanding -fno-builtin -nostdlib \
  "$ROOT/src/c/geometry.c" \
  "$ROOT/src/wasm/geometry_int128_builtins.c" \
  "$ROOT/tests/c/geometry_wasm_selftest.c" \
  -Wl,--no-entry \
  -Wl,--export=arbor_geometry_wasm_selftest \
  -Wl,--strip-all \
  -o "$MODULE"

node - "$MODULE" <<'NODE'
const fs = require('fs');
const path = process.argv[2];
const bytes = fs.readFileSync(path);
const moduleObject = new WebAssembly.Module(bytes);
const imports = WebAssembly.Module.imports(moduleObject);
if (imports.length !== 0) {
  console.error(`FAIL: WASM geometry module has ${imports.length} imports`);
  process.exit(2);
}
const instance = new WebAssembly.Instance(moduleObject, {});
const value = instance.exports.arbor_geometry_wasm_selftest();
console.log(`wasm_import_count=${imports.length}`);
console.log(`wasm_selftest_result=${value}`);
if (value !== 0) process.exit(value);
NODE

sha="$(sha256sum "$MODULE" | awk '{print $1}')"
printf 'geometry_wasm_sha256=%s\n' "$sha"
cat > "$OUT/result.env" <<EVIDENCE
G4_WASM_RUNTIME_RESULT=PASS
GEOMETRY_WASM_SHA256=$sha
EVIDENCE
echo "G4_WASM_RUNTIME_RESULT=PASS"
