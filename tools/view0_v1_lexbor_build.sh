#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
MODE="${1:-release}"

CANONICAL_SRC='build/view0-v1/native/lexbor-src'
COMPAT_SRC='build/view0-v1/native/lexbor-compat-src'
BUILD="build/view0-v1/native/lexbor-build-$MODE"
LIB="$BUILD/liblexbor_static.a"

LEXBOR_COMMIT='2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe'
LEXBOR_TREE='70da8da84cabdc4f02d47378602c41090b2b610c'
CANONICAL_MANIFEST_SHA='a38edb39fe84f7fff90ff6206e6114aa3edab3c75ff363abaa11ee200d23e20d'
SOURCE_FILE_COUNT='1055'
CANONICAL_IN_BODY_SHA='28b8b1d15329f5f387005982a9a2788a16f66696505f740249f548e400be22ef'
COMPAT_IN_BODY_SHA='142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8'
COMPAT_MANIFEST_SHA='e5e126ad79684b69f42a81a356c268d7cce978d0a0b3948214550683007a15e5'

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

need_eq() {
    [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"
}

case "$MODE" in
    release|sanitize) ;;
    *) fail 'usage: view0_v1_lexbor_build.sh [release|sanitize]' ;;
esac

for tool in cmake cc sha256sum git tar find sort wc awk python3; do
    command -v "$tool" >/dev/null 2>&1 || fail "required tool missing: $tool"
done

# The exact upstream v3.0.0 worktree remains the immutable acquisition authority.
# Every build reconstructs an ignored source copy from the pinned Git object and
# applies only the P3-qualified rp/rt Boolean compatibility correction there.
bash tools/view0_v1_lexbor_acquire.sh >/dev/null
need_eq "$(git -C "$CANONICAL_SRC" rev-parse HEAD)" "$LEXBOR_COMMIT" 'canonical Lexbor commit'
need_eq "$(git -C "$CANONICAL_SRC" rev-parse HEAD^{tree})" "$LEXBOR_TREE" 'canonical Lexbor tree'
canonical_manifest='build/view0-v1/native/lexbor-source-manifest.sha256'
need_eq "$(sha256sum "$canonical_manifest" | awk '{print $1}')" "$CANONICAL_MANIFEST_SHA" 'canonical Lexbor source manifest'
need_eq "$(wc -l < "$canonical_manifest" | tr -d ' ')" "$SOURCE_FILE_COUNT" 'canonical Lexbor source file count'
canonical_in_body="$CANONICAL_SRC/source/lexbor/html/tree/insertion_mode/in_body.c"
need_eq "$(sha256sum "$canonical_in_body" | awk '{print $1}')" "$CANONICAL_IN_BODY_SHA" 'canonical Lexbor in_body.c'

rm -rf "$COMPAT_SRC"
mkdir -p "$COMPAT_SRC"
git -C "$CANONICAL_SRC" archive --format=tar "$LEXBOR_COMMIT" | tar -xf - -C "$COMPAT_SRC"

compat_in_body="$COMPAT_SRC/source/lexbor/html/tree/insertion_mode/in_body.c"
python3 - "$compat_in_body" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
s = p.read_text(encoding='utf-8')
old = "    if (lxb_html_tree_node_is(node, LXB_TAG_RTC) == false\n        || lxb_html_tree_node_is(node, LXB_TAG_RUBY) == false)\n"
new = "    if (lxb_html_tree_node_is(node, LXB_TAG_RTC) == false\n        && lxb_html_tree_node_is(node, LXB_TAG_RUBY) == false)\n"
if s.count(old) != 1:
    raise SystemExit(f'FAIL: Lexbor ruby compatibility preimage count={s.count(old)} expected=1')
if new in s:
    raise SystemExit('FAIL: Lexbor compatibility source unexpectedly already contains corrected predicate')
p.write_text(s.replace(old, new, 1), encoding='utf-8')
PY
need_eq "$(sha256sum "$compat_in_body" | awk '{print $1}')" "$COMPAT_IN_BODY_SHA" 'derived compatibility in_body.c'

compat_manifest='build/view0-v1/native/lexbor-compat-source-manifest.sha256'
(
    cd "$COMPAT_SRC"
    find . -type f -printf '%P\n' | LC_ALL=C sort |
        while IFS= read -r path; do
            sha256sum -- "$path"
        done
) > "$compat_manifest"
need_eq "$(wc -l < "$compat_manifest" | tr -d ' ')" "$SOURCE_FILE_COUNT" 'derived compatibility source file count'
need_eq "$(sha256sum "$compat_manifest" | awk '{print $1}')" "$COMPAT_MANIFEST_SHA" 'derived compatibility source manifest'

cmake_args=(
    -S "$COMPAT_SRC"
    -B "$BUILD"
    -DCMAKE_BUILD_TYPE=Release
    -DLEXBOR_BUILD_SHARED=OFF
    -DLEXBOR_BUILD_STATIC=ON
    -DLEXBOR_BUILD_TESTS=OFF
    -DLEXBOR_BUILD_EXAMPLES=OFF
)

if [[ "$MODE" == sanitize ]]; then
    cmake_args+=(
        '-DCMAKE_C_FLAGS=-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'
    )
fi

cmake "${cmake_args[@]}" >/dev/null
cmake --build "$BUILD" --parallel >/dev/null
[[ -f "$LIB" ]] || fail "Lexbor static library missing after $MODE build: $LIB"

printf 'VIEW0_V1_LEXBOR_BUILD_MODE=%s\n' "$MODE"
printf 'VIEW0_V1_LEXBOR_STATIC_LIBRARY=%s\n' "$LIB"
printf 'VIEW0_V1_LEXBOR_STATIC_LIBRARY_SHA256=%s\n' "$(sha256sum "$LIB" | awk '{print $1}')"
printf 'VIEW0_V1_LEXBOR_COMPATIBILITY_SOURCE_PATH=%s\n' "$COMPAT_SRC"
printf 'VIEW0_V1_LEXBOR_COMPATIBILITY_IN_BODY_SHA256=%s\n' "$COMPAT_IN_BODY_SHA"
printf 'VIEW0_V1_LEXBOR_COMPATIBILITY_SOURCE_MANIFEST_SHA256=%s\n' "$COMPAT_MANIFEST_SHA"
echo 'VIEW0_V1_LEXBOR_CANONICAL_SOURCE_MUTATED=NO'
echo 'VIEW0_V1_LEXBOR_RUBY_RPRT_COMPATIBILITY=OR_TO_AND_DERIVED_BUILD_ONLY'
echo 'PASS: exact Lexbor v3.0.0 canonical source + deterministic ruby compatibility build source established'
