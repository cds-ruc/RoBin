#!/usr/bin/env bash
# Fig. 17 (Section 6.2): timely re-construction: bulkload fraction vs traditional indexes
#   bash artifact/experiments/fig17_reconstruction/run.sh               # subset: bulkload fractions 60%, 90%, 100% on Linear and Face (claims tier, ~25 min)
#   bash artifact/experiments/fig17_reconstruction/run.sh --full        # full grid of the paper (test_ro_thp_single_thread + test_ro_thp_single_thread_extended_bulksize, ~71 h)
#   bash artifact/experiments/fig17_reconstruction/run.sh --plots-only  # figures + verdicts from the released data only
# Options: --tier claims|figures|core|extended, --datasets "...", --repeat N, --force. See README.md.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
experiment_main fig17_reconstruction "$@"
