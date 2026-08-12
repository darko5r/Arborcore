#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
STATIC="$BUILD/libarborcore.a"
SHARED_FULL="$BUILD/libarborcore.so.1.0.0"
SHARED="$BUILD/libarborcore.so.1"
SHARED_LINK="$BUILD/libarborcore.so"
STATIC_CONSUMER="$BUILD/abi-static-consumer"
SHARED_CONSUMER="$BUILD/abi-shared-consumer"
PUBLIC="$ROOT/abi/arborcore-1.symbols"
INTERNAL="$ROOT/abi/arborcore-1.internal-symbols"

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

for f in "$STATIC" "$SHARED_FULL" "$STATIC_CONSUMER" "$SHARED_CONSUMER"; do
  [[ -r "$f" ]] || { echo "FAIL: missing readiness artifact $f" >&2; exit 2; }
done
[[ -L "$SHARED" && "$(readlink "$SHARED")" == "libarborcore.so.1.0.0" ]] || {
  echo "FAIL: libarborcore.so.1 symlink is missing or incorrect" >&2
  exit 1
}
[[ -L "$SHARED_LINK" && "$(readlink "$SHARED_LINK")" == "libarborcore.so.1" ]] || {
  echo "FAIL: libarborcore.so development symlink is missing or incorrect" >&2
  exit 1
}

# S8: deterministic archive membership. start.o must never be a library member.
expected_members=(
  write.o memory_threshold.o memory.o security.o bytes.o ascii.o bytes_scan.o
  parse_u64.o u64_checked.o range.o u64_format.o hex_codec.o percent_codec.o
  base64.o buffer.o arena.o io.o net.o http_parser.o router.o event.o
  connection.o http_response.o request_target.o route_pattern.o server.o
)
printf '%s\n' "${expected_members[@]}" | sort > "$tmp/expected-members"
ar t "$STATIC" | sort > "$tmp/actual-members"
if ! diff -u "$tmp/expected-members" "$tmp/actual-members"; then
  echo "FAIL: static archive membership differs from ABI readiness policy." >&2
  exit 1
fi
if ar t "$STATIC" | grep -qx 'start.o'; then
  echo "FAIL: process entry object start.o leaked into libarborcore.a" >&2
  exit 1
fi

"$STATIC_CONSUMER" || { echo "FAIL: static ABI consumer execution" >&2; exit 1; }

# S7: shared object must be self-contained, hardened and export exactly v1.
readelf -dW "$SHARED_FULL" > "$tmp/dynamic"
readelf -lW "$SHARED_FULL" > "$tmp/program"
readelf -rW "$SHARED_FULL" > "$tmp/relocs"

if grep -q '(NEEDED)' "$tmp/dynamic"; then
  echo "FAIL: libarborcore.so.1 unexpectedly depends on another shared library." >&2
  grep '(NEEDED)' "$tmp/dynamic" >&2
  exit 1
fi
grep -Fq 'Library soname: [libarborcore.so.1]' "$tmp/dynamic" || {
  echo "FAIL: SONAME is not libarborcore.so.1" >&2
  exit 1
}
if grep -Eq 'TEXTREL|FLAGS.*TEXTREL' "$tmp/dynamic"; then
  echo "FAIL: shared-object readiness produced TEXTREL." >&2
  exit 1
fi
if grep -Eq 'R_X86_64_(32|32S)([^A-Za-z0-9_]|$)' "$tmp/relocs"; then
  echo "FAIL: non-PIC 32-bit absolute relocation remains in shared object." >&2
  grep -E 'R_X86_64_(32|32S)' "$tmp/relocs" >&2
  exit 1
fi
stack_line="$(grep 'GNU_STACK' "$tmp/program" || true)"
[[ -n "$stack_line" ]] || { echo "FAIL: shared object has no GNU_STACK header" >&2; exit 1; }
stack_flags="$(awk '$1=="GNU_STACK" {print $(NF-1); exit}' "$tmp/program")"
if [[ "$stack_flags" == *E* ]]; then
  echo "FAIL: shared object GNU_STACK is executable: $stack_line" >&2
  exit 1
fi

nm -D --defined-only "$SHARED_FULL" \
  | awk 'NF >= 3 {name=$3; sub(/@.*/,"",name); if (name != "ARBORCORE_1.0") print name}' \
  | sort -u > "$tmp/dyn-public"
if ! diff -u "$PUBLIC" "$tmp/dyn-public"; then
  echo "FAIL: shared dynamic symbol surface differs from ABI v1 manifest." >&2
  exit 1
fi
while IFS= read -r sym; do
  [[ -z "$sym" ]] && continue
  if grep -Fxq "$sym" "$tmp/dyn-public"; then
    echo "FAIL: internal symbol exported dynamically: $sym" >&2
    exit 1
  fi
done < "$INTERNAL"

LD_LIBRARY_PATH="$BUILD${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "$SHARED_CONSUMER" || {
  echo "FAIL: shared ABI consumer execution" >&2
  exit 1
}

# Consumer and libraries must also retain non-executable stacks.
for elf in "$STATIC_CONSUMER" "$SHARED_CONSUMER" "$SHARED_FULL"; do
  program_out="$(readelf -lW "$elf")"
  line="$(printf '%s\n' "$program_out" | grep 'GNU_STACK' || true)"
  flags="$(printf '%s\n' "$program_out" | awk '$1=="GNU_STACK" {print $(NF-1); exit}')"
  [[ -n "$line" && "$flags" != *E* ]] || {
    echo "FAIL: executable/missing GNU_STACK for $elf" >&2
    exit 1
  }
done

static_sha="$(sha256sum "$STATIC" | awk '{print $1}')"
shared_sha="$(sha256sum "$SHARED_FULL" | awk '{print $1}')"
echo "static_library_sha256=$static_sha"
echo "shared_library_sha256=$shared_sha"
echo "shared_export_count=$(wc -l < "$tmp/dyn-public")"
echo "PASS: static and shared library readiness"
