#!/usr/bin/env bash
set -euo pipefail

root=${ARBORCORE_ROOT:-$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)}

command -v curl >/dev/null 2>&1 || {
    printf 'FAIL: curl is required for LIFE0 live verification\n' >&2
    exit 1
}
command -v rg >/dev/null 2>&1 || {
    printf 'FAIL: rg is required for LIFE0 live verification\n' >&2
    exit 1
}

work=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-life0-live.XXXXXXXX")
server_pid=''
cleanup() {
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
        kill -TERM "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
    if [[ "$work" == "${TMPDIR:-/tmp}/arborcore-life0-live."* ]]; then
        rm -R -- "$work"
    fi
}
trap cleanup EXIT

run_signal_case() {
    local name=$1
    local signal_name=$2
    local server=$3
    local template=$4
    local ready_prefix=$5
    local path=$6
    local stopped_pattern=$7
    local life_pattern=$8
    local log="$work/${name}-${signal_name}.log"

    [[ -x "$server" ]] || {
        printf 'FAIL: LIFE0 server is not executable: %s\n' "$server" >&2
        return 1
    }
    [[ -f "$template" ]] || {
        printf 'FAIL: LIFE0 template is missing: %s\n' "$template" >&2
        return 1
    }

    "$server" "$template" 0 >"$log" 2>&1 &
    server_pid=$!
    local ready=''
    for _ in {1..120}; do
        ready=$(sed -n "s/^${ready_prefix}=//p" "$log" | tail -n 1)
        if [[ -n "$ready" ]]; then
            break
        fi
        if ! kill -0 "$server_pid" 2>/dev/null; then
            printf 'FAIL: %s exited before LIFE0 readiness\n' "$name" >&2
            sed -n '1,160p' "$log" >&2
            return 1
        fi
        sleep 0.05
    done
    [[ "$ready" =~ ^http://127\.0\.0\.1:[0-9]+/ ]] || {
        printf 'FAIL: %s did not publish a readiness URL\n' "$name" >&2
        sed -n '1,160p' "$log" >&2
        return 1
    }
    local base=${ready%"$path"}
    curl --http1.1 --silent --show-error --fail --max-time 5 \
        --header 'Connection: close' \
        --output "$work/${name}-${signal_name}.body" \
        "$base$path"

    kill -"$signal_name" "$server_pid"
    local stopped=0
    for _ in {1..120}; do
        if ! kill -0 "$server_pid" 2>/dev/null; then
            stopped=1
            break
        fi
        sleep 0.05
    done
    [[ $stopped -eq 1 ]] || {
        printf 'FAIL: %s did not finish its %s drain\n' "$name" "$signal_name" >&2
        sed -n '1,200p' "$log" >&2
        return 1
    }
    wait "$server_pid"
    server_pid=''

    rg -q "$stopped_pattern" "$log"
    rg -q "$life_pattern" "$log"
    printf 'LIFE0_LIVE_CASE=%s_%s_PASS\n' "$name" "$signal_name"
}

hello_server=${HELLO0_SERVER:-$root/build/hello0/hello0}
hello_template=${HELLO0_TEMPLATE:-$root/examples/hello0/page.html}
echo_server=${ECHO0_SERVER:-$root/build/echo0/echo0}
echo_template=${ECHO0_TEMPLATE:-$root/examples/echo0/page.html}

hello_stopped='^HELLO0_STOPPED middleware=1 controller=1 service=1 presenter=1$'
hello_life='^HELLO0_LIFE0 phase=CLOSED active_at_drain_start=0 inactive_before_deadline=0 forced_at_deadline=0 deadline_expired=0 drain_start_ms=[0-9]+ drain_finish_ms=[0-9]+ first_failure=0$'
echo_stopped='^ECHO0_STOPPED middleware=1 controller=1 service=1 presenter=1$'
echo_life='^ECHO0_LIFE0 phase=CLOSED active_at_drain_start=0 inactive_before_deadline=0 forced_at_deadline=0 deadline_expired=0 drain_start_ms=[0-9]+ drain_finish_ms=[0-9]+ first_failure=0$'

run_signal_case \
    HELLO0 INT "$hello_server" "$hello_template" HELLO0_READY /hello \
    "$hello_stopped" "$hello_life"
run_signal_case \
    HELLO0 TERM "$hello_server" "$hello_template" HELLO0_READY /hello \
    "$hello_stopped" "$hello_life"
run_signal_case \
    ECHO0 INT "$echo_server" "$echo_template" ECHO0_READY /echo/Arborcore \
    "$echo_stopped" "$echo_life"
run_signal_case \
    ECHO0 TERM "$echo_server" "$echo_template" ECHO0_READY /echo/Arborcore \
    "$echo_stopped" "$echo_life"

printf 'LIFE0_LIVE_SIGNALS=PASS_4_OF_4\n'
printf 'PASS: LIFE0-R0 live SIGINT and SIGTERM drains for HELLO0 and ECHO0\n'
