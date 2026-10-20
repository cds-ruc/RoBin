#!/usr/bin/env bash
# Fetch the paper's datasets into datasets/ and verify them: five files of
# 1,600,000,008 bytes (8.0 GB); the cases use linear, covid, fb-1 and osm.
#
#   covid, osm, fb   downloaded from the GRE host (datasets/download.sh)
#   linear           generated: consecutive integers 1..200M
#   fb-1             generated: fb shifted by -1
#
# Each file is 200M uint64 keys plus an 8-byte count: 1,600,000,008 bytes,
# raw (no decompression). 8.0 GB in total. An interrupted download is resumed;
# a file whose checksum does not match is deleted and fetched again once.
#
# Usage: bash artifact/data/fetch.sh [--verify-only]
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
cd "$ROOT/datasets"

fetch() {
  bash download.sh
  if [ ! -e linear ] || [ ! -e fb-1 ]; then
    python3 gen_linear_fb-1.py   # writes both files
  fi
}

# bad_files: names whose sha256 does not match (missing files count as bad)
bad_files() {
  local sum name
  while read -r sum name; do
    [ -n "$name" ] || continue
    if [ ! -e "$name" ] || [ "$(sha256sum "$name" | cut -d' ' -f1)" != "$sum" ]; then
      printf '%s\n' "$name"
    fi
  done < "$HERE/SHA256SUMS"
}

if [ "${1:-}" != "--verify-only" ]; then
  fetch
fi

printf 'verifying sha256 (artifact/data/SHA256SUMS)...\n'
bad="$(bad_files)"
if [ -n "$bad" ] && [ "${1:-}" != "--verify-only" ]; then
  # a corrupt file is not resumable: delete it (linear and fb-1 are generated
  # together, and fb-1 from fb) and fetch once more
  for name in $bad; do
    case "$name" in
      fb) rm -f fb fb-1 ;;
      linear|fb-1) rm -f linear fb-1 ;;
      *) rm -f "$name" ;;
    esac
    printf '  checksum mismatch: %s, deleted and fetched again\n' "$name"
  done
  fetch
  bad="$(bad_files)"
fi
if [ -n "$bad" ]; then
  printf 'checksum mismatch: %s\n' $bad >&2
  printf 'rerun: bash artifact/data/fetch.sh (it deletes a mismatching file and fetches it again)\n' >&2
  exit 1
fi
for d in covid osm fb linear fb-1; do
  printf '  %-6s %s bytes  OK\n' "$d" "$(stat -L -c %s "$d")"
done
