#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }

base_commit='7ed6ffd71cda186f5c199593e607cc321a7956c4'
base_tree='0829bb1b49709f2abb2f404e06ac8671c337c154'
expected_pathlist_sha='433d2449cfa12906e37df50e7a4c3e79f7a32e58f147d789d00b49b626d19521'

[[ "$(git rev-parse HEAD)" == "$base_commit" ]] || fail 'base commit drift'
[[ "$(git rev-parse 'HEAD^{tree}')" == "$base_tree" ]] || fail 'base tree drift'
[[ "$(git symbolic-ref --short HEAD)" == main ]] || fail 'C0 must remain on main'
git diff --cached --quiet || fail 'staged change present'

tmp=$(mktemp -d /tmp/arborcore-v1n2-c0-scope.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
{
  git diff --name-only
  git ls-files --others --exclude-standard
} | sort -u > "$tmp/actual-paths.txt"

cat > "$tmp/expected-paths.txt" <<'EOF'
Makefile
docs/VIEW_CORE_VIEW0.md
tests/c/view0_v1n2_c0_foundation_test.c
tests/data/view0_v1n2_c0_external_authority.tsv
tests/data/view0_v1n2_c0_rule_authority.tsv
tools/c/view0_conformance/native.c
tools/c/view0_conformance/v1n2_c0.c
tools/c/view0_conformance/v1n2_c0.h
tools/include/arborcore/view0_conformance/native.h
tools/view0_v1n2_c0_contract_verify.sh
tools/view0_v1n2_c0_gate.sh
tools/view0_v1n2_c0_native_verify.sh
tools/view0_v1n2_c0_scope_verify.sh
view/arborcore-view-core-1.contract
EOF

cmp -s "$tmp/expected-paths.txt" "$tmp/actual-paths.txt" || {
  diff -u "$tmp/expected-paths.txt" "$tmp/actual-paths.txt" >&2 || true
  fail 'V1N2 C0 source scope drift'
}
printf '%s  %s\n' "$expected_pathlist_sha" "$tmp/actual-paths.txt" | sha256sum -c -
[[ "$(wc -l < "$tmp/actual-paths.txt")" == 14 ]] || fail 'delta path count drift'
[[ "$(git diff --name-only | wc -l)" == 5 ]] || fail 'replacement path count drift'
[[ "$(git ls-files --others --exclude-standard | wc -l)" == 9 ]] || fail 'new path count drift'
! grep -Eq '/g(07|08|09|10|11)([_.]|/)' "$tmp/actual-paths.txt" || \
  fail 'later group source entered C0'
[[ "$(grep -Ec '^ARBOR_VIEW_V1_G(07|08|09|10|11)_' tools/include/arborcore/view0_conformance/native.h)" == 0 ]] || \
  fail 'later group public diagnostic identity entered C0'

echo 'VIEW0_V1N2_C0_DELTA_PATH_COUNT=14'
echo 'VIEW0_V1N2_C0_REPLACEMENT_PATH_COUNT=5'
echo 'VIEW0_V1N2_C0_NEW_PATH_COUNT=9'
echo "VIEW0_V1N2_C0_PATHLIST_SHA256=$expected_pathlist_sha"
echo 'VIEW0_V1N2_C0_STAGED_CHANGES=NO'
echo 'VIEW0_V1N2_C0_LATER_GROUP_SOURCE_PATHS=ZERO'
echo 'PASS: exact V1N2 C0 zero-diagnostic foundation scope established'
