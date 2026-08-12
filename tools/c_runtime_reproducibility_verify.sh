#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
work="$(mktemp -d /tmp/arborcore-c-runtime-repro.XXXXXX)"
trap 'rm -rf "$work"' EXIT

copy_tree() {
    local dst="$1"
    mkdir -p "$dst"
    (cd "$ROOT" && tar \
        --exclude='./.git' \
        --exclude='./build' \
        --exclude='./generated' \
        -cf - .) | (cd "$dst" && tar -xf -)
}

copy_tree "$work/a"
copy_tree "$work/b"

make -C "$work/a" c-runtime-library >/dev/null
make -C "$work/b" c-runtime-library >/dev/null

ha="$(sha256sum "$work/a/build/libarborcore_runtime.a" | awk '{print $1}')"
hb="$(sha256sum "$work/b/build/libarborcore_runtime.a" | awk '{print $1}')"

if [[ "$ha" != "$hb" ]]; then
    echo "FAIL: C runtime archive is not byte-for-byte reproducible." >&2
    echo "build_a=$ha" >&2
    echo "build_b=$hb" >&2
    exit 1
fi

echo "c_runtime_reproducible_sha256=$ha"
echo "PASS: independent C runtime archive builds are byte-for-byte reproducible"
