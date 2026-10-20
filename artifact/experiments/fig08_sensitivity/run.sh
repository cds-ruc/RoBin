#!/usr/bin/env bash
# Fig. 8a-c (Findings 10-11): sensitivity to one workload variable
#   bash artifact/experiments/fig08_sensitivity/run.sh               # subset: the key cases (claims tier ~20 min; `--tier figures` adds the Fig. 8a-c cases on all four datasets, ~5 h)
#   bash artifact/experiments/fig08_sensitivity/run.sh --full        # full grid of the paper (test_ro_thp_single_thread, ~62 h)
#   bash artifact/experiments/fig08_sensitivity/run.sh --plots-only  # figures + verdicts from the released data only
# Options: --tier claims|figures|core|extended, --datasets "...", --repeat N, --force. See README.md.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/../../lib/common.sh"
experiment_main fig08_sensitivity "$@"
