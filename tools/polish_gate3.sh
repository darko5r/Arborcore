#!/bin/bash
set -euo pipefail

run_test() {
    local label="$1" exe="$2"
    echo "### POLISH GATE #3: $label"
    local status=0
    "$exe" || status=$?
    if [[ "$status" -ne 0 ]]; then
        echo "FAIL: $exe exit=$status"
        exit "$status"
    fi
    echo "PASS: $(basename "$exe")"
    echo
}

run_test "epoll event contracts" ./build/event-test
run_test "connection-state contracts" ./build/connection-test
run_test "bounded HTTP response serialization" ./build/http-response-test
run_test "request-target decomposition" ./build/request-target-test
run_test "parameter route patterns" ./build/route-pattern-test
run_test "server lifecycle over socketpair" ./build/server-test
run_test "real loopback keep-alive HTTP lifecycle" ./build/polish-gate3-test

echo "### POLISH GATE #3: production dependency boundaries"
[[ "$(nm -u build/event.o | wc -l)" -eq 0 ]]
[[ "$(nm -u build/connection.o | wc -l)" -eq 0 ]]
[[ "$(nm -u build/request_target.o | wc -l)" -eq 0 ]]

for sym in buffer_append u64_decimal_length u64_format_decimal; do
    nm -u build/http_response.o | grep -q " U $sym$"
done
for sym in bytes_equal request_target_from_request; do
    nm -u build/route_pattern.o | grep -q " U $sym$"
done
for sym in net_socket_tcp4 net_bind net_listen net_accept4 event_epoll_create event_epoll_add event_epoll_modify connection_init http_parse_request route_pattern_dispatch http_response_serialize arena_reset; do
    nm -u build/server.o | grep -q " U $sym$"
done
echo "PASS: event/runtime layers have the expected dependency graph"
echo

echo "### POLISH GATE #3: link closure"
for exe in build/event-test build/connection-test build/http-response-test build/request-target-test build/route-pattern-test build/server-test build/polish-gate3-test; do
    if nm -u "$exe" | grep -q .; then
        echo "FAIL: unresolved symbol in $exe"
        nm -u "$exe"
        exit 1
    fi
done
echo "PASS: Gate #3 executables have no unresolved symbols"
echo

echo "### POLISH GATE #3: public ABI symbols"
check_symbol() {
    local obj="$1" sym="$2"
    if ! readelf -sW "$obj" | grep -Eq "FUNC[[:space:]]+GLOBAL.*[[:space:]]$sym$"; then
        echo "FAIL: $obj missing GLOBAL FUNC $sym"
        exit 1
    fi
    echo "PASS: $obj -> $sym"
}
check_symbol build/event.o event_epoll_create
check_symbol build/event.o event_epoll_add
check_symbol build/event.o event_epoll_modify
check_symbol build/event.o event_epoll_remove
check_symbol build/event.o event_epoll_wait
check_symbol build/connection.o connection_init
check_symbol build/connection.o connection_transition
check_symbol build/http_response.o http_response_serialize
check_symbol build/request_target.o request_target_split
check_symbol build/request_target.o request_target_from_request
check_symbol build/route_pattern.o route_pattern_match
check_symbol build/route_pattern.o route_pattern_dispatch
check_symbol build/server.o server_open_listener
check_symbol build/server.o server_create_epoll
check_symbol build/server.o server_accept_connection
check_symbol build/server.o server_handle_http_once
check_symbol build/server.o server_close_connection
echo

echo "### POLISH GATE #3: GNU-stack"
for obj in build/event.o build/connection.o build/http_response.o build/request_target.o build/route_pattern.o build/server.o build/polish_gate3_test.o; do
    readelf -SW "$obj" | grep -q '\.note.GNU-stack' || { echo "FAIL: $obj GNU-stack"; exit 1; }
    echo "PASS: $obj"
done
echo

echo "### POLISH GATE #3: code-size baseline"
size build/event.o build/connection.o build/http_response.o build/request_target.o build/route_pattern.o build/server.o
added_text="$(size build/event.o build/connection.o build/http_response.o build/request_target.o build/route_pattern.o build/server.o | awk 'NR > 1 { sum += $1 } END { print sum + 0 }')"
echo "foundation_text_bytes=6088"
echo "gate3_added_text_bytes=$added_text"
echo "candidate_production_text_bytes=$((6088 + added_text))"

echo
echo "POLISH GATE #3 PASSED"
