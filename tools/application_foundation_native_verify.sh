#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
make application-foundation-native-test
make application-foundation-sanitize
