#!/usr/bin/env bash
# Materialise the competitor sources when `git submodule update --init` cannot
# reach github.com.  The `conf_rel` branch vendors the very same IndexRepo trees
# that the submodules point at, so they can be extracted from there.
#
# Usage: bash artifact/lib/vendor_submodules.sh
set -euo pipefail

REF_BRANCH="${REF_BRANCH:-origin/conf_rel}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$HERE/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

git -C "$REPO_ROOT" archive "$REF_BRANCH" src/competitor | tar -x -C "$TMP"

for dir in "$TMP"/src/competitor/*/src; do
  [ -d "$dir" ] || continue
  name="$(basename "$(dirname "$dir")")"
  target="$REPO_ROOT/src/competitor/$name/src"
  mkdir -p "$target"
  cp -a "$dir/." "$target/"
  printf 'vendored %s\n' "src/competitor/$name/src"
done

echo
echo "done. note: this writes untracked files inside the submodule paths;"
echo "remove them with: git submodule foreach --recursive 'git clean -xfd'"
