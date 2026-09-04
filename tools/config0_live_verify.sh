#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

root=${ARBORCORE_ROOT:-$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)}
hello=$root/build/hello0/hello0
echo_server=$root/build/echo0/echo0
hello_template=$root/examples/hello0/page.html
echo_template=$root/examples/echo0/page.html

for executable in "$hello" "$echo_server"; do
    [[ -x "$executable" ]] || {
        printf 'FAIL: CONFIG0 live executable is missing\n' >&2
        exit 1
    }
done

work=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-config0-live.XXXXXXXX")
server_pid=''
cleanup()
{
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
        kill -TERM "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
    case "$work" in
        "${TMPDIR:-/tmp}"/arborcore-config0-live.*)
            rm -R -- "$work"
            ;;
    esac
}
trap cleanup EXIT

ARBORCORE_ROOT=$root bash "$root/tools/hello0_live_verify.sh"
ARBORCORE_ROOT=$root bash "$root/tools/echo0_live_verify.sh"
printf 'CONFIG0_LEGACY_POSITIONAL=PASS_HELLO0_AND_ECHO0\n'

run_named()
{
    local label=$1
    local prefix=$2
    local signal_name=$3
    shift 3
    local log=$work/$label.log
    "$@" >"$log" 2>&1 &
    server_pid=$!
    local ready=''
    for _ in {1..100}; do
        ready=$(sed -n "s/^${prefix}_READY=//p" "$log" | tail -n 1)
        [[ -n "$ready" ]] && break
        if ! kill -0 "$server_pid" 2>/dev/null; then
            printf 'FAIL: %s exited before readiness\n' "$label" >&2
            sed -n '1,80p' "$log" >&2
            exit 1
        fi
        sleep 0.05
    done
    [[ "$ready" =~ ^http://127\.0\.0\.1:[0-9]+/ ]] || {
        printf 'FAIL: %s did not publish readiness\n' "$label" >&2
        exit 1
    }
    curl --http1.1 --silent --show-error --max-time 5 \
        --header 'Connection: close' --output /dev/null "$ready"
    kill "-$signal_name" "$server_pid"
    wait "$server_pid"
    server_pid=''
    rg -q "^${prefix}_STOPPED " "$log"
    rg -q "^${prefix}_LIFE0 phase=CLOSED " "$log"
}

printf 'template=%s\nport=0\nbacklog=16\nevent_wait_ms=250\ndrain_timeout_ms=2000\n' \
    "$hello_template" >"$work/hello.conf"
printf 'template=%s\nport=0\nbacklog=16\nevent_wait_ms=250\ndrain_timeout_ms=2000\n' \
    "$echo_template" >"$work/echo.conf"

run_named hello-file HELLO0 TERM \
    "$hello" "--config=$work/hello.conf"
run_named echo-file ECHO0 INT \
    "$echo_server" "--config=$work/echo.conf"
printf 'CONFIG0_FILE_MODE=PASS_HELLO0_TERM_ECHO0_INT\n'

run_named hello-environment HELLO0 INT \
    env ARBORCORE_TEMPLATE="$hello_template" ARBORCORE_PORT=0 "$hello"
run_named echo-environment ECHO0 TERM \
    env ARBORCORE_TEMPLATE="$echo_template" ARBORCORE_PORT=0 "$echo_server"
printf 'CONFIG0_ENVIRONMENT_MODE=PASS_HELLO0_INT_ECHO0_TERM\n'

run_named hello-command HELLO0 TERM \
    "$hello" "--template=$hello_template" --port=0 --backlog=16
run_named echo-command ECHO0 INT \
    "$echo_server" "--template=$echo_template" --port=0 --event-wait-ms=1
printf 'CONFIG0_COMMAND_LINE_MODE=PASS_HELLO0_AND_ECHO0\n'

run_named hello-precedence HELLO0 TERM \
    env ARBORCORE_PORT=1 \
    "$hello" "--config=$work/hello.conf" --port=0
run_named echo-precedence ECHO0 TERM \
    env ARBORCORE_BACKLOG=8 \
    "$echo_server" "--config=$work/echo.conf" --port=0 --backlog=4
printf 'CONFIG0_LIVE_PRECEDENCE=PASS_FILE_ENVIRONMENT_COMMAND_LINE\n'

set +e
"$hello" "--config=$work/absent.conf" >"$work/invalid.out" 2>&1
invalid_status=$?
set -e
[[ $invalid_status -eq 2 ]]
rg -q '^HELLO0_CONFIG_FILE_ERROR$' "$work/invalid.out"
! rg -q "$work/absent.conf" "$work/invalid.out"
printf 'CONFIG0_INVALID_ACQUISITION=PASS_BEFORE_RESOURCE_PUBLICATION\n'

printf 'CONFIG0_LIVE_SIGNALS=PASS_HELLO0_INT_TERM_ECHO0_INT_TERM\n'
printf 'PASS: CONFIG0 legacy, file, environment, command-line, precedence and lifecycle evidence\n'
