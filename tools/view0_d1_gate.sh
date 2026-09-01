#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

echo '### VIEW0 D1 EXACT SCOPE'
bash tools/view0_d1_scope_verify.sh

echo '### VIEW0 D1 CONTRACT / FROZEN BOUNDARIES'
bash tools/view0_d1_contract_verify.sh

echo '### VIEW0 D1 DOCUMENTATION CONSISTENCY'
bash tools/view0_d1_doc_consistency_verify.sh

echo '### VIEW0 D1 RUNNABLE FILE-BASED EXAMPLES / NATIVE V1 / M1 / C4'
bash tools/view0_d1_examples_verify.sh

echo '### VIEW0 D1 FINAL DIFF / POLICY'
git diff --check
git diff --cached --quiet || { echo 'FAIL: D1 gate may not stage changes' >&2; exit 1; }
bash tools/view0_d1_scope_verify.sh >/dev/null
bash tools/view0_d1_contract_verify.sh >/dev/null
bash tools/view0_d1_doc_consistency_verify.sh >/dev/null

echo 'VIEW0_D1_GATE=PASS'
echo 'VIEW0_D1_ADMISSION=YES_POST_INDEPENDENT_REVIEW_AND_FREEZE'
echo 'VIEW0_D1_STAGED_CHANGES=NO'
echo 'VIEW0_D1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_D1_REMOTE_WRITE_PERFORMED=NO'
echo 'PASS: post-V1N4 VIEW0 D1 manuals, runnable examples, and documentation-consistency gate'
