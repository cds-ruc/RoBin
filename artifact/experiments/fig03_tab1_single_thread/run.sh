#!/usr/bin/env bash
# Fig. 3 and Table 1 (Findings 1-4): single-threaded lookup and worst-case insert
#   bash artifact/experiments/fig03_tab1_single_thread/run.sh               # subset: the key cases (claims tier, ~1 h)
#   bash artifact/experiments/fig03_tab1_single_thread/run.sh --full        # full grid of the paper (test_ro_thp_single_thread, ~62 h)
#   bash artifact/experiments/fig03_tab1_single_thread/run.sh --plots-only  # figures + verdicts from the released data only
# Options: --tier claims|figures|core|extended, --datasets "...", --force. See README.md.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
experiment_main fig03_tab1_single_thread "$@"
