#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(pwd)}"
PROFILE="${ARBORCORE_PERF_PROFILE:-local}"

ROUNDS="${ARBORCORE_MEMORY_ROUNDS:-3}"
SAMPLES="${ARBORCORE_MEMORY_SAMPLES:-7}"
WARMUPS="${ARBORCORE_MEMORY_WARMUPS:-2}"

CONFIRM_ROUNDS="${ARBORCORE_MEMORY_CONFIRM_ROUNDS:-3}"
CONFIRM_SAMPLES="${ARBORCORE_MEMORY_CONFIRM_SAMPLES:-11}"

QWORD_WIN_PCT="${ARBORCORE_MEMORY_QWORD_WIN_PCT:-10}"
REP_WIN_PCT="${ARBORCORE_MEMORY_REP_WIN_PCT:-5}"
CONSECUTIVE="${ARBORCORE_MEMORY_CONSECUTIVE_SIZES:-3}"

if [[ -n "${ARBORCORE_BENCH_CPU:-}" ]]; then
    CPU="$ARBORCORE_BENCH_CPU"
else
    allowed="$(awk '/^Cpus_allowed_list:/ {print $2}' /proc/self/status 2>/dev/null || true)"
    first="${allowed%%,*}"
    CPU="${first%%-*}"
    CPU="${CPU:-0}"
fi

tmpdir="$(mktemp -d /tmp/arborcore-memory-qualify.XXXXXX)"
trap 'rm -rf "$tmpdir"' EXIT

run_pinned() {
    if command -v taskset >/dev/null 2>&1; then
        taskset -c "$CPU" "$@"
    else
        "$@"
    fi
}

iterations_for_size() {
    local size="$1"
    if (( size <= 16 )); then
        echo 4000000
    elif (( size <= 64 )); then
        echo 2000000
    elif (( size <= 128 )); then
        echo 1000000
    else
        echo 600000
    fi
}

build_benchmark() {
    local function="$1" size="$2" iterations="$3"
    local object="$tmpdir/${function}-${size}.o"
    local executable="$tmpdir/${function}-${size}"

    if [[ ! -x "$executable" ]]; then
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

        ld -o "$executable" "$object" memory.o write.o
    fi

    printf '%s\n' "$executable"
}

read_elapsed() {
    local executable="$1"
    run_pinned "$executable" | od -An -tu8 | tr -d '[:space:]'
}

median_numbers() {
    sort -n | awk '
        { a[NR] = $1 }
        END {
            if (NR == 0) exit 2
            if (NR % 2) print a[(NR + 1) / 2]
            else print int((a[NR / 2] + a[NR / 2 + 1]) / 2)
        }
    '
}

# Output:
# median:min:max:spread_basis_points
measure_with_shape() {
    local function="$1" size="$2" rounds="$3" samples="$4"
    local iterations executable round sample value round_median
    local values=()
    local round_values=()

    iterations="$(iterations_for_size "$size")"
    executable="$(build_benchmark "$function" "$size" "$iterations")"

    for ((round=1; round<=rounds; round++)); do
        for ((sample=1; sample<=WARMUPS; sample++)); do
            read_elapsed "$executable" >/dev/null
        done

        values=()
        for ((sample=1; sample<=samples; sample++)); do
            value="$(read_elapsed "$executable")"
            values+=("$value")
        done

        round_median="$(printf '%s\n' "${values[@]}" | median_numbers)"
        round_values+=("$round_median")
    done

    median="$(printf '%s\n' "${round_values[@]}" | median_numbers)"
    min="$(printf '%s\n' "${round_values[@]}" | sort -n | head -n1)"
    max="$(printf '%s\n' "${round_values[@]}" | sort -n | tail -n1)"

    spread_bp="$(
        awk -v lo="$min" -v hi="$max" -v med="$median" '
            BEGIN {
                if (med == 0) print 0
                else printf "%d\n", ((hi - lo) * 10000.0 / med) + 0.5
            }
        '
    )"

    printf '%s:%s:%s:%s\n' "$median" "$min" "$max" "$spread_bp"
}

