#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"

BASE='848f475ed536bb01133daa81b0888a40588fe0ee'
BRANCH='application-ddd-mvc-af4-ddd-support'

expected=(
Makefile
application/arborcore-application-ddd-support-1.contract
docs/APPLICATION_DDD_SUPPORT_AF4.md
include/arborcore/ddd_support.h
src/c/ddd_support.c
tests/asm/ddd_support_abi_test.asm
tests/c/ddd_support_adversarial_test.c
tests/c/ddd_support_test.c
tools/ddd_support_abi_verify.sh
tools/ddd_support_baseline_verify.sh
tools/ddd_support_contract_verify.sh
tools/ddd_support_gate.sh
tools/ddd_support_native_verify.sh
tools/ddd_support_reproducibility_verify.sh
tools/ddd_support_scope_verify.sh
)

fail(){ printf 'FAIL: %s\n' "$*" >&2; exit 1; }
[[ "$(git branch --show-current)" == "$BRANCH" ]] || fail 'wrong AF4 construction branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'AF4 construction HEAD moved from base'
git diff --cached --quiet || fail 'AF4 candidate must remain unstaged during construction qualification'
git diff --check

mapfile -t actual < <(
  git status --porcelain=v1 --untracked-files=all |
    cut -c4- |
    LC_ALL=C sort
)
mapfile -t wanted < <(printf '%s\n' "${expected[@]}" | LC_ALL=C sort)

[[ "${#actual[@]}" == 15 ]] || fail "candidate path count ${#actual[@]} != 15"
for i in "${!wanted[@]}"; do
  [[ "${actual[$i]}" == "${wanted[$i]}" ]] ||
    fail "scope mismatch at $i: ${actual[$i]} != ${wanted[$i]}"
done

printf 'AF4_CANDIDATE_PATH_COUNT=15\n'
printf '%s\n' "${wanted[@]}"
printf 'AF4_LOWER_LAYER_RETROFIT_REQUIRED=NO\n'
printf 'PASS: AF4 construction scope exact; AF0-AF3 untouched\n'
