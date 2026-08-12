#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
CPU="$(arborcore_bench_cpu)"
OUT="$ROOT/build/runtime-syscall-evidence"
mkdir -p "$OUT"
if ! command -v strace >/dev/null 2>&1; then
  echo "E7_SYSCALL_EVIDENCE=SKIP_STRACE_UNAVAILABLE"
  exit 0
fi
trace="$OUT/trace.txt"
strace -qq -e trace=epoll_ctl,fcntl -o "$trace" taskset -c "$CPU" "$ROOT/build/bench-loopback" >/dev/null
epoll_ctl_count="$(grep -c 'epoll_ctl(' "$trace" || true)"
fcntl_count="$(grep -c 'fcntl(' "$trace" || true)"
echo "epoll_ctl_count=$epoll_ctl_count"
echo "fcntl_count=$fcntl_count"
# 1501 keep-alive requests should no longer toggle EPOLLOUT per response.
if (( epoll_ctl_count > 10 )); then
  echo "REVIEW REQUIRED: immediate-write path still performs excessive epoll_ctl operations." >&2
  exit 1
fi
if (( fcntl_count != 0 )); then
  echo "REVIEW REQUIRED: listener setup still uses fcntl despite atomic socket flags." >&2
  exit 1
fi
echo "E6_DECISION=ADMIT_ATOMIC_LISTENER_FLAGS"
echo "E7_DECISION=ADMIT_IMMEDIATE_WRITE_EPOLL_ARM_ON_EAGAIN"