field() {
    printf '%s\n' "$1" | cut -d: -f"$2"
}

wins_by_percent() {
    local candidate="$1" reference="$2" win_pct="$3"
    (( candidate * 100 <= reference * (100 - win_pct) ))
}

triple_wins() {
    local candidate_name="$1" reference_name="$2" win_pct="$3"
    shift 3
    local size candidate reference
    for size in "$@"; do
        candidate="$(field "$(eval "printf '%s' \"\${${candidate_name}[$size]}\"")" 1)"
        reference="$(field "$(eval "printf '%s' \"\${${reference_name}[$size]}\"")" 1)"
        wins_by_percent "$candidate" "$reference" "$win_pct" || return 1
    done
    return 0
}

confirm_region() {
    local candidate_function="$1" reference_function="$2" win_pct="$3"
    shift 3
    local size candidate reference
    local pass=1

    echo "  confirmation:"
    for size in "$@"; do
        reference="$(measure_with_shape "$reference_function" "$size" "$CONFIRM_ROUNDS" "$CONFIRM_SAMPLES")"
        candidate="$(measure_with_shape "$candidate_function" "$size" "$CONFIRM_ROUNDS" "$CONFIRM_SAMPLES")"

        printf '    size=%s reference=%s candidate=%s\n' "$size" "$reference" "$candidate"

        if ! wins_by_percent "$(field "$candidate" 1)" "$(field "$reference" 1)" "$win_pct"; then
            pass=0
        fi
    done

    (( pass == 1 ))
}

current_qword="$(awk '/^%define MEMORY_COPY_QWORD_MIN/ {print $3}' generated/memory_thresholds.inc)"
current_rep="$(awk '/^%define MEMORY_COPY_REP_MIN/ {print $3}' generated/memory_thresholds.inc)"

echo "### Arborcore memory qualification v3"
echo "profile=$PROFILE CPU=$CPU"
echo "explore=${ROUNDS}x${SAMPLES} confirm=${CONFIRM_ROUNDS}x${CONFIRM_SAMPLES} warmups=$WARMUPS"
echo "current_qword_min=$current_qword current_rep_min=$current_rep"
echo

# -------------------------------
# Scalar -> qword
# -------------------------------
declare -A scalar_small qword_small

for size in {1..16}; do
    scalar_small["$size"]="$(measure_with_shape memory_copy_scalar "$size" "$ROUNDS" "$SAMPLES")"
    qword_small["$size"]="$(measure_with_shape memory_copy_qword "$size" "$ROUNDS" "$SAMPLES")"

    printf 'small %2d: scalar=%s qword=%s\n' \
        "$size" "${scalar_small[$size]}" "${qword_small[$size]}"
done

qword_min="$current_qword"
qword_confirmed=0

for size in {1..14}; do
    sizes=("$size" "$((size + 1))" "$((size + 2))")

    if triple_wins qword_small scalar_small "$QWORD_WIN_PCT" "${sizes[@]}"; then
        echo
        echo "Provisional qword minimum: $size bytes"
        if confirm_region memory_copy_qword memory_copy_scalar "$QWORD_WIN_PCT" "${sizes[@]}"; then
            qword_min="$size"
            qword_confirmed=1
            break
        fi
        echo "  confirmation rejected this region"
    fi
done

if (( qword_confirmed == 0 )); then
    echo
    echo "No newly confirmed qword crossover; retaining $current_qword bytes"
fi

echo
echo "Selected qword minimum: $qword_min bytes"
echo

# -------------------------------
# Qword -> REP
# -------------------------------
rep_sizes=()
for ((size=64; size<=256; size+=8)); do
    rep_sizes+=("$size")
done

declare -A qword_large rep_large

for size in "${rep_sizes[@]}"; do
    qword_large["$size"]="$(measure_with_shape memory_copy_qword "$size" "$ROUNDS" "$SAMPLES")"
    rep_large["$size"]="$(measure_with_shape memory_copy_rep "$size" "$ROUNDS" "$SAMPLES")"

    printf 'large %3d: qword=%s rep=%s\n' \
        "$size" "${qword_large[$size]}" "${rep_large[$size]}"
