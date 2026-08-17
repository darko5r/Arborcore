#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"; cd "$ROOT"
echo '### MVC0 SCOPE / BASELINE / CONTRACT'
bash tools/mvc0_scope_verify.sh
bash tools/mvc0_baseline_verify.sh
bash tools/mvc0_contract_verify.sh
echo '### MVC0 NATIVE / REAL SOCKET / ABI'
bash tools/mvc0_native_verify.sh
bash tools/mvc0_abi_verify.sh
echo '### MVC0 SANITIZERS'
make -s mvc0-sanitize
echo '### FROZEN C RUNTIME / LEGACY SERVER REGRESSION'
make -s c-runtime-check
make -s c-runtime-sanitize
make -s server-test
build/server-test
echo 'PASS: frozen C runtime + legacy status-only server regression'
echo '### AF0-AF4 APPLICATION FOUNDATION REGRESSION'
make -s application-foundation-native-test
make -s application-foundation-sanitize
make -s application-capability-kernel-native-test
make -s application-capability-kernel-sanitize
make -s application-service-runtime-native-test
make -s application-service-runtime-adversarial-test
make -s application-service-runtime-sanitize
make -s ddd-support-native-test
make -s ddd-support-adversarial-test
make -s ddd-support-sanitize
echo 'PASS: AF0-AF4 regression qualification'
echo '### MVC0 GCC ANALYZER'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
for src in src/c/mvc.c src/c/application_transport.c; do cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -fPIC -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -c "$src" -o "$tmp/$(basename "$src" .c).o"; done
echo 'PASS: MVC0 GCC -fanalyzer'
echo '### MVC0 REPRODUCIBILITY'
bash tools/mvc0_reproducibility_verify.sh
echo '### MVC0 FINAL POLICY / DIFF'
git diff --check
if grep -RniE '(^|[^[:alnum:]_])(mysql|mariadb|sqlite|postgres|libpq|RInside|webgpu|javascript)([^[:alnum:]_]|$)' include/arborcore/mvc.h include/arborcore/application_transport.h src/c/mvc.c src/c/application_transport.c src/asm/application_transport.asm; then echo 'FAIL: deferred infrastructure/presentation leakage in MVC0 production' >&2; exit 1; fi
printf 'MVC0_GATE=PASS
MVC0_LOWER_LAYER_RETROFIT_REQUIRED=NO
MVC0_STAGED_CHANGES=NO
MVC0_NEW_COMMIT_CREATED=NO
MVC0_REMOTE_WRITE_PERFORMED=NO
'
