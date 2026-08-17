#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"
BASE='34214b51efefe49890b3af2b4bbee924daa27d08'
BRANCH='mvc0-core-rich-transport'
expected=(
Makefile
mvc/arborcore-mvc-core-transport-1.contract
docs/MVC_CORE_TRANSPORT_MVC0.md
include/arborcore/application_transport.h
include/arborcore/mvc.h
src/asm/application_transport.asm
src/c/application_transport.c
src/c/mvc.c
tests/asm/mvc0_abi_test.asm
tests/c/mvc_core_test.c
tests/c/mvc_adversarial_test.c
tests/c/mvc_integration_test.c
tests/c/mvc_end_to_end_test.c
tools/mvc0_baseline_verify.sh
tools/mvc0_contract_verify.sh
tools/mvc0_native_verify.sh
tools/mvc0_abi_verify.sh
tools/mvc0_scope_verify.sh
tools/mvc0_reproducibility_verify.sh
tools/mvc0_gate.sh
)
fail(){ printf 'FAIL: %s\n' "$*" >&2; exit 1; }
[[ "$(git branch --show-current)" == "$BRANCH" ]] || fail 'wrong MVC0 construction branch'
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || fail 'MVC0 construction HEAD moved from base'
git diff --cached --quiet || fail 'MVC0 candidate must remain unstaged during qualification'
git diff --check
mapfile -t actual < <(git status --porcelain=v1 --untracked-files=all | cut -c4- | LC_ALL=C sort)
mapfile -t wanted < <(printf '%s\n' "${expected[@]}" | LC_ALL=C sort)
[[ "${#actual[@]}" == 20 ]] || fail "candidate path count ${#actual[@]} != 20"
for i in "${!wanted[@]}"; do [[ "${actual[$i]}" == "${wanted[$i]}" ]] || fail "scope mismatch at $i: ${actual[$i]} != ${wanted[$i]}"; done
printf 'MVC0_CANDIDATE_PATH_COUNT=20\n'
printf '%s\n' "${wanted[@]}"
printf 'MVC0_LOWER_LAYER_RETROFIT_REQUIRED=NO\n'
printf 'PASS: MVC0 construction scope exact; frozen server/ABI/AF0-AF4/renderer/browser untouched\n'
