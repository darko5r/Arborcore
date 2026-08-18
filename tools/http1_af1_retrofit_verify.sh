#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE='5caeedb4d7c2b92bee829e27dc615efd5a658cac'
# Layout-defining public header must remain byte-exact.
git diff --quiet "$BASE" -- include/arborcore/application.h
# The retrofit is semantic, not structural.
grep -Fq 'return status >= UINT64_C(200) && status <= UINT64_C(599);' src/c/application_foundation.c
grep -Fq 'response_status_legacy_serializer_supported' src/c/application_foundation.c
grep -Fq 'plan->status == UINT64_C(205)' src/c/application_foundation.c
grep -Fq 'plan->status == UINT64_C(304)' src/c/application_foundation.c
grep -Fxq 'AF_RESPONSE_PLAN_LAYOUT_HTTP1_RETROFIT=UNCHANGED_32_BYTES' application/arborcore-application-ddd-mvc-foundation-1.contract
grep -Fxq 'AF_RESPONSE_PLAN_SUPPORTED_STATUSES=FINAL_200_599' application/arborcore-application-ddd-mvc-foundation-1.contract
grep -Fxq 'AF_LEGACY_RESPONSE_PLAN_SERIALIZER_SUPPORTED_STATUSES=200_201_204_400_404_500' application/arborcore-application-ddd-mvc-foundation-1.contract
bash tools/application_foundation_contract_verify.sh
make -s application-foundation-native-test
make -s application-foundation-sanitize
echo 'AF1_RESPONSE_PLAN_LAYOUT_CHANGE=NO'
echo 'AF1_FINAL_STATUS_RANGE=200_599'
echo 'AF1_LEGACY_SERIALIZER_SUBSET_PRESERVED=YES'
echo 'PASS: controlled AF1 semantic retrofit and dependent native baseline'
