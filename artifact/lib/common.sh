# Shared steps of artifact/run_small.sh, artifact/run_all.sh and
# artifact/experiments/*/run.sh.  Source it; do not run it.

ARTIFACT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="$(cd "$ARTIFACT/.." && pwd)"
# ROBIN_RESULTS redirects the results directory, as in artifact/lib/layout.py;
# made absolute and exported so the shell steps and the Python tools agree
RESULTS="$(realpath -m "${ROBIN_RESULTS:-$ARTIFACT/results}")"
export ROBIN_RESULTS="$RESULTS"
export numanode="${numanode:-0}"

# full-grid functions of reproduce.sh that each experiment needs
declare -A FULL_TESTS=(
  [fig03_tab1_single_thread]="test_ro_thp_single_thread"
  [fig04_tab2_multi_thread]="test_ro_thp_multi_thread"
  [fig05_06_tail_latency]="test_ro_lat_single_thread test_ro_lat_multi_thread"
  [fig07_space]="test_ro_thp_single_thread test_ro_thp_multi_thread"
  [fig08_sensitivity]="test_ro_thp_single_thread"
  [fig09_out_of_bound]="test_ro_thp_single_thread test_ro_thp_single_thread_extended_oob"
  [fig16_data_augmentation]="test_ro_thp_single_thread test_ro_thp_single_thread_data_augmentation"
  [fig17_reconstruction]="test_ro_thp_single_thread test_ro_thp_single_thread_extended_bulksize"
)
# operation hours of each function at the paper's throughput (serial)
declare -A FULL_HOURS=(
  [test_ro_thp_single_thread]=62
  [test_ro_thp_multi_thread]=13
  [test_ro_lat_single_thread]=33
  [test_ro_lat_multi_thread]=4
  [test_ro_thp_single_thread_extended_bulksize]=9
  [test_ro_thp_single_thread_extended_oob]=4
  [test_ro_thp_single_thread_data_augmentation]=25
)
ALL_FULL_TESTS="test_ro_thp_single_thread test_ro_thp_multi_thread test_ro_lat_single_thread \
test_ro_lat_multi_thread test_ro_thp_single_thread_extended_bulksize \
test_ro_thp_single_thread_extended_oob test_ro_thp_single_thread_data_augmentation"

step() { printf '\n=== %s (%s) ===\n' "$1" "$(date '+%F %T')"; }

# need_value OPTION VALUE... : an option that takes a value got one (call with "$@")
need_value() {
  if [ $# -lt 2 ] || [ -z "$2" ] || [ "${2#--}" != "$2" ]; then
    echo "option $1 needs a value (see --help)" >&2
    exit 1
  fi
}

check_datasets() {
  local missing=""
  for d in linear covid fb-1 osm; do
    [ -r "$ROOT/datasets/$d" ] || missing="$missing $d"
  done
  if [ -n "$missing" ]; then
    echo "missing datasets:$missing - run: bash artifact/data/fetch.sh" >&2
    exit 1
  fi
}

# build/ must hold a finished release build: a profiling build left behind
# (bash build.sh profiling, or a fig10_15 run killed before its rebuild trap)
# or a debug build would change every throughput and latency measured
ensure_release_build() {
  local cache="$ROOT/build/CMakeCache.txt"
  if [ ! -x "$ROOT/build/microbench" ] || [ ! -r "$cache" ] \
     || ! grep -q '^CMAKE_BUILD_TYPE:[A-Z]*=Release$' "$cache" \
     || grep -q '^PROFILING:[A-Z]*=ON$' "$cache"; then
    step "build (release)"
    (cd "$ROOT" && bash build.sh release)
  fi
}

prepare_cases() {
  step "released measurements + case set"
  bash "$ARTIFACT/data/fetch_reference.sh"
  python3 "$ARTIFACT/lib/gen_cases.py"
}

# run_full_tests TEST... : run reproduce.sh functions not run yet (marker per function)
run_full_tests() {
  local force="${FORCE:-0}" tests=() total=0 t
  for t in "$@"; do
    if [ "$force" -eq 1 ] || [ ! -e "$RESULTS/.full/$t.done" ]; then
      tests+=("$t"); total=$((total + FULL_HOURS[$t]))
    else
      printf '  done before: %s\n' "$t"
    fi
  done
  [ ${#tests[@]} -eq 0 ] && return 0
  printf '  to run: %s (~%d h of operations, serial)\n' "${tests[*]}" "$total"
  mkdir -p "$RESULTS/.full"
  for t in "${tests[@]}"; do
    step "full grid: $t (~${FULL_HOURS[$t]} h)"
    (cd "$ROOT" && RESULT_DIR="$RESULTS" bash reproduce.sh "$t")
    touch "$RESULTS/.full/$t.done"
  done
  python3 "$ARTIFACT/lib/layout.py" derive-space "$RESULTS"
}

# analyze_and_plot [EXPERIMENT...] : verdicts, then the paper's figures from both sources
analyze_and_plot() {
  step "analysis + figures"
  python3 "$ARTIFACT/lib/analyze.py"
  local filter=()
  [ $# -gt 0 ] && filter=(--experiments "$@")
  python3 "$ARTIFACT/plot/plot.py" --mode paper "${filter[@]}"
  python3 "$ARTIFACT/plot/plot.py" --mode results "${filter[@]}"
}

# experiment_main NAME ARGS... : the body of experiments/<NAME>/run.sh
experiment_main() {
  local name="$1"; shift
  local full=0 plots_only=0 pass=()
  FORCE=0
  while [ $# -gt 0 ]; do
    case "$1" in
      --full) full=1; shift ;;
      --force) FORCE=1; pass+=(--force); shift ;;
      --plots-only) plots_only=1; shift ;;
      --tier) need_value "$@"; pass+=(--tier "$2"); shift 2 ;;
      --datasets|--indexes)
        need_value "$@"
        # shellcheck disable=SC2206
        local values=($2); pass+=("$1" "${values[@]}"); shift 2 ;;
      -h|--help)
        echo "usage: $0 [--full] [--tier claims|figures|core|extended] [--datasets \"...\"] [--force] [--plots-only]"
        return 0 ;;
      *) echo "unknown option: $1" >&2; return 1 ;;
    esac
  done
  cd "$ROOT"
  if [ "$plots_only" -eq 0 ]; then
    check_datasets
    ensure_release_build
    prepare_cases
    if [ "$full" -eq 1 ]; then
      # shellcheck disable=SC2086
      run_full_tests ${FULL_TESTS[$name]}
    elif python3 "$ARTIFACT/lib/run_cases.py" --experiments "$name" "${pass[@]}" --dry-run > /dev/null 2>&1; then
      step "subset: $name"
      python3 "$ARTIFACT/lib/run_cases.py" --experiments "$name" "${pass[@]}"
    else
      echo "$name has no subset cases; its figure needs the full grid: $0 --full" >&2
    fi
  else
    prepare_cases
  fi
  analyze_and_plot "$name"
}
