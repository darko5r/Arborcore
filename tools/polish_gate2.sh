#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

run_test() {
    local title="$1"
    local exe="$2"
    echo
    echo "### POLISH GATE #2: $title"
    local status=0
    "$exe" || status=$?
    if [[ "$status" -ne 0 ]]; then
        echo "FAIL: $exe exit=$status" >&2
        exit "$status"
    fi
    echo "PASS: $(basename "$exe")"
}

deps_of() {
    nm -u "$1" | awk '{print $2}' | LC_ALL=C sort
}

expect_deps() {
    local object="$1"
    shift
    local actual expected
    actual="$(deps_of "$object")"
    expected="$(printf '%s\n' "$@" | sed '/^$/d' | LC_ALL=C sort)"
    if [[ "$actual" != "$expected" ]]; then
        echo "FAIL: unexpected dependencies for $object" >&2
        echo "Expected:" >&2
        printf '%s\n' "$expected" >&2
        echo "Actual:" >&2
        printf '%s\n' "$actual" >&2
        exit 1
    fi
}

run_test "Linux I/O contracts" ./build/io-test
run_test "loopback TCP contracts" ./build/net-test
run_test "strict HTTP/1.1 grammar and framing" ./build/http-test
run_test "exact router and dispatch contracts" ./build/router-test
run_test "request-lifecycle integration" ./build/polish-gate2-test

echo
echo "### POLISH GATE #2: production dependency boundaries"
expect_deps build/io.o
expect_deps build/net.o io_close
expect_deps build/http_parser.o \
    bytes_equal_ascii_ci \
    bytes_find_crlf \
    bytes_parse_u64_decimal
expect_deps build/router.o bytes_equal
echo "PASS: runtime layers have the expected dependency graph"

echo
echo "### POLISH GATE #2: integration link closure"
if nm -u build/polish-gate2-test | grep -q .; then
    echo "FAIL: polish-gate2-test contains unresolved symbols" >&2
    nm -u build/polish-gate2-test >&2
    exit 1
fi
if nm -u build/http-test | grep -q .; then
    echo "FAIL: http-test contains unresolved symbols" >&2
    nm -u build/http-test >&2
    exit 1
fi
if nm -u build/router-test | grep -q .; then
    echo "FAIL: router-test contains unresolved symbols" >&2
    nm -u build/router-test >&2
    exit 1
fi
echo "PASS: HTTP/router/lifecycle executables have no unresolved symbols"

echo
echo "### POLISH GATE #2: public ABI symbols"
for spec in \
    "build/io.o:io_read_retry io_write_retry io_close io_set_nonblocking" \
    "build/net.o:net_socket_tcp4 net_bind net_listen net_accept4 net_shutdown net_close" \
    "build/http_parser.o:http_parse_request" \
    "build/router.o:router_find_exact router_dispatch"
do
    object="${spec%%:*}"
    symbols="${spec#*:}"
    for symbol in $symbols; do
        if ! readelf -sW "$object" \
            | awk -v s="$symbol" '$4=="FUNC" && $5=="GLOBAL" && $8==s {found=1} END{exit !found}'; then
            echo "FAIL: $object does not export GLOBAL FUNC $symbol" >&2
            exit 1
        fi
        echo "PASS: $object -> $symbol"
    done
done

echo
echo "### POLISH GATE #2: GNU-stack"
for obj in \
    build/io.o \
    build/net.o \
    build/http_parser.o \
    build/router.o \
    build/polish_gate2_test.o
do
    if ! readelf -SW "$obj" | grep -q '\.note.GNU-stack'; then
        echo "FAIL: $obj has no .note.GNU-stack" >&2
        exit 1
    fi
    echo "PASS: $obj"
done

echo
echo "### POLISH GATE #2: code-size baseline"
size \
    build/io.o \
    build/net.o \
    build/http_parser.o \
    build/router.o

echo
echo "POLISH GATE #2 PASSED"
