#!/usr/bin/env bash
# Subset reproduction of RoBin: the key cases of the throughput, latency, space
# and mitigation claims (Fig. 3-9, 16, 17), at full scale.  Fig. 10-15
# (profiling) are artifact/experiments/fig10_15_root_cause/run.sh.
#
#   released measurements -> case set -> key cases on this machine (one at a
#   time, ~4 h for the claims tier) -> claim verdicts -> every paper figure,
#   drawn from the released data and from this machine's results
#
# Usage:
#   bash artifact/run_small.sh                     # claims tier (~4 h)
#   bash artifact/run_small.sh --plots-only        # released data only: verdicts + figures (~1 min)
#   bash artifact/run_small.sh --tier figures      # after the default: the cases Fig. 5, 6, 8 draw on all datasets (~11 h)
#   bash artifact/run_small.sh --experiments "fig09_out_of_bound fig07_space"
#   bash artifact/run_small.sh --datasets "linear covid"
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"
cd "$ROOT"

TIER=claims
EXPERIMENTS=""
DATASETS=""
PLOTS_ONLY=0
FORCE=0
while [ $# -gt 0 ]; do
  case "$1" in
    --tier) TIER="$2"; shift 2 ;;
    --experiments) EXPERIMENTS="$2"; shift 2 ;;
    --datasets) DATASETS="$2"; shift 2 ;;
    --plots-only) PLOTS_ONLY=1; shift ;;
    --force) FORCE=1; shift ;;
    -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
    *) echo "unknown option: $1" >&2; exit 1 ;;
  esac
done

if [ "$PLOTS_ONLY" -eq 0 ]; then
  check_datasets
  ensure_release_build
fi
prepare_cases

if [ "$PLOTS_ONLY" -eq 0 ]; then
  step "key cases: tier $TIER"
  args=(--tier "$TIER")
  # shellcheck disable=SC2206
  [ -n "$EXPERIMENTS" ] && args+=(--experiments $EXPERIMENTS)
  # shellcheck disable=SC2206
  [ -n "$DATASETS" ] && args+=(--datasets $DATASETS)
  [ "$FORCE" -eq 1 ] && args+=(--force)
  python3 "$ARTIFACT/lib/run_cases.py" "${args[@]}"
fi

# shellcheck disable=SC2086
analyze_and_plot $EXPERIMENTS

step "produced"
cat <<EOF
  artifact/SUMMARY.md          the paper's headline numbers and Tables 1-2 recomputed (and your results, once you have them)
  artifact/figures/paper/      the paper's figures from the released measurements
EOF
if [ "$PLOTS_ONLY" -eq 0 ]; then
  cat <<EOF
  artifact/figures/results/    the same figures from your results (MANIFEST.md: coverage)
  artifact/results/            your results, under the paper's file names
EOF
fi
