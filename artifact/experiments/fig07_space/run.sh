#!/usr/bin/env bash
# Fig. 7 (Findings 8-9): index space
#   bash artifact/experiments/fig07_space/run.sh               # subset: the key cases (claims tier, ~10 min)
#   bash artifact/experiments/fig07_space/run.sh --full        # full grid of the paper (test_ro_thp_single_thread + test_ro_thp_multi_thread, ~75 h)
#   bash artifact/experiments/fig07_space/run.sh --plots-only  # figures + verdicts from the released data only
# Options: --tier claims|figures|core|extended, --datasets "...", --repeat N, --force. See README.md.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
experiment_main fig07_space "$@"
