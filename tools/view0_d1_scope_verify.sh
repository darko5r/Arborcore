#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

d1_freeze='af924e2b3bd58379d4815896d694251addbe0831'
d1_tree='e3c93b32d846f55d85b938bc20744b199ea3b591'
d1_parent='f72d01785bd1b147f940e17e5c3871f00265c574'
d1_subject='Freeze VIEW0 D1 manuals and runnable examples gate'

[[ "$(git branch --show-current)" == 'main' ]] || { echo 'FAIL: D1 gate requires branch main' >&2; exit 1; }
git diff --cached --quiet || { echo 'FAIL: D1 gate requires an unstaged/clean index' >&2; exit 1; }

git cat-file -e "$d1_freeze^{commit}" || { echo 'FAIL: frozen D1 commit object missing' >&2; exit 1; }
[[ "$(git rev-parse "$d1_freeze^{tree}")" == "$d1_tree" ]] || { echo 'FAIL: frozen D1 tree mismatch' >&2; exit 1; }
[[ "$(git rev-parse "$d1_freeze^")" == "$d1_parent" ]] || { echo 'FAIL: frozen D1 parent mismatch' >&2; exit 1; }
[[ "$(git show -s --format='%s' "$d1_freeze")" == "$d1_subject" ]] || { echo 'FAIL: frozen D1 subject mismatch' >&2; exit 1; }

expected_freeze_paths=$(cat <<'PATHS'
Makefile
docs/VIEW_CORE_VIEW0.md
examples/view0/README.md
examples/view0/nasm_view.asm
examples/view0/page.html
examples/view0/render.c
tools/view0_d1_contract_verify.sh
tools/view0_d1_doc_consistency_verify.sh
tools/view0_d1_examples_verify.sh
tools/view0_d1_gate.sh
tools/view0_d1_scope_verify.sh
view/arborcore-view-core-1.contract
PATHS
)
expected_freeze_paths=$(printf '%s\n' "$expected_freeze_paths" | LC_ALL=C sort)
actual_freeze_paths=$(git diff-tree --no-commit-id --name-only -r "$d1_freeze" | LC_ALL=C sort)
[[ "$actual_freeze_paths" == "$expected_freeze_paths" ]] || {
    echo 'FAIL: frozen D1 commit path set differs' >&2
    printf '%s\n' '--- expected ---' "$expected_freeze_paths" '--- actual ---' "$actual_freeze_paths" >&2
    exit 1
}

head=$(git rev-parse HEAD)
if [[ "$head" == "$d1_freeze" ]]; then
    actual_status=$(git status --porcelain=v1 --untracked-files=all | LC_ALL=C sort)
    expected_status=$(cat <<'STATUS'
 M docs/VIEW_CORE_VIEW0.md
 M tools/view0_d1_doc_consistency_verify.sh
 M tools/view0_d1_scope_verify.sh
STATUS
)
    expected_status=$(printf '%s\n' "$expected_status" | LC_ALL=C sort)
    if [[ "$actual_status" == "$expected_status" ]]; then
        state='POST_FREEZE_STATUS_RECONCILIATION_CANDIDATE'
    elif [[ -z "$actual_status" ]]; then
        state='FROZEN_CLEAN'
    else
        echo 'FAIL: D1 freeze worktree is neither clean nor the exact status-reconciliation candidate' >&2
        printf '%s\n' '--- expected candidate ---' "$expected_status" '--- actual ---' "$actual_status" >&2
        exit 1
    fi
else
    git merge-base --is-ancestor "$d1_freeze" "$head" || {
        echo 'FAIL: current HEAD is not a descendant of the frozen D1 boundary' >&2; exit 1;
    }
    [[ -z "$(git status --porcelain=v1 --untracked-files=all)" ]] || {
        echo 'FAIL: D1 gate on post-freeze descendants requires a clean worktree' >&2; exit 1;
    }
    state='FROZEN_DESCENDANT'
fi

echo 'VIEW0_D1_FREEZE_HEAD=af924e2b3bd58379d4815896d694251addbe0831'
echo 'VIEW0_D1_FREEZE_TREE=e3c93b32d846f55d85b938bc20744b199ea3b591'
echo 'VIEW0_D1_FREEZE_PATHS=12_OF_12'
echo "VIEW0_D1_SCOPE_STATE=$state"
echo 'VIEW0_D1_STAGED_CHANGES=NO'
echo 'PASS: VIEW0 D1 frozen boundary and descendant-safe regression scope'
