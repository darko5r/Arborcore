#!/usr/bin/env bash
set -euo pipefail

root=${ARBORCORE_ROOT:-$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)}
server=${ECHO0_SERVER:-$root/build/echo0/echo0}
template=${ECHO0_TEMPLATE:-$root/examples/echo0/page.html}
requested_port=${ECHO0_PORT:-0}

[[ -x "$server" ]] || {
    printf 'FAIL: ECHO0 server is not executable: %s\n' "$server" >&2
    exit 1
}
[[ -f "$template" ]] || {
    printf 'FAIL: ECHO0 template is missing: %s\n' "$template" >&2
    exit 1
}
command -v curl >/dev/null 2>&1 || {
    printf 'FAIL: curl is required for ECHO0 live verification\n' >&2
    exit 1
}
command -v rg >/dev/null 2>&1 || {
    printf 'FAIL: rg is required for ECHO0 live verification\n' >&2
    exit 1
}

work=$(mktemp -d "${TMPDIR:-/tmp}/arborcore-echo0-live.XXXXXXXX")
server_pid=''
cleanup() {
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
        kill -TERM "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
    if [[ "$work" == "${TMPDIR:-/tmp}/arborcore-echo0-live."* ]]; then
        rm -R -- "$work"
    fi
}
trap cleanup EXIT

"$server" "$template" "$requested_port" >"$work/server.log" 2>&1 &
server_pid=$!

ready=''
for _ in {1..100}; do
    ready=$(sed -n 's/^ECHO0_READY=//p' "$work/server.log" | tail -n 1)
    if [[ -n "$ready" ]]; then
        break
    fi
    if ! kill -0 "$server_pid" 2>/dev/null; then
        printf 'FAIL: ECHO0 exited before readiness\n' >&2
        sed -n '1,120p' "$work/server.log" >&2
        exit 1
    fi
    sleep 0.05
done
[[ "$ready" =~ ^http://127\.0\.0\.1:[0-9]+/echo/Arborcore$ ]] || {
    printf 'FAIL: ECHO0 readiness URL was not published\n' >&2
    sed -n '1,120p' "$work/server.log" >&2
    exit 1
}
base=${ready%/echo/Arborcore}
page_target='/echo/A&B-Ol%C3%A1'

curl --http1.1 --silent --show-error --max-time 5 \
    --header 'Connection: close' \
    --request-target "$page_target" \
    --dump-header "$work/page.headers" \
    --output "$work/page.body" \
    "$base/"
curl --http1.1 --silent --show-error --max-time 5 \
    --header 'Connection: close' \
    --dump-header "$work/redirect.headers" \
    --output "$work/redirect.body" \
    "$base/"
curl --http1.1 --silent --show-error --max-time 5 \
    --header 'Connection: close' \
    --dump-header "$work/missing.headers" \
    --output "$work/missing.body" \
    "$base/missing"

rg -q '^HTTP/1\.1 200 OK\r?$' "$work/page.headers"
[[ $(rg -c '^Cache-Control: no-store\r?$' "$work/page.headers") -eq 1 ]]
[[ $(rg -c '^Content-Type: text/html; charset=utf-8\r?$' "$work/page.headers") -eq 1 ]]
rg -q '^Connection: close\r?$' "$work/page.headers"
rg -q '^<!doctype html>$' "$work/page.body"
rg -q '<html lang="en">' "$work/page.body"
rg -q '<meta charset="utf-8">' "$work/page.body"
rg -q '<h1>Arborcore ECHO0</h1>' "$work/page.body"
rg -Fq '<p>Echo: A&amp;B-Ol%C3%A1</p>' \
    "$work/page.body"

rg -q '^HTTP/1\.1 302 Found\r?$' "$work/redirect.headers"
[[ $(rg -c '^Cache-Control: no-store\r?$' "$work/redirect.headers") -eq 1 ]]
[[ $(rg -c '^Location: /echo/Arborcore\r?$' "$work/redirect.headers") -eq 1 ]]
rg -q '^Content-Length: 0\r?$' "$work/redirect.headers"
[[ ! -s "$work/redirect.body" ]]

rg -q '^HTTP/1\.1 404 Not Found\r?$' "$work/missing.headers"
rg -q '^Content-Length: 0\r?$' "$work/missing.headers"
[[ ! -s "$work/missing.body" ]]
! rg -q '^Cache-Control:' "$work/missing.headers"

kill -TERM "$server_pid"
wait "$server_pid"
server_pid=''
rg -q '^ECHO0_STOPPED middleware=2 controller=2 service=2 presenter=2$' \
    "$work/server.log"

printf 'ECHO0_BROWSER_URL=%s\n' "$ready"
printf 'PASS: ECHO0 live loopback route values, escaping, fields and shutdown\n'
