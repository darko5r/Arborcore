#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
F1_R11="${VIEW0_G03_F1_R11_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-M0B-F1-R11-g03-r6-matrix-support-correction-freeze-candidate-fbab0823185f71b714fb8c989be1fb1748b1f3626accad44c2947cfffdf4b559.tar.gz}"
fail(){ echo "FAIL: $*" >&2; exit 1; }; need_eq(){ [[ "$1" == "$2" ]] || fail "$3: got=$1 expected=$2"; }
contract=view/arborcore-view-core-1.contract
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R6A|0.1-VIEW0-V1N1-G03-R7A) ;;
  *) fail "current contract is not an accepted R6A extension: $current_version" ;;
esac
for item in \
 'VIEW0_V1N1_G03_R6A_CONTRACT_REVISION=0.1-VIEW0-V1N1-G03-R6A' \
 'VIEW0_V1N1_G03_R6A_PREDECESSOR_CANDIDATE_PATH_COUNT=183' \
 'VIEW0_V1N1_G03_R6A_PREDECESSOR_CANDIDATE_MANIFEST_SHA256=6ad4599e7e535e0c6398efc12c7b1f1dda3dd51faba05cafa1e387f4311741c4' \
 'VIEW0_V1N1_G03_R6A_F1_R11_BUNDLE_SHA256=fbab0823185f71b714fb8c989be1fb1748b1f3626accad44c2947cfffdf4b559' \
 'VIEW0_V1N1_G03_R6A_F1_R11_MATRIX_SHA256=49aecd66cadfa9107e8b14647cf7a54cdac3bb2aab588659768a0a221f99dc70' \
 'VIEW0_V1N1_G03_R6A_RULE_ID=0x0000000030030006' \
 'VIEW0_V1N1_G03_R6A_RULE_SYMBOL=ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT' \
 'VIEW0_V1N1_G03_R6A_DIAGNOSTIC_ANCHOR=OWNER_DIAGNOSTICS_ONLY_NO_DEDICATED_R6_ANCHOR' \
 'VIEW0_V1N1_G03_R6A_OWNERSHIP_LEDGER=7_OF_7' \
 'VIEW0_V1N1_G03_R6A_FIXTURE_PLAN=6_OF_6' \
 'VIEW0_V1N1_G03_R6A_FORBIDDEN_VALID_SCALAR_COUNT=127' \
 'VIEW0_V1N1_G03_R6A_P0_CASE_COVERAGE=508_OF_508' \
 'VIEW0_V1N1_G03_R6A_RUNTIME_RETENTION_LEDGER_SHA256=01d26c8d35b591fcd3b301536de638d4d0ba160d59041196d8929ba23f18a05b' \
 'VIEW0_V1N1_G03_R6A_RETAINED_NATIVE_RULE_OBJECT_CLEAN_RESET=8_OF_8' \
 'VIEW0_V1N1_G03_R6A_SCOPE_IDENTITY_COLLATION=LC_ALL_C' \
 'VIEW0_V1N1_G03_R6A_NEW_SEMANTIC_EVALUATOR=NO' \
 'VIEW0_V1N1_G03_R6A_NEW_DIAGNOSTIC=NO' \
 'VIEW0_V1N1_G03_R6A_RUNTIME_SEMANTIC_SOURCE_CHANGE=NO' \
 'VIEW0_V1N1_G03_R6A_DEDICATED_RESULT_FLAG=NONE' \
 'VIEW0_V1N1_G03_R6A_SECOND_UTF8_DECODER=NO' \
 'VIEW0_V1N1_G03_R6A_SECOND_HTML_PARSER=NO' \
 'VIEW0_V1N1_G03_R6A_C0_SR1_RETROFIT_REQUIRED=NO' \
 'VIEW0_V1N1_G03_R6A_RETAINED_LOWER_CURRENT_STATE_ZERO_BOUNDARY=R7_ONLY' \
 'VIEW0_V1N1_G03_R6A_RETAINED_LOWER_STALE_R6_R7_ZERO_MARKERS=ZERO' \
 'VIEW0_V1N1_G03_R6A_G03_R7_RULE_IDS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_G03_R6A_IMPLEMENTATION_COMPLETE=YES_RETAINED_OWNER_INTEGRATION' \
 'VIEW0_V1N1_G03_R6A_G03_GROUP_FREEZE=NO' \
 'VIEW0_V1N1_G03_R6A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do grep -Fxq "$item" "$contract" || fail "missing R6A contract marker: $item"; done
[[ -f "$F1_R11" ]] || fail "missing F1-R11: $F1_R11"
need_eq "$(sha256sum "$F1_R11"|awk '{print $1}')" 'fbab0823185f71b714fb8c989be1fb1748b1f3626accad44c2947cfffdf4b559' 'F1-R11 archive'
tmp=$(mktemp -d /tmp/arborcore-r6a-contract.XXXXXX); trap 'rm -rf "$tmp"' EXIT
tar -xzf "$F1_R11" -C "$tmp"
f1="$tmp/Arborcore-VIEW0-V1N1-M0B-F1-R11-g03-r6-matrix-support-correction-freeze-candidate"
(cd "$f1" && sha256sum -c f1-r11-manifest.sha256 >/dev/null)
for spec in \
 'f1-final-rule-spec-test-matrix.tsv|49aecd66cadfa9107e8b14647cf7a54cdac3bb2aab588659768a0a221f99dc70' \
 'f1-r11-r6-diagnostic-ownership.tsv|8c157fcfeca9b588c8c7be0caa154d6aa643b6627522cfa15b0d580ac5392e2c' \
 'f1-r11-r6-fixture-plan.tsv|1d8d6b646bf356831624f92a7438814e4e0e842f5e304346f5d2e292a42b9d24' \
 'f1-r11-r6-support-plan.tsv|ac962f61a74749dc7ca6af1b9a656a62837bb64f689190f0b73fd2366c74a92c' \
 'f1-r11-r6-provenance.tsv|1693c6758254cde3fae6586ce56d8aedd91b3e9d5ded76ffbdfae51291521788'; do IFS='|' read -r n h <<<"$spec"; need_eq "$(sha256sum "$f1/$n"|awk '{print $1}')" "$h" "$n"; done
