#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

predecessor_head='f72d01785bd1b147f940e17e5c3871f00265c574'
predecessor_tree='0b5b631b3915239c205b773f623e2c6660ecf35d'
freeze_subject='Freeze VIEW0 D1 manuals and runnable examples gate'

[[ "$(git branch --show-current)" == 'main' ]] || { echo 'FAIL: D1 requires branch main' >&2; exit 1; }
git diff --cached --quiet || { echo 'FAIL: D1 gate requires an unstaged/clean index' >&2; exit 1; }

expected_paths=$(cat <<'PATHS'
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
expected_paths=$(printf '%s\n' "$expected_paths" | LC_ALL=C sort)

head=$(git rev-parse HEAD)
tree=$(git rev-parse 'HEAD^{tree}')

if [[ "$head" == "$predecessor_head" ]]; then
    [[ "$tree" == "$predecessor_tree" ]] || { echo 'FAIL: D1 predecessor tree mismatch' >&2; exit 1; }
    actual_status=$(git status --porcelain=v1 --untracked-files=all | LC_ALL=C sort)
    expected_status=$(cat <<'STATUS'
 M Makefile
 M docs/VIEW_CORE_VIEW0.md
 M view/arborcore-view-core-1.contract
?? examples/view0/README.md
?? examples/view0/nasm_view.asm
?? examples/view0/page.html
?? examples/view0/render.c
?? tools/view0_d1_contract_verify.sh
?? tools/view0_d1_doc_consistency_verify.sh
?? tools/view0_d1_examples_verify.sh
?? tools/view0_d1_gate.sh
?? tools/view0_d1_scope_verify.sh
STATUS
)
    expected_status=$(printf '%s\n' "$expected_status" | LC_ALL=C sort)
    [[ "$actual_status" == "$expected_status" ]] || {
        echo 'FAIL: D1 candidate path set differs' >&2
        printf '%s\n' '--- expected ---' "$expected_status" '--- actual ---' "$actual_status" >&2
        exit 1
    }
    [[ -z "$(git diff --name-only -- include src abi application mvc http browser renderer geometry packaging)" ]] || {
        echo 'FAIL: D1 must not modify production/API/ABI paths' >&2; exit 1;
    }
    state='CANDIDATE_UNSTAGED'
else
    [[ "$(git rev-parse HEAD^)" == "$predecessor_head" ]] || { echo 'FAIL: frozen D1 parent mismatch' >&2; exit 1; }
    [[ "$(git show -s --format='%s' HEAD)" == "$freeze_subject" ]] || { echo 'FAIL: frozen D1 commit subject mismatch' >&2; exit 1; }
    [[ -z "$(git status --porcelain=v1 --untracked-files=all)" ]] || { echo 'FAIL: frozen D1 worktree must be clean' >&2; exit 1; }
    actual_paths=$(git diff-tree --no-commit-id --name-only -r HEAD | LC_ALL=C sort)
    [[ "$actual_paths" == "$expected_paths" ]] || {
        echo 'FAIL: frozen D1 commit path set differs' >&2
        printf '%s\n' '--- expected ---' "$expected_paths" '--- actual ---' "$actual_paths" >&2
        exit 1
    }
    state='FROZEN_CLEAN'
fi

echo 'VIEW0_D1_PREDECESSOR_HEAD=f72d01785bd1b147f940e17e5c3871f00265c574'
echo 'VIEW0_D1_PREDECESSOR_TREE=0b5b631b3915239c205b773f623e2c6660ecf35d'
echo 'VIEW0_D1_CANDIDATE_PATHS=12_OF_12'
echo "VIEW0_D1_SCOPE_STATE=$state"
echo 'VIEW0_D1_STAGED_CHANGES=NO'
echo 'PASS: VIEW0 D1 exact post-V1N4 construction/frozen scope'
