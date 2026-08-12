#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
FREEZE="$ROOT/abi/arborcore-1.freeze"
[[ -r "$FREEZE" ]] || { echo "FAIL: missing $FREEZE" >&2; exit 2; }
# shellcheck disable=SC1090
source "$FREEZE"

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
stage="$tmp/stage"
prefix=/usr/local
libdir="$stage$prefix/lib"
abidir="$stage$prefix/share/arborcore/abi"

make -C "$ROOT" \
  DESTDIR="$stage" \
  PREFIX="$prefix" \
  LIBDIR="$prefix/lib" \
  DATADIR="$prefix/share" \
  install-libraries >/dev/null

required=(
  "$libdir/libarborcore.a"
  "$libdir/libarborcore.so.1.0.0"
  "$abidir/README.md"
  "$abidir/arborcore-1.freeze"
  "$abidir/arborcore-1.internal-symbols"
  "$abidir/arborcore-1.layout"
  "$abidir/arborcore-1.map"
  "$abidir/arborcore-1.symbols"
)
for f in "${required[@]}"; do
  [[ -f "$f" ]] || { echo "FAIL: staged install missing $f" >&2; exit 1; }
done
[[ -L "$libdir/libarborcore.so.1" ]] || { echo "FAIL: missing SONAME symlink" >&2; exit 1; }
[[ -L "$libdir/libarborcore.so" ]] || { echo "FAIL: missing development symlink" >&2; exit 1; }
[[ "$(readlink "$libdir/libarborcore.so.1")" == "libarborcore.so.1.0.0" ]] || {
  echo "FAIL: incorrect libarborcore.so.1 symlink target" >&2; exit 1;
}
[[ "$(readlink "$libdir/libarborcore.so")" == "libarborcore.so.1" ]] || {
  echo "FAIL: incorrect libarborcore.so symlink target" >&2; exit 1;
}

# The staged default-prefix install must contain exactly the canonical package
# manifest: no missing files and no accidental extras.
find "$stage$prefix" \( -type f -o -type l \) -printf '%P\n' | sort > "$tmp/actual-install-files"
sort "$ROOT/packaging/arborcore-library-files.list" > "$tmp/expected-install-files"
if ! diff -u "$tmp/expected-install-files" "$tmp/actual-install-files"; then
  echo "FAIL: staged installation differs from canonical package manifest." >&2
  exit 1
fi

static_sha="$(sha256sum "$libdir/libarborcore.a" | awk '{print $1}')"
shared_sha="$(sha256sum "$libdir/libarborcore.so.1.0.0" | awk '{print $1}')"
[[ "$static_sha" == "$STATIC_LIBRARY_SHA256" ]] || { echo "FAIL: staged static-library hash" >&2; exit 1; }
[[ "$shared_sha" == "$SHARED_LIBRARY_SHA256" ]] || { echo "FAIL: staged shared-library hash" >&2; exit 1; }

readelf -dW "$libdir/libarborcore.so.1.0.0" \
  | grep -Fq 'Library soname: [libarborcore.so.1]' || {
    echo "FAIL: staged shared library SONAME" >&2; exit 1;
  }

# Prove a consumer can link/run against the installed static archive.
ld -o "$tmp/static-consumer" "$BUILD/abi_consumer_test.o" "$libdir/libarborcore.a"
"$tmp/static-consumer" || { echo "FAIL: staged static consumer" >&2; exit 1; }

# Prove a consumer can link/run through the installed shared symlink chain.
interp="$(readelf -lW /bin/sh | sed -n 's/.*Requesting program interpreter: \([^]]*\)\].*/\1/p' | head -n1)"
[[ -n "$interp" ]] || { echo "FAIL: cannot resolve system dynamic interpreter" >&2; exit 1; }
ld -o "$tmp/shared-consumer" "$BUILD/abi_consumer_test.o" \
  -L"$libdir" -larborcore -dynamic-linker "$interp"
LD_LIBRARY_PATH="$libdir" "$tmp/shared-consumer" || {
  echo "FAIL: staged shared consumer" >&2; exit 1;
}

# Uninstall must remove only Arborcore-owned files and leave no staged files or
# symlinks behind in this isolated DESTDIR.
make -C "$ROOT" \
  DESTDIR="$stage" \
  PREFIX="$prefix" \
  LIBDIR="$prefix/lib" \
  DATADIR="$prefix/share" \
  uninstall-libraries >/dev/null

leftovers="$(find "$stage" \( -type f -o -type l \) -print 2>/dev/null || true)"
if [[ -n "$leftovers" ]]; then
  echo "FAIL: staged uninstall left Arborcore files:" >&2
  printf '%s\n' "$leftovers" >&2
  exit 1
fi

echo "install_static_sha256=$static_sha"
echo "install_shared_sha256=$shared_sha"
echo "PASS: staged install, static/shared consumption, and uninstall"
