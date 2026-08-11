#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
CPU="$(arborcore_bench_cpu)"
OUT="$ROOT/build/server-benchmark-syscalls"
mkdir -p "$OUT"

if ! command -v strace >/dev/null 2>&1; then
    echo "SKIP: strace is not installed."
    exit 0
fi

trace='read,write,epoll_wait,epoll_ctl,epoll_create1,accept4,close,connect,socket,bind,listen,shutdown,fcntl'

echo "### strace syscall profile: loopback keep-alive benchmark"
strace -qq -c -e trace="$trace" taskset -c "$CPU" "$ROOT/build/bench-loopback" \
    >/dev/null 2>"$OUT/loopback.txt"
cat "$OUT/loopback.txt"
echo
echo "Diagnostic only: strace instrumentation is not used for timing qualification."
