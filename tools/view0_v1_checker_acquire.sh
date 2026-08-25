#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
VNU_SHA256='c70279e260e5f4f9e95b3890ef6c9548f90ebdaec2a75219ec41694ab6241e34'
VNU_RELEASE_COMMIT='01d1e57683dd6e995c95d60173a06e58c6cb5699'
VNU_RELEASE_ASSET_TIMESTAMP='2026-08-15T02:23:54Z'
VNU_VERSION='26.8.15'
VNU_NPM_URL='https://registry.npmjs.org/vnu-jar/-/vnu-jar-26.8.15.tgz'
VNU_GITHUB_URL='https://github.com/validator/validator/releases/download/latest/vnu.jar'
ORACLE_DIR='build/view0-v1/oracle'
JAR="$ORACLE_DIR/vnu.jar"
EXPLICIT_ORACLE="${ARBORCORE_VIEW0_V1_ORACLE_FILE:-}"

fail() { printf 'FAIL: %s\n' "$*" >&2; exit 1; }

for tool in java curl tar sha256sum; do
    command -v "$tool" >/dev/null 2>&1 || fail "required V1 oracle tool missing: $tool"
done
java_spec=$(java -XshowSettings:properties -version 2>&1 | awk -F'= ' '/java.specification.version/{print $2; exit}')
[[ "$java_spec" =~ ^[0-9]+$ ]] || fail "unable to determine Java specification version: $java_spec"
(( java_spec >= 17 )) || fail "Nu HTML Checker requires Java 17+; found Java $java_spec"

mkdir -p "$ORACLE_DIR"

verify_oracle() {
    local path=$1
    [[ -f "$path" ]] || return 1
    [[ "$(sha256sum "$path" | awk '{print $1}')" == "$VNU_SHA256" ]] || return 1
    local version
    version=$(LC_ALL=C java -jar "$path" --version 2>&1 | head -n1)
    [[ "$version" == "$VNU_VERSION (01d1e57)" ]] || return 1
    return 0
}

acquisition=''

if [[ -f "$JAR" ]]; then
    if ! verify_oracle "$JAR"; then
        fail 'cached V1 oracle differs from the pinned digest/version; preserve it for diagnosis and restore deliberately'
    fi
    acquisition='CACHED_EXACT'
elif [[ -n "$EXPLICIT_ORACLE" ]]; then
    [[ -f "$EXPLICIT_ORACLE" ]] || fail "explicit V1 oracle file does not exist: $EXPLICIT_ORACLE"
    verify_oracle "$EXPLICIT_ORACLE" || fail 'explicit V1 oracle file does not match the pinned digest/version'
    tmp="$ORACLE_DIR/vnu.jar.tmp"
    rm -f "$tmp"
    cp -- "$EXPLICIT_ORACLE" "$tmp"
    [[ "$(sha256sum "$tmp" | awk '{print $1}')" == "$VNU_SHA256" ]] || fail 'explicit-oracle copy changed unexpectedly'
    mv "$tmp" "$JAR"
    acquisition='EXPLICIT_FILE_EXACT'
else
    work=$(mktemp -d "$ORACLE_DIR/acquire.XXXXXX")
    trap 'rm -rf "$work"' EXIT
    npm_tgz="$work/vnu-jar.tgz"
    npm_jar="$work/package/build/dist/vnu.jar"

    if curl --proto '=https' --tlsv1.2 -fL --retry 3 --connect-timeout 20 \
        "$VNU_NPM_URL" -o "$npm_tgz" \
        && tar -xzf "$npm_tgz" -C "$work" package/build/dist/vnu.jar \
        && verify_oracle "$npm_jar"; then
        cp -- "$npm_jar" "$JAR.tmp"
        [[ "$(sha256sum "$JAR.tmp" | awk '{print $1}')" == "$VNU_SHA256" ]] || fail 'versioned npm oracle copy changed unexpectedly'
        mv "$JAR.tmp" "$JAR"
        acquisition='NPM_VERSIONED_EXACT'
    else
        rm -f "$JAR.tmp"
        if ! curl --proto '=https' --tlsv1.2 -fL --retry 3 --connect-timeout 20 \
            "$VNU_GITHUB_URL" -o "$JAR.tmp"; then
            fail 'both versioned npm and GitHub-latest V1 oracle acquisition channels failed'
        fi
        verify_oracle "$JAR.tmp" || {
            rm -f "$JAR.tmp"
            fail 'GitHub latest no longer matches the pinned V1 oracle; supply the exact jar with ARBORCORE_VIEW0_V1_ORACLE_FILE'
        }
        mv "$JAR.tmp" "$JAR"
        acquisition='GITHUB_LATEST_DIGEST_CHECKED_FALLBACK'
    fi

    rm -rf "$work"
    trap - EXIT
fi

verify_oracle "$JAR" || fail 'final cached V1 oracle identity/version mismatch'
actual=$(sha256sum "$JAR" | awk '{print $1}')
version=$(LC_ALL=C java -jar "$JAR" --version 2>&1 | head -n1)
printf 'VIEW0_V1_ORACLE_ACQUISITION=%s\n' "$acquisition"
printf 'VIEW0_V1_ORACLE_SHA256=%s\n' "$actual"
printf 'VIEW0_V1_ORACLE_VERSION=%s\n' "$VNU_VERSION"
printf 'VIEW0_V1_ORACLE_RELEASE_COMMIT=%s\n' "$VNU_RELEASE_COMMIT"
printf 'VIEW0_V1_ORACLE_RELEASE_ASSET_TIMESTAMP=%s\n' "$VNU_RELEASE_ASSET_TIMESTAMP"
printf 'VIEW0_V1_ORACLE_PRIMARY_RECONSTRUCTION=NPM_VERSIONED_TARBALL\n'
printf 'VIEW0_V1_ORACLE_NPM_URL=%s\n' "$VNU_NPM_URL"
printf 'VIEW0_V1_ORACLE_GITHUB_LATEST_FALLBACK=DIGEST_CHECKED_ONLY\n'
printf 'VIEW0_V1_JAVA_SPECIFICATION_VERSION=%s\n' "$java_spec"
printf 'VIEW0_V1_ORACLE_VERSION_LINE=%s\n' "$version"
echo 'PASS: exact pinned Nu HTML Checker oracle is reconstructible outside tracked source'
