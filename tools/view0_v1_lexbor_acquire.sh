#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

TAG='v3.0.0'
COMMIT='2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe'
TREE='70da8da84cabdc4f02d47378602c41090b2b610c'
SOURCE_MANIFEST_SHA256='a38edb39fe84f7fff90ff6206e6114aa3edab3c75ff363abaa11ee200d23e20d'
SOURCE_FILE_COUNT='1055'
GIT_ARCHIVE_SHA256='b738cffc343868268d59109be5a1378dc854bfc06ddd5564954060398d3016e6'
REMOTE='https://github.com/lexbor/lexbor.git'
CACHE='build/view0-v1/native/lexbor-src'
OVERRIDE="${ARBORCORE_VIEW0_V1_LEXBOR_SOURCE:-}"

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

verify_source_worktree_exact()
{
    local source_path="$1"
    local label="$2"

    [[ -d "$source_path/.git" ]] ||
        fail "$label is not a Git worktree"

    local source_status
    if ! source_status="$(
        git -C "$source_path" \
            status --porcelain=v1 --untracked-files=all --ignored=matching
    )"; then
        fail "$label status inspection failed; preserve for diagnosis"
    fi
    [[ -z "$source_status" ]] ||
        fail "$label contains modified, untracked, or ignored source-tree files; preserve for diagnosis"
}

for tool in git sha256sum sort wc awk; do
    command -v "$tool" >/dev/null 2>&1 || fail "required tool missing: $tool"
done

git check-ignore -q "$CACHE" || fail 'Lexbor source cache must remain ignored build evidence'

if [[ ! -e "$CACHE" ]]; then
    mkdir -p "$(dirname "$CACHE")"
    if [[ -n "$OVERRIDE" ]]; then
        verify_source_worktree_exact "$OVERRIDE" 'explicit Lexbor source override'
        [[ "$(git -C "$OVERRIDE" rev-parse HEAD)" == "$COMMIT" ]] ||
            fail 'explicit Lexbor source override commit mismatch'
        [[ "$(git -C "$OVERRIDE" rev-parse 'HEAD^{tree}')" == "$TREE" ]] ||
            fail 'explicit Lexbor source override tree mismatch'
        git clone -q --no-hardlinks --no-checkout "$OVERRIDE" "$CACHE"
        git -C "$CACHE" checkout -q --detach "$COMMIT"
        acquisition='EXPLICIT_LOCAL_SOURCE_EXACT'
    else
        git init -q "$CACHE"
        git -C "$CACHE" remote add origin "$REMOTE"
        git -C "$CACHE" fetch -q --depth=1 origin "refs/tags/$TAG:refs/tags/$TAG"
        git -C "$CACHE" checkout -q --detach "$COMMIT"
        acquisition='GIT_TAG_EXACT'
    fi
else
    [[ -d "$CACHE/.git" ]] || fail 'existing Lexbor cache is not a Git worktree; preserve for diagnosis'
    acquisition='CACHED_EXACT'
fi

[[ "$(git -C "$CACHE" rev-parse HEAD)" == "$COMMIT" ]] ||
    fail 'cached Lexbor commit mismatch; preserve cache for diagnosis'
[[ "$(git -C "$CACHE" rev-parse 'HEAD^{tree}')" == "$TREE" ]] ||
    fail 'cached Lexbor tree mismatch; preserve cache for diagnosis'
verify_source_worktree_exact "$CACHE" 'cached Lexbor source'

manifest_file='build/view0-v1/native/lexbor-source-manifest.sha256'
(
    cd "$CACHE"
    git ls-files -z |
        LC_ALL=C sort -z |
        while IFS= read -r -d '' path; do
            sha256sum "$path"
        done
) > "$manifest_file"

manifest_sha=$(sha256sum "$manifest_file" | awk '{print $1}')
manifest_count=$(wc -l < "$manifest_file")
[[ "$manifest_sha" == "$SOURCE_MANIFEST_SHA256" ]] ||
    fail "canonical Lexbor source manifest mismatch: $manifest_sha"
[[ "$manifest_count" -eq "$SOURCE_FILE_COUNT" ]] ||
    fail "Lexbor source file count mismatch: $manifest_count"

archive_sha=$(git -C "$CACHE" archive --format=tar HEAD | sha256sum | awk '{print $1}')
[[ "$archive_sha" == "$GIT_ARCHIVE_SHA256" ]] ||
    fail "Lexbor git archive mismatch: $archive_sha"

printf 'VIEW0_V1_LEXBOR_ACQUISITION=%s\n' "$acquisition"
echo "VIEW0_V1_LEXBOR_SOURCE_PATH=$CACHE"
echo "VIEW0_V1_LEXBOR_TAG=$TAG"
echo "VIEW0_V1_LEXBOR_COMMIT=$COMMIT"
echo "VIEW0_V1_LEXBOR_TREE=$TREE"
echo "VIEW0_V1_LEXBOR_CANONICAL_SOURCE_MANIFEST_SHA256=$SOURCE_MANIFEST_SHA256"
echo "VIEW0_V1_LEXBOR_CANONICAL_SOURCE_FILE_COUNT=$SOURCE_FILE_COUNT"
echo "VIEW0_V1_LEXBOR_GIT_ARCHIVE_SHA256=$GIT_ARCHIVE_SHA256"
echo 'VIEW0_V1_LEXBOR_SOURCE_TREE_EXTRA_POLICY=REJECT_MODIFIED_UNTRACKED_AND_IGNORED'
echo 'PASS: exact Lexbor v3.0.0 source is available as ignored development evidence'
