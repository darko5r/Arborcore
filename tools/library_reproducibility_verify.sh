#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FREEZE="$ROOT/abi/arborcore-1.freeze"
[[ -r "$FREEZE" ]] || { echo "FAIL: missing $FREEZE" >&2; exit 2; }
# shellcheck disable=SC1090
source "$FREEZE"

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

snapshot_build() {
  local lane="$1"
  local dst="$tmp/$lane"
  mkdir -p "$dst"
  (
    cd "$ROOT"
    tar --exclude='./build' --exclude='./.git' -cf - .
  ) | tar -C "$dst" -xf -
  make -C "$dst" clean >/dev/null
  make -C "$dst" libarborcore-libraries >/dev/null
}

snapshot_build a
snapshot_build b

static_a="$tmp/a/build/libarborcore.a"
static_b="$tmp/b/build/libarborcore.a"
shared_a="$tmp/a/build/libarborcore.so.1.0.0"
shared_b="$tmp/b/build/libarborcore.so.1.0.0"

cmp -s "$static_a" "$static_b" || {
  echo "FAIL: independent static-library builds are not byte-for-byte reproducible." >&2
  exit 1
}
cmp -s "$shared_a" "$shared_b" || {
  echo "FAIL: independent shared-library builds are not byte-for-byte reproducible." >&2
  exit 1
}

static_sha="$(sha256sum "$static_a" | awk '{print $1}')"
shared_sha="$(sha256sum "$shared_a" | awk '{print $1}')"

[[ "$static_sha" == "$STATIC_LIBRARY_SHA256" ]] || {
  echo "FAIL: static library differs from frozen ABI artifact identity." >&2
  echo "expected=$STATIC_LIBRARY_SHA256" >&2
  echo "actual=$static_sha" >&2
  exit 1
}
[[ "$shared_sha" == "$SHARED_LIBRARY_SHA256" ]] || {
  echo "FAIL: shared library differs from frozen ABI artifact identity." >&2
  echo "expected=$SHARED_LIBRARY_SHA256" >&2
  echo "actual=$shared_sha" >&2
  exit 1
}

echo "repro_static_sha256=$static_sha"
echo "repro_shared_sha256=$shared_sha"
echo "PASS: independent library builds are byte-for-byte reproducible"
