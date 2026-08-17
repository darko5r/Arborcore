#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"
make -s ddd-support-native-test
make -s ddd-support-adversarial-test
printf 'PASS: AF4 native functional and adversarial qualification\n'
