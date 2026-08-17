#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"; cd "$ROOT"
make -s mvc0-core-test
make -s mvc0-adversarial-test
make -s mvc0-integration-test
make -s mvc0-end-to-end-test
printf 'PASS: MVC0 core, adversarial, AF2/AF3/AF4 integration and real-socket qualification
'
