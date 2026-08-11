#!/usr/bin/env bash
set -euo pipefail

arborcore_bench_root() {
    cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd
}

arborcore_bench_cpu() {
    if [[ -n "${ARBORCORE_BENCH_CPU:-}" ]]; then
        printf '%s\n' "$ARBORCORE_BENCH_CPU"
        return
    fi

    local allowed first
    allowed="$(awk '/^Cpus_allowed_list:/ {print $2}' /proc/self/status 2>/dev/null || true)"
    if [[ -z "$allowed" ]]; then
        printf '%s\n' 0
        return
    fi
    first="${allowed%%,*}"
    first="${first%%-*}"
    printf '%s\n' "$first"
}

arborcore_run_pinned() {
    local cpu="$1"
    shift
    if command -v taskset >/dev/null 2>&1; then
        taskset -c "$cpu" "$@"
    else
        "$@"
    fi
}

arborcore_production_text_bytes() {
    local root="$1"
    local objects=(
        start write memory_threshold memory bytes ascii bytes_scan parse_u64
        u64_checked range u64_format hex_codec percent_codec base64 buffer arena io net
        http_parser router event connection http_response request_target route_pattern server
    )
    local args=()
    local obj
    for obj in "${objects[@]}"; do
        args+=("$root/build/$obj.o")
    done
    size "${args[@]}" | awk 'NR > 1 { total += $1 } END { printf "%.0f\n", total }'
}


arborcore_perf_profile() {
    local profile="${ARBORCORE_PERF_PROFILE:-local}"
    if [[ ! "$profile" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
        echo "Invalid ARBORCORE_PERF_PROFILE: $profile" >&2
        return 2
    fi
    printf '%s\n' "$profile"
}

arborcore_perf_baseline_path() {
    local root="$1"
    local profile
    profile="$(arborcore_perf_profile)"
    printf '%s/generated/performance/%s.env\n' "$root" "$profile"
}

arborcore_cpu_model() {
    lscpu | awk -F: '/^Model name:/ {
        sub(/^[ \t]+/, "", $2)
        print $2
        exit
    }'
}

arborcore_architecture() {
    uname -m
}

arborcore_scaling_driver() {
    local f=/sys/devices/system/cpu/cpu0/cpufreq/scaling_driver
    if [[ -r "$f" ]]; then
        cat "$f"
    fi
}


arborcore_production_source_sha256() {
    local root="$1"
    (
        cd "$root"
        find src/asm -maxdepth 1 -type f -name '*.asm' -print0 \
            | sort -z \
            | xargs -0 sha256sum \
            | sha256sum \
            | awk '{print $1}'
    )
}
