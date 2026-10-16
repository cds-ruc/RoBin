#!/usr/bin/env bash
# Fetch the four datasets of the paper into datasets/ and verify them.
#
#   covid, osm, fb   downloaded from the GRE host (datasets/download.sh)
#   linear           generated: consecutive integers 1..200M
#   fb-1             generated: fb shifted by -1
#
# Each file is 200M uint64 keys plus an 8-byte count: 1,600,000,008 bytes,
# raw (no decompression). 8.0 GB in total. Existing files are kept.
#
# Usage: bash artifact/data/fetch.sh [--verify-only]
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
cd "$ROOT/datasets"

if [ "${1:-}" != "--verify-only" ]; then
  bash download.sh
  if [ ! -e linear ] || [ ! -e fb-1 ]; then
    python3 gen_linear_fb-1.py
  fi
fi

printf 'verifying sha256 (artifact/data/SHA256SUMS)...\n'
sha256sum -c "$HERE/SHA256SUMS"
for d in covid osm fb linear fb-1; do
  printf '  %-6s %s bytes\n' "$d" "$(stat -L -c %s "$d")"
done
