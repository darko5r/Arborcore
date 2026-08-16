#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
make application-capability-kernel-native-test
make application-capability-kernel-sanitize
echo 'PASS: AF2 native/adversarial and ASan/UBSan capability-kernel qualification'
