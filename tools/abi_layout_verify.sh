#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LAYOUT="$ROOT/abi/arborcore-1.layout"
[[ -s "$LAYOUT" ]] || { echo "FAIL: missing $LAYOUT" >&2; exit 2; }

require_define() {
  local file="$1" name="$2" expected="$3" actual
  actual="$(awk -v n="$name" '$1=="%define" && $2==n {print $3; exit}' "$ROOT/$file")"
  [[ "$actual" == "$expected" ]] || {
    echo "FAIL: $file $name=$actual expected=$expected" >&2
    exit 1
  }
}
require_layout() {
  local key="$1" value="$2"
  grep -Fxq "$key=$value" "$LAYOUT" || {
    echo "FAIL: ABI layout missing $key=$value" >&2
    exit 1
  }
}

require_define src/asm/buffer.asm BUFFER_SIZE 24
require_define src/asm/buffer.asm BUFFER_DATA 0
require_define src/asm/buffer.asm BUFFER_LENGTH 8
require_define src/asm/buffer.asm BUFFER_CAPACITY 16
require_define src/asm/arena.asm ARENA_SIZE 24
require_define src/asm/arena.asm ARENA_BASE 0
require_define src/asm/arena.asm ARENA_CAPACITY 8
require_define src/asm/arena.asm ARENA_OFFSET 16
require_define src/asm/connection.asm CONNECTION_SIZE 96
require_define src/asm/http_parser.asm HTTP_REQUEST_SIZE 96
require_define src/asm/request_target.asm REQUEST_TARGET_SIZE 32
require_define src/asm/route_pattern.asm ROUTE_SIZE 40
require_define src/asm/route_pattern.asm ROUTE_PARAM_SIZE 32
require_define src/asm/event.asm EPOLL_EVENT_SIZE 12
require_define src/asm/event.asm EPOLL_EVENT_EVENTS 0
require_define src/asm/event.asm EPOLL_EVENT_DATA 4
require_define src/asm/server.asm SERVER_MORE_WORK 1
require_define src/asm/server.asm SERVER_REQUEST_BUDGET 8

for kv in \
  'BUFFER.size 24' 'BUFFER.data 0' 'BUFFER.length 8' 'BUFFER.capacity 16' \
  'ARENA.size 24' 'ARENA.base 0' 'ARENA.capacity 8' 'ARENA.offset 16' \
  'CONNECTION.size 96' 'HTTP_REQUEST.size 96' 'REQUEST_TARGET.size 32' \
  'ROUTE.size 40' 'ROUTE_PARAM.size 32' 'EPOLL_EVENT.size 12' \
  'EPOLL_EVENT.events 0' 'EPOLL_EVENT.data 4' \
  'SERVER_MORE_WORK 1' 'SERVER_REQUEST_BUDGET 8'; do
  key="${kv% *}"; value="${kv##* }"; require_layout "$key" "$value"
done

grep -Fq 'zero-size allocation still honors the requested power-of-two alignment' "$ROOT/docs/CORE_SPEC.md" || {
  echo "FAIL: zero-size arena ABI semantics are not frozen in CORE_SPEC.md" >&2
  exit 1
}
require_layout ARENA.zero_size_aligned_allocation align_and_may_advance_frontier

echo "PASS: ABI-v1 layouts and zero-size arena semantics match production source"
