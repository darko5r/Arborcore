#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
make application-service-runtime-native-test
make application-service-runtime-adversarial-test
make application-service-runtime-sanitize
echo 'PASS: AF3 native, adversarial and ASan/UBSan service-runtime qualification'
