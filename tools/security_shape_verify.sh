#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OBJ="$ROOT/build/security.o"
[[ -r "$OBJ" ]] || { echo "FAIL: missing $OBJ" >&2; exit 2; }

clear_dis="$(objdump -d -Mintel --disassemble=memory_secure_clear "$OBJ")"
equal_dis="$(objdump -d -Mintel --disassemble=memory_equal_constant_time "$OBJ")"

printf '%s\n' "$clear_dis" | grep -Eq 'rep[[:space:]]+stos' || {
  echo "FAIL: memory_secure_clear is not an explicit REP STOSB overwrite." >&2
  exit 1
}
if printf '%s\n' "$clear_dis" | grep -Eq '[[:space:]]call[[:space:]]'; then
  echo "FAIL: memory_secure_clear must not delegate to an optimizable helper." >&2
  exit 1
fi

if printf '%s\n' "$equal_dis" | grep -Eq '[[:space:]]call[[:space:]]'; then
  echo "FAIL: constant-time equality must remain self-contained." >&2
  exit 1
fi
if printf '%s\n' "$equal_dis" | grep -Eq '\bcmov[a-z]*\b'; then
  echo "FAIL: unexpected data-selection control operation in constant-time equality." >&2
  exit 1
fi
# Exactly two conditional jumps are expected: zero-length skip and the
# length-controlled loop back-edge.  The content fold itself has no branch.
branch_count="$(printf '%s\n' "$equal_dis" | awk '
  /^[[:space:]]*[0-9a-f]+:/ {
    for (i=1;i<=NF;i++) if ($i ~ /^j[a-z]+$/ && $i != "jmp") c++
  }
  END {print c+0}
')"
if [[ "$branch_count" -ne 2 ]]; then
  echo "FAIL: memory_equal_constant_time conditional-branch count=$branch_count expected=2" >&2
  printf '%s\n' "$equal_dis" >&2
  exit 1
fi
printf '%s\n' "$equal_dis" | grep -Eq '\bsete\b' || {
  echo "FAIL: equality result is not normalized through a branchless SETE result." >&2
  exit 1
}

echo "constant_time_conditional_branches=$branch_count"
echo "PASS: security primitive instruction-shape contract"
