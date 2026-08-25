#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"

echo '### VIEW0 V1N0 SCOPE / BASELINE / CONTRACT'
bash tools/view0_v1_scope_verify.sh
bash tools/view0_v1_baseline_verify.sh
bash tools/view0_v1_contract_verify.sh

echo '### VIEW0 V1N0 EXACT SOURCE REVERIFICATION / CLEAN DERIVED-BUILD RESET'
bash tools/view0_v1_lexbor_acquire.sh

rm -rf \
    build/view0-v1/native/lexbor-compat-src \
    build/view0-v1/native/lexbor-build-release \
    build/view0-v1/native/lexbor-build-sanitize
rm -f \
    build/view0-v1/native/lexbor-compat-source-manifest.sha256
rm -f \
    build/view0-v1/native/native.o \
    build/view0-v1/native/lexbor_adapter.o \
    build/view0-v1/native/main.o \
    build/view0-v1/native/foundation_test.o \
    build/view0-v1/native/foundation_adversarial_test.o \
    build/view0-v1/native/arborcore-view0-html-check \
    build/view0-v1/native/foundation-test \
    build/view0-v1/native/foundation-adversarial-test \
    build/view0-v1/native/foundation-sanitize-test

for stale in \
    build/view0-v1/native/lexbor-compat-src \
    build/view0-v1/native/lexbor-compat-source-manifest.sha256 \
    build/view0-v1/native/lexbor-build-release \
    build/view0-v1/native/lexbor-build-sanitize \
    build/view0-v1/native/native.o \
    build/view0-v1/native/lexbor_adapter.o \
    build/view0-v1/native/main.o \
    build/view0-v1/native/foundation_test.o \
    build/view0-v1/native/foundation_adversarial_test.o \
    build/view0-v1/native/arborcore-view0-html-check \
    build/view0-v1/native/foundation-test \
    build/view0-v1/native/foundation-adversarial-test \
    build/view0-v1/native/foundation-sanitize-test; do
    [[ ! -e "$stale" ]] || {
        echo "FAIL: V1N0 derived build output survived authoritative clean reset: $stale" >&2
        exit 1
    }
done

echo 'VIEW0_V1N0_AUTHORITATIVE_EXACT_SOURCE_REVERIFY=PASS'
echo 'VIEW0_V1N0_AUTHORITATIVE_DERIVED_BUILD_RESET=PASS'
echo 'PASS: authoritative V1N0 gate starts from exact source and no prior V1N0 derived binaries'

echo '### VIEW0 M1 COMPLETE REGRESSION UNDER NATIVE V1 FOUNDATION'
bash tools/view0_m1_gate.sh
echo 'PASS: source-review-closed M1 production semantics preserved under V1N0'

echo '### VIEW0 V1 GENERATED ARTIFACT REGRESSION UNDER NATIVE AUTHORITY'
bash tools/view0_v1_native_verify.sh
echo 'PASS: template/native-C/NASM canonical artifact generation preserved under native authority'

echo '### VIEW0 V1N0 NATIVE C / LEXBOR FOUNDATION'
bash tools/view0_v1_native_foundation_verify.sh

[[ -x build/view0-v1/native/arborcore-view0-html-check ]] ||
    { echo 'FAIL: fresh release checker was not rebuilt' >&2; exit 1; }
[[ -f build/view0-v1/native/lexbor-build-release/liblexbor_static.a ]] ||
    { echo 'FAIL: fresh release Lexbor library was not rebuilt' >&2; exit 1; }
[[ "$(sha256sum build/view0-v1/native/lexbor-compat-src/source/lexbor/html/tree/insertion_mode/in_body.c | awk '{print $1}')" == '142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8' ]] ||
    { echo 'FAIL: release derived Lexbor ruby compatibility identity mismatch' >&2; exit 1; }
echo 'VIEW0_V1N0_LX1_RELEASE_COMPATIBILITY_REBUILD=PASS'
echo 'VIEW0_V1N0_AUTHORITATIVE_RELEASE_REBUILD=PASS'

echo '### VIEW0 V1N0 SANITIZERS — ARBORCORE ADAPTER + SANITIZED LEXBOR'
make -s view0-v1n0-sanitize
[[ -x build/view0-v1/native/foundation-sanitize-test ]] ||
    { echo 'FAIL: fresh sanitizer checker was not rebuilt' >&2; exit 1; }
[[ -f build/view0-v1/native/lexbor-build-sanitize/liblexbor_static.a ]] ||
    { echo 'FAIL: fresh sanitized Lexbor library was not rebuilt' >&2; exit 1; }
[[ "$(sha256sum build/view0-v1/native/lexbor-compat-src/source/lexbor/html/tree/insertion_mode/in_body.c | awk '{print $1}')" == '142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8' ]] ||
    { echo 'FAIL: sanitized derived Lexbor ruby compatibility identity mismatch' >&2; exit 1; }
echo 'VIEW0_V1N0_LX1_SANITIZE_COMPATIBILITY_REBUILD=PASS'
echo 'VIEW0_V1N0_AUTHORITATIVE_SANITIZE_REBUILD=PASS'

echo '### VIEW0 V1N0 FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]]
echo 'VIEW0_V1N0_GATE=PASS'
echo 'VIEW0_V1N0_STAGED_CHANGES=NO'
echo 'VIEW0_V1N0_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N0_REMOTE_WRITE_PERFORMED=NO'
