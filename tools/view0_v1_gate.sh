#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### VIEW0 V1 SCOPE / BASELINE / CONTRACT'
bash tools/view0_v1_scope_verify.sh
bash tools/view0_v1_baseline_verify.sh
bash tools/view0_v1_contract_verify.sh

echo '### VIEW0 M1 COMPLETE REGRESSION UNDER V1 TOOLING EXTENSION'
bash tools/view0_m1_gate.sh
echo 'PASS: source-review-closed M1 production semantics preserved under V1'

echo '### VIEW0 V1 GENERATOR / CONFORMANCE ORACLE'
bash tools/view0_v1_native_verify.sh
bash tools/view0_v1_conformance_verify.sh

echo '### VIEW0 V1 FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]]
echo 'VIEW0_V1_GATE=PASS'
echo 'VIEW0_V1_STAGED_CHANGES=NO'
echo 'VIEW0_V1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1_REMOTE_WRITE_PERFORMED=NO'
