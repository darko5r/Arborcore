#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODE="${1:-candidate}"

case "$MODE" in
    candidate) strict_target=verify-server-performance-candidate ;;
    strict)    strict_target=verify-server-performance ;;
    *) echo "usage: $0 [candidate|strict]" >&2; exit 2 ;;
esac

set +e
admissibility_output="$(bash "$ROOT/tools/server_environment_admissibility.sh" 2>&1)"
admissibility_rc=$?
set -e
printf '%s\n' "$admissibility_output"

case "$admissibility_rc" in
    0)
        echo "QUALIFICATION_MODE=HISTORICAL_STRICT"
        make -C "$ROOT" ARBORCORE_PERF_PROFILE="${ARBORCORE_PERF_PROFILE:-local}" "$strict_target"
        ;;
    10)
        echo "QUALIFICATION_MODE=PAIRED_REFERENCE"
        bash "$ROOT/tools/server_paired_compare.sh"
        ;;
    *)
        echo "FAIL: benchmark environment qualification is indeterminate." >&2
        exit "$admissibility_rc"
        ;;
esac
