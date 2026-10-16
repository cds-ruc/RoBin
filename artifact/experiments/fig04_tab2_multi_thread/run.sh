#!/usr/bin/env bash
# Fig. 4 and Table 2 (Findings 5-6): 16-threaded lookup and worst-case insert
#   bash artifact/experiments/fig04_tab2_multi_thread/run.sh               # subset: the key cases (claims tier, ~5 min)
#   bash artifact/experiments/fig04_tab2_multi_thread/run.sh --full        # full grid of the paper (test_ro_thp_multi_thread, ~13 h)
#   bash artifact/experiments/fig04_tab2_multi_thread/run.sh --plots-only  # figures + verdicts from the released data only
# Options: --tier claims|figures|core|extended, --datasets "...", --force. See README.md.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
experiment_main fig04_tab2_multi_thread "$@"
