#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# Build standalone detector
make -C "$REPO_DIR/standalone" -s

# Generate frames and run detector
python3 "$REPO_DIR/tools/sim/sim_gen.py" > "$REPO_DIR/frames.csv"
"$REPO_DIR/standalone/detector_main" "$REPO_DIR/frames.csv"