done

rep_min="$current_rep"
rep_confirmed=0

for ((i=0; i<=${#rep_sizes[@]}-CONSECUTIVE; i++)); do
    sizes=()
    for ((j=0; j<CONSECUTIVE; j++)); do
        sizes+=("${rep_sizes[$((i + j))]}")
    done

    if triple_wins rep_large qword_large "$REP_WIN_PCT" "${sizes[@]}"; then
        echo
        echo "Provisional REP minimum: ${sizes[0]} bytes"
        if confirm_region memory_copy_rep memory_copy_qword "$REP_WIN_PCT" "${sizes[@]}"; then
            rep_min="${sizes[0]}"
            rep_confirmed=1
            break
        fi
        echo "  confirmation rejected this region"
    fi
done

if (( rep_confirmed == 0 )); then
    echo
    echo "No newly confirmed REP crossover; retaining $current_rep bytes"
fi

echo
echo "Selected REP minimum: $rep_min bytes"

if (( rep_min <= qword_min )); then
    echo "FAIL: invalid generated thresholds" >&2
    exit 1
fi

mkdir -p generated generated/performance

output="$(mktemp generated/memory_thresholds.inc.XXXXXX)"
cat > "$output" <<EOF
; Generated by Arborcore memory qualification v3.
;
; Machine-specific policy. Do not manually tune these values.

%define MEMORY_COPY_QWORD_MIN $qword_min
%define MEMORY_COPY_REP_MIN   $rep_min
EOF
mv "$output" generated/memory_thresholds.inc

profile_path="generated/performance/memory-${PROFILE}.env"
profile_tmp="$(mktemp "${profile_path}.XXXXXX")"

git_head="$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || true)"
tree_state=clean
if ! git -C "$ROOT" diff --quiet -- src/asm \
   || ! git -C "$ROOT" diff --cached --quiet -- src/asm \
   || [[ -n "$(git -C "$ROOT" ls-files --others --exclude-standard -- src/asm)" ]]
then
    tree_state=dirty
fi

source_sha="$(
    cd "$ROOT"
    find src/asm -maxdepth 1 -type f -name '*.asm' -print0 \
        | sort -z \
        | xargs -0 sha256sum \
        | sha256sum \
        | awk '{print $1}'
)"

cat > "$profile_tmp" <<EOF
# Arborcore machine-specific memory-copy qualification profile.
BASELINE_VERSION=3
PERFORMANCE_PROFILE=$PROFILE
POLICY_STATE=ACCEPTED
PRODUCTION_COMMIT=$git_head
PRODUCTION_TREE_STATE=$tree_state
PRODUCTION_SOURCE_SHA256=$source_sha
BENCH_CPU=$CPU
EXPLORATORY_ROUNDS=$ROUNDS
EXPLORATORY_SAMPLES_PER_ROUND=$SAMPLES
CONFIRMATION_ROUNDS=$CONFIRM_ROUNDS
CONFIRMATION_SAMPLES_PER_ROUND=$CONFIRM_SAMPLES
WARMUPS_PER_ROUND=$WARMUPS
QWORD_REQUIRED_WIN_PCT=$QWORD_WIN_PCT
REP_REQUIRED_WIN_PCT=$REP_WIN_PCT
CONSECUTIVE_SIZES_REQUIRED=$CONSECUTIVE
MEMORY_COPY_QWORD_MIN=$qword_min
MEMORY_COPY_REP_MIN=$rep_min
CPU_MODEL=$(lscpu | awk -F: '/^Model name:/ {sub(/^[ \t]+/, "", $2); print $2; exit}' | sed 's/ /\\ /g')
KERNEL=$(uname -srmo | sed 's/ /\\ /g')
EOF
mv "$profile_tmp" "$profile_path"

echo
echo "Policy written to: generated/memory_thresholds.inc"
echo "Profile written to: $profile_path"
