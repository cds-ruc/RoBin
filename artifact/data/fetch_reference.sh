#!/usr/bin/env bash
# Export the paper's released measurements into artifact/reference/.
#
# The reference CSVs live on the `conf_rel` branch (the runs behind the paper).
# They are large, so they are not copied into every checkout; this script
# materialises them on demand.  artifact/lib/*.py and artifact/plot/plot.py read
# from artifact/reference/ and never write there.
set -euo pipefail

REF_BRANCH="${REF_BRANCH:-origin/conf_rel}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$HERE/../.." && pwd)"
TARGET="$HERE/../reference"

FILES=(
  result/single_thread_thp.csv
  result/multi_thread_thp.csv
  result/single_thread_tail_lat.csv
  result/multi_thread_tail_lat.csv
  result/memory_consumption.csv
  result/single_thread_thp.alex_lipp_extended.csv
  result/single_thread_thp.alex_oob.csv
  result/single_thread_thp.alex_lipp_preload_1_100_uint64.csv
  result/single_thread_thp.alex_lipp_preload_1_100_dataset.csv
  result/single_thread_thp.alex_lipp_preload_1_10_uint64.csv
  result/single_thread_thp.alex_lipp_preload_1_10_dataset.csv
)

if ! git -C "$REPO_ROOT" rev-parse --verify --quiet "$REF_BRANCH" > /dev/null; then
  echo "branch $REF_BRANCH not found: run 'git fetch origin conf_rel' first (or set REF_BRANCH)" >&2
  exit 1
fi

mkdir -p "$TARGET"
for file in "${FILES[@]}"; do
  target="$TARGET/$(basename "$file")"
  git -C "$REPO_ROOT" show "$REF_BRANCH:$file" > "$target"
  printf '%s <- %s (%s rows)\n' "artifact/reference/$(basename "$file")" "$REF_BRANCH:$file" "$(( $(wc -l < "$target") - 1 ))"
done