cmp -s tests/data/view0_v1n1_g03_r6a_diagnostic_ownership.tsv "$f1/f1-r11-r6-diagnostic-ownership.tsv" || fail 'installed R6 ownership ledger drift'
cmp -s tests/data/view0_v1n1_g03_r6a_fixture_plan.tsv "$f1/f1-r11-r6-fixture-plan.tsv" || fail 'installed R6 fixture plan drift'
cmp -s tests/data/view0_v1n1_g03_r6a_support_plan.tsv "$f1/f1-r11-r6-support-plan.tsv" || fail 'installed R6 support plan drift'
cmp -s tests/data/view0_v1n1_g03_r6a_provenance.tsv "$f1/f1-r11-r6-provenance.tsv" || fail 'installed R6 provenance drift'
need_eq "$(sha256sum tests/data/view0_v1n1_g03_r6a_runtime_retention.tsv|awk '{print $1}')" '01d26c8d35b591fcd3b301536de638d4d0ba160d59041196d8929ba23f18a05b' 'runtime retention ledger'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]]; then
    tail -n +2 tests/data/view0_v1n1_g03_r6a_runtime_retention.tsv | while IFS=$'\t' read -r p h; do need_eq "$(sha256sum "$p"|awk '{print $1}')" "$h" "runtime retained byte identity $p"; done
fi
grep -Fxq 'export LC_ALL=C' tools/view0_v1n1_g03_r6a_scope_verify.sh || fail 'R6A scope verifier does not self-pin LC_ALL=C'
gate=tools/view0_v1n1_g03_r6a_gate.sh
for obj in native.o lexbor_adapter.o g03_c0_provenance.o g03_r1a.o g03_r2a.o g03_r3a.o g03_r4a.o g03_r5a.o; do
    grep -Fq "build/view0-v1/native/$obj" "$gate" || fail "R6A clean reset missing retained native rule object: $obj"
done
grep -Fxq "echo 'VIEW0_V1N1_G03_R6A_RETAINED_NATIVE_RULE_OBJECT_CLEAN_RESET=8_OF_8'" "$gate" || fail 'R6A gate clean-reset evidence marker missing'
[[ ! -e tools/c/view0_conformance/g03_r6a.c && ! -e tools/c/view0_conformance/g03_r6a.h ]] || fail 'dedicated R6 evaluator exists'
! grep -ERq 'ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT|0x0000000030030006|G03_R6' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R6 runtime semantic/flag code introduced'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
retained_stage=R6A
[[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]] && retained_stage=R7A
for rule in r1a r2a r3a r4a r5a; do
    out=$(bash "tools/view0_v1n1_g03_${rule}_contract_verify.sh")
    retained_marker="VIEW0_V1N1_G03_${rule^^}_RETAINED_UNDER_G03_${retained_stage}=PASS"
    grep -Fxq "$retained_marker" <<<"$out" || fail "$rule did not publish ${retained_stage} retention"
    stale_zero_marker="VIEW0_V1N1_G03_${rule^^}_G03_R6_R7_RULE_IDS_IMPLEMENTED=ZERO"
    ! grep -Fxq "$stale_zero_marker" <<<"$out" || fail "$rule republished stale R6+R7 zero marker"
    if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]]; then
        current_zero_marker="VIEW0_V1N1_G03_${rule^^}_G03_R7_RULE_IDS_IMPLEMENTED=ZERO"
        grep -Fxq "$current_zero_marker" <<<"$out" || fail "$rule did not publish R7-only current-state boundary under R6A"
    else
        ! grep -Fq 'G03_R7_RULE_IDS_IMPLEMENTED=ZERO' <<<"$out" || fail "$rule republished stale R7-zero marker under R7A"
    fi
done
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]]; then
    grep -Fq '#define ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY UINT64_C(0x0000000030030007)' tools/include/arborcore/view0_conformance/native.h || fail 'R7A extension rule ID missing under retained R6A'
    grep -Fq '#include "g03_r7a.h"' tools/c/view0_conformance/native.c || fail 'R7A extension integration missing under retained R6A'
fi
echo 'VIEW0_V1N1_G03_R6A_RETAINED_LOWER_CURRENT_STATE_EVIDENCE=PASS'
[[ "$current_version" == '0.1-VIEW0-V1N1-G03-R7A' ]] && echo 'VIEW0_V1N1_G03_R6A_RETAINED_UNDER_G03_R7A=PASS'
echo 'VIEW0_V1N1_G03_R6A_F1_R11_STRUCTURED_BOUNDARY=PASS'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R6A' ]]; then
    echo 'VIEW0_V1N1_G03_R6A_RUNTIME_SEMANTIC_BYTES_RETAINED=17_OF_17'
else
    echo 'VIEW0_V1N1_G03_R6A_RUNTIME_SEMANTIC_OWNERSHIP_RETAINED_UNDER_G03_R7A=PASS'
fi
echo 'VIEW0_V1N1_G03_R6A_RETAINED_R1_R5_CONTRACT_VERIFIERS=PASS'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'PASS: G03 R6A contract binds F1-R11 and retained owner/runtime boundaries without new semantic code'
