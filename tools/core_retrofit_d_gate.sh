#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
PROFILE="$(arborcore_perf_profile)"
REFERENCE="${ARBORCORE_QUALIFICATION_REFERENCE_COMMIT:?ARBORCORE_QUALIFICATION_REFERENCE_COMMIT is required}"
LOG="$ROOT/build/core-retrofit-d-full-check.log"
BASE_TEXT_BYTES=10628

cd "$ROOT"

echo "### Core Retrofit D5: warning-clean reconstruction"
make clean >/dev/null
mkdir -p build
set +e
{
    make
    make check
} >"$LOG" 2>&1
rc=$?
set -e
cat "$LOG"
if [[ "$rc" -ne 0 ]]; then
    echo "FAIL: full reconstruction/check exit=$rc" >&2
    exit "$rc"
fi
if grep -nEi 'warning:|clock skew' "$LOG"; then
    echo "FAIL: warning-clean gate failed" >&2
    exit 1
fi
echo "PASS: warning-clean full reconstruction"

echo
echo "### Core Retrofit D5: D-specific link/dependency closure"
if nm -u build/request_target.o | grep -q .; then
    echo "FAIL: request_target.o unexpectedly depends on another object" >&2
    nm -u build/request_target.o >&2
    exit 1
fi

expect_http=(buffer_append buffer_append_prechecked_disjoint memory_move u64_decimal_length u64_format_decimal)
for sym in "${expect_http[@]}"; do
    nm -u build/http_response.o | grep -q " U $sym$" || { echo "FAIL: http_response.o missing dependency $sym" >&2; exit 1; }
done
for sym in bytes_equal request_target_from_request; do
    nm -u build/route_pattern.o | grep -q " U $sym$" || { echo "FAIL: route_pattern.o missing dependency $sym" >&2; exit 1; }
done
for exe in \
    build/core-request-target-property-test \
    build/core-route-contract-test \
    build/core-http-response-contract-test \
    build/core-route-index-experiment-test
 do
    if nm -u "$exe" | grep -q .; then
        echo "FAIL: unresolved symbol in $exe" >&2
        nm -u "$exe" >&2
        exit 1
    fi
 done
echo "PASS: D-specific link/dependency closure"

echo
echo "### Core Retrofit D5: public ABI"
for spec in \
    'build/request_target.o:request_target_split request_target_from_request' \
    'build/route_pattern.o:route_pattern_match route_pattern_dispatch' \
    'build/router.o:router_find_exact router_dispatch' \
    'build/http_response.o:http_response_serialize'
do
    obj="${spec%%:*}"
    syms="${spec#*:}"
    for sym in $syms; do
        readelf -sW "$obj" | awk -v s="$sym" '$4=="FUNC" && $5=="GLOBAL" && $8==s{ok=1} END{exit !ok}' \
            || { echo "FAIL: $obj missing GLOBAL FUNC $sym" >&2; exit 1; }
    done
done
echo "PASS: D production ABI"

echo
echo "### Core Retrofit D5: size evidence"
current_text="$(arborcore_production_text_bytes "$ROOT")"
delta=$(( current_text - BASE_TEXT_BYTES ))
printf 'retrofit_d_reference_text_bytes=%s\n' "$BASE_TEXT_BYTES"
printf 'current_production_text_bytes=%s\n' "$current_text"
printf 'delta_bytes=%+d\n' "$delta"
size build/request_target.o build/route_pattern.o build/router.o build/http_response.o

# More than 5%% production .text growth is a quality-policy REVIEW, not a silent
# pass. Fail the gate so the user explicitly evaluates the trade-off.
limit=$(( BASE_TEXT_BYTES + (BASE_TEXT_BYTES * 5 + 99) / 100 ))
if (( current_text > limit )); then
    echo "REVIEW REQUIRED: Retrofit D production .text grew by more than 5%." >&2
    exit 1
fi

echo
echo "### Core Retrofit D4: prepared/static route-index experiment"
make route-index-experiment
for obj in build/route_index_candidate.o build/route_index_bench.o build/core_route_index_experiment_test.o; do
    readelf -SW "$obj" | grep -q '\.note.GNU-stack' || { echo "FAIL: $obj GNU-stack" >&2; exit 1; }
done
echo "PASS: D4 experiment GNU-stack notes"

echo
echo "### Core Retrofit D0/D5: environment-qualified performance"
ARBORCORE_QUALIFICATION_REFERENCE_COMMIT="$REFERENCE" \
ARBORCORE_PERF_PROFILE="$PROFILE" \
    bash "$ROOT/tools/server_performance_qualified.sh" candidate

echo
echo "### CORE RETROFIT D GATE PASSED"
echo "reference_commit=$REFERENCE"
echo "production_source_sha256=$(arborcore_production_source_sha256 "$ROOT")"
echo "production_text_bytes=$(arborcore_production_text_bytes "$ROOT")"
echo "memory_policy_sha256=$(arborcore_memory_policy_sha256 "$ROOT")"
