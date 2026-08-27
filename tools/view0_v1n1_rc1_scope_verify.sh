#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
[[ "$(git rev-parse HEAD)" == 115d5dcee8e755edcfd0f5c447f9cfc9a0e38893 ]] || fail 'base commit drift'
[[ "$(git rev-parse HEAD^{tree})" == 91d04bcaf60b176d8e7ec738a0fcfb58ca1e706f ]] || fail 'base tree drift'
[[ "$(git branch --show-current)" == view0-v1-completion ]] || fail 'branch drift'
[[ -z "$(git diff --name-only --cached)" ]] || fail 'staged changes present'
paths=$(mktemp /tmp/arborcore-v1n1-rc1-paths.XXXXXX)
trap 'find "$paths" -type f -delete' EXIT
{ git diff --name-only; git ls-files --others --exclude-standard; } | sort -u > "$paths"
count=$(wc -l < "$paths")
hash=$(sha256sum "$paths" | awk '{print $1}')
[[ "$count" -eq 151 ]] || fail "candidate path count $count"
[[ "$hash" == 843013d97e8cac899916c665d9387509b0c8b83891f32cd79eaf1be7dd13f638 ]] || fail "candidate path-list $hash"
[[ "$(find tools tests view docs -type f \( -iname '*g07*' -o -iname '*v1n2*' \) | wc -l)" -eq 0 ]] || fail 'later-wave paths present'
[[ "$(sed -n '1p' view/arborcore-view-core-1.contract)" == 'ARBORCORE_VIEW_CORE_VERSION=0.1-VIEW0-V1N1-RC1' ]] || fail 'RC1 version'
echo "VIEW0_V1N1_RC1_CANDIDATE_PATH_COUNT=$count"
echo "VIEW0_V1N1_RC1_CANDIDATE_PATHLIST_SHA256=$hash"
echo 'VIEW0_V1N1_RC1_TRANSACTION_DELTA_PATH_COUNT=23'
echo 'VIEW0_V1N1_RC1_TRANSACTION_REPLACEMENT_PATH_COUNT=17'
echo 'VIEW0_V1N1_RC1_TRANSACTION_NEW_PATH_COUNT=6'
echo 'VIEW0_V1N1_RC1_G07_PATHS=ZERO'
echo 'VIEW0_V1N1_RC1_STAGED_CHANGES=NO'
echo 'PASS: exact V1N1 RC1 reconciliation source scope established over G06-GF1'
