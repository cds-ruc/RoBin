#!/usr/bin/env bash
# Fig. 5 and Fig. 6 (Finding 7): 99.9th-percentile tail latency, single- and 16-threaded
#   bash artifact/experiments/fig05_06_tail_latency/run.sh               # subset: the key cases (claims tier ~50 min; `--tier figures` adds the cases the figures draw, ~2.5 h)
#   bash artifact/experiments/fig05_06_tail_latency/run.sh --full        # full grid of the paper (test_ro_lat_single_thread + test_ro_lat_multi_thread, ~37 h)
#   bash artifact/experiments/fig05_06_tail_latency/run.sh --plots-only  # figures + verdicts from the released data only
# Options: --tier claims|figures|core|extended, --datasets "...", --force. See README.md.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
experiment_main fig05_06_tail_latency "$@"
