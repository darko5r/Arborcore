#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
PROFILE="$(arborcore_perf_profile)"
REFERENCE="${ARBORCORE_QUALIFICATION_REFERENCE_COMMIT:?ARBORCORE_QUALIFICATION_REFERENCE_COMMIT is required}"

if ! git -C "$ROOT" cat-file -e "$REFERENCE^{commit}" 2>/dev/null; then
    echo "QUALIFICATION_INDETERMINATE: reference commit is unavailable: $REFERENCE" >&2
    exit 20
fi

workspace="$(mktemp -d /tmp/arborcore-admissibility.XXXXXX)"
ref="$workspace/reference"
log="$workspace/reference.log"

cleanup() {
    git -C "$ROOT" worktree remove --force "$ref" >/dev/null 2>&1 || true
    rm -rf "$workspace"
}
trap cleanup EXIT

git -C "$ROOT" worktree add --detach "$ref" "$REFERENCE" >/dev/null
mkdir -p "$ref/generated"
if [[ -d "$ROOT/generated" ]]; then
    cp -a "$ROOT/generated/." "$ref/generated/"
fi

set +e
(
    cd "$ref"
    make clean >/dev/null
    make ARBORCORE_PERF_PROFILE="$PROFILE" verify-server-performance
) >"$log" 2>&1
rc=$?
set -e

reference_commit="$(git -C "$ref" rev-parse HEAD)"
result_dir="$ref/build/server-performance-verify-${PROFILE}"
summary="$result_dir/summary.tsv"
environment="$result_dir/environment.txt"
envelope_failure='FAIL: one or more median regressions exceeded their accepted metric-specific envelopes.'

if [[ "$rc" -eq 0 ]]; then
    echo "ENVIRONMENT_ADMISSIBLE"
    echo "reference_commit=$reference_commit"
    echo "profile=$PROFILE"
    echo "reference_verify_make_exit=$rc"
    exit 0
fi

# GNU make returns exit status 2 when a recipe fails, even when the invoked
# verifier itself returned 1 for an ordinary performance-envelope failure.
# Therefore make's numeric status alone cannot distinguish:
#
#   (a) completed benchmark + historical envelope rejection
# from
#   (b) build/tool/provenance failure.
#
# Classify from retained evidence instead.  An inadmissible environment must
# have completed benchmark artifacts and the verifier's exact envelope-failure
# verdict.  Anything else remains indeterminate/fail-closed.
benchmark_complete=0
if [[ -s "$summary" && -s "$environment" ]]; then
    if awk -F '\t' '
        NR == 1 { next }
        NF > 0 { count += 1 }
        END { exit !(count > 0) }
    ' "$summary"
    then
        benchmark_complete=1
    fi
fi

if (( benchmark_complete == 1 )) && grep -Fq "$envelope_failure" "$log"; then
    echo "ENVIRONMENT_INADMISSIBLE"
    echo "reference_commit=$reference_commit"
    echo "profile=$PROFILE"
    echo "reference_verify_make_exit=$rc"
    echo "reason=unchanged_reference_exceeds_historical_envelope"
    echo "reference_summary=$summary"
    echo "reference_environment=$environment"
    echo "The historical profile must not be used for causal candidate rejection in this session."
    exit 10
fi

echo "QUALIFICATION_INDETERMINATE" >&2
echo "reference_commit=$reference_commit" >&2
echo "profile=$PROFILE" >&2
echo "reference_verify_make_exit=$rc" >&2
echo "reference_summary_present=$([[ -s "$summary" ]] && echo yes || echo no)" >&2
echo "reference_environment_present=$([[ -s "$environment" ]] && echo yes || echo no)" >&2
echo "--- reference verifier log ---" >&2
cat "$log" >&2
exit 20
