#!/usr/bin/env bash
# Fig. 10-15 (Section 5): root-cause analysis from instrumented runs.
#   bash artifact/experiments/fig10_15_root_cause/run.sh               # ~2-3 h
#   bash artifact/experiments/fig10_15_root_cause/run.sh --plots-only  # figures from existing logs
# The profiling binary changes throughput, so it is built only for this step and
# the release binary is rebuilt afterwards (build.sh always recreates build/).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
cd "$ROOT"
LOG_DIR="$RESULTS/profiling_log"

if [ "${1:-}" != "--plots-only" ]; then
  check_datasets
  step "build (profiling)"
  bash build.sh profiling
  trap 'step "build (release)"; bash build.sh release' EXIT
  step "profiling cases (run_case_profiling.sh)"
  rm -rf "$LOG_DIR"; mkdir -p "$LOG_DIR"
  DATASET_DIR="$ROOT/datasets" LOG_DIR="$LOG_DIR" bash run_case_profiling.sh
fi

step "figures"
python3 "$ARTIFACT/plot/plot.py" --mode profiling --log-dir "$LOG_DIR"
