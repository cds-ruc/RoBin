#!/usr/bin/env bash
# Full reproduction of RoBin: every measurement grid of the paper (~150 h of
# operations, serial), the Section 5 profiling, every figure and verdict.
#
# Each grid of reproduce.sh is recorded as done in artifact/results/.full/ once
# it finishes; an interrupted run resumes with the next grid.
#
# Usage:
#   bash artifact/run_all.sh                    # everything
#   bash artifact/run_all.sh --skip-profiling   # without Section 5 (Fig. 10-15)
#   bash artifact/run_all.sh --force            # rerun grids that already finished
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"
cd "$ROOT"

PROFILING=1
FORCE=0
while [ $# -gt 0 ]; do
  case "$1" in
    --skip-profiling) PROFILING=0; shift ;;
    --force) FORCE=1; shift ;;
    -h|--help) sed -n '2,11p' "$0"; exit 0 ;;
    *) echo "unknown option: $1" >&2; exit 1 ;;
  esac
done
export FORCE

check_datasets
ensure_release_build
prepare_cases

step "plan"
for t in $ALL_FULL_TESTS; do printf '  %-48s ~%3d h\n' "$t" "${FULL_HOURS[$t]}"; done
[ "$PROFILING" -eq 1 ] && printf '  %-48s ~  3 h\n' "Section 5 profiling (run_case_profiling.sh)"

# shellcheck disable=SC2086
run_full_tests $ALL_FULL_TESTS

if [ "$PROFILING" -eq 1 ]; then
  bash "$ARTIFACT/experiments/fig10_15_root_cause/run.sh"
fi

analyze_and_plot

step "produced"
cat <<EOF
  artifact/results/            every grid, under the paper's file names
  artifact/SUMMARY.md          the paper's headline numbers and Tables 1-2 recomputed, and your results
  artifact/figures/paper/      the paper's figures from the released measurements
  artifact/figures/results/    the same figures from your results
EOF
[ "$PROFILING" -eq 1 ] && echo "  artifact/figures/profiling/  Fig. 10-15"
true
