#!/usr/bin/env bash
# Fig. 16 (Section 6.1): bulkload with 1% augmented keys
#   bash artifact/experiments/fig16_data_augmentation/run.sh               # subset: the key cases (claims tier, ~25 min)
#   bash artifact/experiments/fig16_data_augmentation/run.sh --full        # full grid of the paper (test_ro_thp_single_thread + test_ro_thp_single_thread_data_augmentation, ~87 h)
#   bash artifact/experiments/fig16_data_augmentation/run.sh --plots-only  # figures + verdicts from the released data only
# Options: --tier claims|figures|core|extended, --datasets "...", --force. See README.md.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
experiment_main fig16_data_augmentation "$@"
