#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
PROFILE="$(arborcore_perf_profile)"
REFERENCE="${ARBORCORE_QUALIFICATION_REFERENCE_COMMIT:?reference commit required}"
LOG="$ROOT/build/core-retrofit-e-full-check.log"
BASE_TEXT_BYTES=10756
cd "$ROOT"

echo "### Core Retrofit E10: warning-clean reconstruction"
make clean >/dev/null
mkdir -p build
set +e
{ make; make check; } >"$LOG" 2>&1
rc=$?
set -e
cat "$LOG"
[[ "$rc" -eq 0 ]] || { echo "FAIL: full reconstruction/check exit=$rc" >&2; exit "$rc"; }
if grep -nEi 'warning:|clock skew' "$LOG"; then echo "FAIL: warning-clean gate failed" >&2; exit 1; fi
echo "PASS: warning-clean full reconstruction"

echo
echo "### Core Retrofit E10: runtime-specific qualification"
./build/core-http-framing-property-test
./build/core-accept-transaction-test
./build/core-pipeline-budget-test
./build/core-event-batch-test
echo "PASS: E1/E3/E4/E5/E7/E9 runtime contracts"

echo
echo "### Core Retrofit E10: ABI/dependency boundaries"
for spec in \
  'build/net.o:net_socket_tcp4 net_socket_tcp4_flags net_bind net_listen net_accept4 net_shutdown net_close' \
  'build/event.o:event_epoll_create event_epoll_add event_epoll_modify event_epoll_remove event_epoll_wait event_monotonic_ms event_deadline_remaining_ms' \
  'build/http_parser.o:http_parse_request http_frame_scan' \
  'build/server.o:server_open_listener server_create_epoll server_accept_connection server_handle_http_once server_close_connection'
do
  obj="${spec%%:*}"; syms="${spec#*:}"
  for sym in $syms; do
    readelf -sW "$obj" | awk -v s="$sym" '$4=="FUNC"&&$5=="GLOBAL"&&$8==s{ok=1}END{exit !ok}' \
      || { echo "FAIL: $obj missing GLOBAL FUNC $sym" >&2; exit 1; }
  done
done
if nm -u build/server.o | grep -q ' U io_set_nonblocking$'; then
  echo "FAIL: server.o still depends on post-socket fcntl nonblocking setup" >&2; exit 1
fi
for sym in net_socket_tcp4_flags net_bind net_listen net_accept4 event_epoll_create event_epoll_add event_epoll_modify connection_init http_frame_scan http_parse_request route_pattern_dispatch http_response_serialize buffer_reset buffer_consume arena_reset; do
  nm -u build/server.o | grep -q " U $sym$" || { echo "FAIL: server.o missing expected dependency $sym" >&2; exit 1; }
done
echo "PASS: E runtime ABI/dependency boundaries"

echo
echo "### Core Retrofit E10: size evidence"
current_text="$(arborcore_production_text_bytes "$ROOT")"
delta=$((current_text-BASE_TEXT_BYTES))
printf 'retrofit_e_reference_text_bytes=%s\n' "$BASE_TEXT_BYTES"
printf 'current_production_text_bytes=%s\n' "$current_text"
printf 'delta_bytes=%+d\n' "$delta"
size build/net.o build/event.o build/connection.o build/http_parser.o build/server.o
limit=$(( BASE_TEXT_BYTES + (BASE_TEXT_BYTES * 8 + 99) / 100 ))
if (( current_text > limit )); then
  echo "REVIEW REQUIRED: Retrofit E production .text grew by more than 8%." >&2
  exit 1
fi

echo
echo "### Core Retrofit E6/E7: syscall qualification"
make runtime-syscall-experiment

echo
echo "### Core Retrofit E8: scatter/gather experiment"
make writev-experiment
for obj in build/response_iovec_candidate.o build/iovec_write_candidate.o build/response_iovec_bench.o build/core_writev_experiment_test.o; do
  readelf -SW "$obj" | grep -q '\.note.GNU-stack' || { echo "FAIL: $obj GNU-stack" >&2; exit 1; }
done

echo
echo "### Core Retrofit E0/E10: environment-qualified performance"
ARBORCORE_QUALIFICATION_REFERENCE_COMMIT="$REFERENCE" ARBORCORE_PERF_PROFILE="$PROFILE" \
  bash "$ROOT/tools/server_performance_qualified.sh" candidate

echo
echo "### CORE RETROFIT E GATE PASSED"
echo "reference_commit=$REFERENCE"
echo "production_source_sha256=$(arborcore_production_source_sha256 "$ROOT")"
echo "production_text_bytes=$(arborcore_production_text_bytes "$ROOT")"
echo "memory_policy_sha256=$(arborcore_memory_policy_sha256 "$ROOT")"
