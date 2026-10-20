#!/usr/bin/env bash
# Fig. 9 (Finding 12): ALEX with one outlier in the bulkload
#   bash artifact/experiments/fig09_out_of_bound/run.sh               # subset: the key cases (claims tier, ~25 min)
#   bash artifact/experiments/fig09_out_of_bound/run.sh --full        # full grid of the paper (test_ro_thp_single_thread + test_ro_thp_single_thread_extended_oob, ~66 h)
#   bash artifact/experiments/fig09_out_of_bound/run.sh --plots-only  # figures + verdicts from the released data only
# Options: --tier claims|figures|core|extended, --datasets "...", --repeat N, --force. See README.md.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
experiment_main fig09_out_of_bound "$@"
