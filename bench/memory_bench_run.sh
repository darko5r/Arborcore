#!/usr/bin/env bash

set -euo pipefail

sizes=(
    "7:20000000"
    "8:20000000"

    "112:10000000"
    "119:10000000"
    "120:10000000"
    "121:10000000"
    "122:10000000"
    "123:10000000"
    "124:10000000"
    "125:10000000"
    "126:10000000"
    "127:10000000"
    "128:5000000"
    "129:5000000"

    "136:5000000"
    "144:5000000"
    "152:4000000"
    "160:4000000"
)

tmpdir="$(mktemp -d /tmp/arborcore-memory-bench.XXXXXX)"

trap 'rm -rf "$tmpdir"' EXIT


build_and_run()
{
    local function="$1"
    local size="$2"
    local iterations="$3"

    local object="$tmpdir/${function}-${size}.o"
    local executable="$tmpdir/${function}-${size}"

    nasm \
        -f elf64 \
        -Wall \
        -Wno-reloc-rel-dword \
        -Wno-reloc-abs-qword \
        -DCOPY_FUNCTION="$function" \
        -DCOPY_SIZE="$size" \
        -DITERATIONS="$iterations" \
        memory_bench.asm \
        -o "$object"

    ld \
        -o "$executable" \
        "$object" \
        memory.o \
        write.o

    "$executable" |
        od -An -tu8 |
        tr -d '[:space:]'
}


printf '%-10s %-12s %-16s %-16s %-16s %-16s\n' \
    "bytes" \
    "iterations" \
    "scalar_ns" \
    "qword_ns" \
    "rep_ns" \
    "public_ns"


for entry in "${sizes[@]}"
do
    size="${entry%%:*}"
    iterations="${entry##*:}"

    scalar="$(
        build_and_run \
            memory_copy_scalar \
            "$size" \
            "$iterations"
    )"

    qword="$(
        build_and_run \
            memory_copy_qword \
            "$size" \
            "$iterations"
    )"

    rep="$(
        build_and_run \
            memory_copy_rep \
            "$size" \
            "$iterations"
    )"

    public="$(
        build_and_run \
            memory_copy \
            "$size" \
            "$iterations"
    )"

    printf '%-10s %-12s %-16s %-16s %-16s %-16s\n' \
        "$size" \
        "$iterations" \
        "$scalar" \
        "$qword" \
        "$rep" \
        "$public"
done
