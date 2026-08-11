#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/tools/server_benchmark_common.sh"
PROFILE="$(arborcore_perf_profile)"
REFERENCE="${ARBORCORE_QUALIFICATION_REFERENCE_COMMIT:?ARBORCORE_QUALIFICATION_REFERENCE_COMMIT is required}"
ROUNDS="${ARBORCORE_PAIRED_ROUNDS:-3}"
BASELINE="$(arborcore_perf_baseline_path "$ROOT")"

[[ -r "$BASELINE" ]] || { echo "Missing historical profile: $BASELINE" >&2; exit 2; }
[[ "$ROUNDS" =~ ^[1-9][0-9]*$ ]] || { echo "Invalid paired round count" >&2; exit 2; }
git -C "$ROOT" cat-file -e "$REFERENCE^{commit}" 2>/dev/null || { echo "Missing paired reference commit: $REFERENCE" >&2; exit 2; }

workspace="$(mktemp -d /tmp/arborcore-paired.XXXXXX)"
ref="$workspace/reference"
results="$workspace/results"
mkdir -p "$results"

cleanup() {
    git -C "$ROOT" worktree remove --force "$ref" >/dev/null 2>&1 || true
}
trap cleanup EXIT

git -C "$ROOT" worktree add --detach "$ref" "$REFERENCE" >/dev/null
mkdir -p "$ref/generated"
if [[ -d "$ROOT/generated" ]]; then
    cp -a "$ROOT/generated/." "$ref/generated/"
fi

run_one() {
    local dir="$1" label="$2" round="$3" target="$4"
    local out="$results/${label}-round-${round}"
    mkdir -p "$out"

    echo "### paired $label round $round/$ROUNDS"
    (
        cd "$dir"
        make clean >/dev/null
        set +e
        make \
          ARBORCORE_PERF_PROFILE="$PROFILE" \
          "$target" \
          >"$out/run.log" 2>&1
        rc=$?
        set -e
        echo "$rc" > "$out/exit_code"
        src="build/server-performance-verify-${PROFILE}"
        test -r "$src/summary.tsv"
        cp "$src/summary.tsv" "$out/summary.tsv"
        cp "$src/environment.txt" "$out/environment.txt"
    )
}

for ((round=1; round<=ROUNDS; round++)); do
    if (( round % 2 == 1 )); then
        run_one "$ref"  reference "$round" verify-server-performance
        run_one "$ROOT" candidate "$round" verify-server-performance-candidate
    else
        run_one "$ROOT" candidate "$round" verify-server-performance-candidate
        run_one "$ref"  reference "$round" verify-server-performance
    fi
done

python - "$results" "$ROUNDS" "$BASELINE" <<'PY'
import csv
import re
import statistics
import sys
from pathlib import Path

root = Path(sys.argv[1])
rounds = int(sys.argv[2])
baseline_path = Path(sys.argv[3])

def read(label, i):
    with (root / f"{label}-round-{i}" / "summary.tsv").open(newline="") as f:
        rows = list(csv.DictReader(f, delimiter="\t"))
    return {r["metric"]: float(r["median_ns_per_op"]) for r in rows}

thresholds = {}
for line in baseline_path.read_text().splitlines():
    m = re.match(r"^([A-Z0-9_]+)_REGRESSION_THRESHOLD_PCT=([0-9.]+)$", line)
    if m:
        thresholds[m.group(1).lower()] = float(m.group(2))

refs = [read("reference", i) for i in range(1, rounds + 1)]
cands = [read("candidate", i) for i in range(1, rounds + 1)]
metrics = sorted(refs[0])

print()
print("### Paired qualification comparison")
print(f"{'metric':30s} {'paired_delta':>13s} {'threshold':>10s} {'result':>10s}")
failed = False
for metric in metrics:
    deltas = [((cands[i][metric] / refs[i][metric]) - 1.0) * 100.0 for i in range(rounds)]
    med = statistics.median(deltas)
    key = metric.replace('-', '_')
    threshold = thresholds.get(key)
    if threshold is None:
        result = "REPORT"
        threshold_text = "n/a"
    else:
        result = "PASS" if med <= threshold else "REVIEW"
        threshold_text = f"{threshold:.1f}%"
        if result == "REVIEW":
            failed = True
    print(f"{metric:30s} {med:12.3f}% {threshold_text:>10s} {result:>10s}")

print()
if failed:
    print("PAIRED_REFERENCE_RESULT=REVIEW_REQUIRED")
    sys.exit(1)
print("PAIRED_REFERENCE_RESULT=PASS")
PY

echo "Paired results retained at: $results"
