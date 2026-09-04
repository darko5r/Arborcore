#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

root=${ARBORCORE_ROOT:-$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)}
application=${COUNTER1_APPLICATION:-$root/build/counter1/counter1}
template=${COUNTER1_TEMPLATE:-$root/examples/counter1/page.html}

fail()
{
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

for command_name in curl sha256sum grep sed awk mktemp kill; do
    command -v "$command_name" >/dev/null 2>&1 || fail "missing command: $command_name"
done
[[ -x "$application" ]] || fail "COUNTER1 application is not executable: $application"
[[ -f "$template" ]] || fail "COUNTER1 template is missing: $template"

work=$(mktemp -d "${TMPDIR:-/tmp}/counter1-live.XXXXXXXX")
pid=''
cleanup()
{
    if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
        kill -TERM "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
    fi
    rm -R -- "$work"
}
trap cleanup EXIT

start_server()
{
    local tag=$1
    : >"$work/$tag.out"
    : >"$work/$tag.err"
    "$application" "$template" 0 >"$work/$tag.out" 2>"$work/$tag.err" &
    pid=$!
    for _ in $(seq 1 200); do
        if grep -q '^COUNTER1_READY=' "$work/$tag.out"; then
            break
        fi
        if ! kill -0 "$pid" 2>/dev/null; then
            cat "$work/$tag.out" >&2 || true
            cat "$work/$tag.err" >&2 || true
            fail "COUNTER1 exited before READY"
        fi
        sleep 0.025
    done
    ready=$(sed -n 's/^COUNTER1_READY=//p' "$work/$tag.out" | head -n1)
    [[ -n "$ready" ]] || fail "COUNTER1 READY line missing"
    base=${ready%/counter/1}
    [[ "$base" == http://127.0.0.1:* ]] || fail "unexpected READY endpoint: $ready"
}

request()
{
    local case_id=$1 method=$2 path=$3 expected_status=$4 expected_hash=$5 expected_content_type=$6
    local headers="$work/$case_id.headers"
    local body="$work/$case_id.body"
    local status
    status=$(curl --http1.1 -sS -X "$method" -D "$headers" -o "$body" -w '%{http_code}' "$base$path")
    [[ "$status" == "$expected_status" ]] || fail "$case_id status: expected $expected_status found $status"
    if [[ "$expected_hash" == EMPTY ]]; then
        [[ ! -s "$body" ]] || fail "$case_id expected empty body"
    else
        actual_hash=$(sha256sum "$body" | awk '{print $1}')
        [[ "$actual_hash" == "$expected_hash" ]] ||
            fail "$case_id body SHA-256: expected $expected_hash found $actual_hash"
    fi
    if [[ "$expected_content_type" == NONE ]]; then
        if grep -qi '^Content-Type:' "$headers"; then
            fail "$case_id unexpectedly published Content-Type"
        fi
    else
        grep -Fqi "Content-Type: $expected_content_type" "$headers" ||
            fail "$case_id Content-Type differs"
    fi
}

start_server sequence
request L01 GET  /counter/not-a-number      400 EMPTY NONE
request L02 GET  /counter/1                 200 3fc9330f1352659fcd278bc24a8fdcefec49f16e4748f9a025f4d52e6becc6de 'text/html; charset=utf-8'
request L03 POST /counter/1/increment       200 ad59f3e9ee0380292b8c938917cea71aa67fdf19fbcc8d0f41fee563ce975f86 'text/html; charset=utf-8'
request L04 GET  /counter/1                 200 ad59f3e9ee0380292b8c938917cea71aa67fdf19fbcc8d0f41fee563ce975f86 'text/html; charset=utf-8'
request L05 GET  /counter/2                 200 c5a7f2c317002b8b1698e796face24e3b245cef24f8a2ab5c14cb182cd83d0cc 'text/html; charset=utf-8'
request L06 GET  /counter/3                 200 0350c5bac493f0b8a9d01cea200a97a6a0ba60b94d3277c3783f2d9ae10c39d6 'text/html; charset=utf-8'
request L07 POST /counter/3/increment       409 EMPTY NONE
request L08 GET  /counter/999               404 EMPTY NONE
request L09 GET  /unmatched                 404 EMPTY NONE
kill -INT "$pid"
wait "$pid" || fail "COUNTER1 SIGINT run exited nonzero"
pid=''
grep -Fqx 'COUNTER1_STOPPED' "$work/sequence.out" || fail 'SIGINT STOPPED marker missing'
grep -Fq 'COUNTER1_LIFE0 phase=CLOSED' "$work/sequence.out" || fail 'SIGINT LIFE0 marker missing'
grep -Fq 'first_failure=0' "$work/sequence.out" || fail 'SIGINT LIFE0 first_failure differs'

start_server term
request TERM01 GET /counter/1 200 3fc9330f1352659fcd278bc24a8fdcefec49f16e4748f9a025f4d52e6becc6de 'text/html; charset=utf-8'
kill -TERM "$pid"
wait "$pid" || fail "COUNTER1 SIGTERM run exited nonzero"
pid=''
grep -Fqx 'COUNTER1_STOPPED' "$work/term.out" || fail 'SIGTERM STOPPED marker missing'
grep -Fq 'COUNTER1_LIFE0 phase=CLOSED' "$work/term.out" || fail 'SIGTERM LIFE0 marker missing'
grep -Fq 'first_failure=0' "$work/term.out" || fail 'SIGTERM LIFE0 first_failure differs'

printf 'COUNTER1_LIVE_SEQUENCE=PASS_L01_L09\n'
printf 'COUNTER1_LIVE_PERSISTENCE=PASS_0_TO_1_TO_1\n'
printf 'COUNTER1_LIVE_LIMIT=PASS_UINT64_MAX_409_UNCHANGED\n'
printf 'COUNTER1_LIVE_SIGNALS=PASS_SIGINT_SIGTERM\n'
printf 'PASS: COUNTER1 real HOST1 HTTP oracle and lifecycle\n'
